#include "Runtime.hpp"
#include "GameHooks.hpp"
#include "config/ConfigManager.hpp"
#include "launcher/ModuleMenu.hpp"
#include "modules/ModuleRegistry.hpp"
#include "runtime/Hooks.hpp"
#include "runtime/Events.hpp"
#include <inventorysorter/Signatures.hpp>
#include <pl/Input.hpp>
#include <atomic>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <mutex>
#include <unistd.h>

namespace inventorysorter::core {
namespace {
std::atomic_bool enabled=false, resolved=false, installed=false;
std::mutex resolveMutex, installMutex;
thread_local bool resolving=false;
void* (*dlopenOriginal)(const char*,int)=nullptr;
inventorysorter::hooks::Handle dlopenHook=nullptr;
bool wired=false; int containerDepth=0;
void* dlopenDetour(const char* filename,int flags){
    void* h=dlopenOriginal?dlopenOriginal(filename,flags):nullptr;
    if(h && filename && std::strstr(filename,"libminecraftpe.so") && !resolving) Runtime::get().minecraftLoaded();
    return h;
}
struct ResolveGuard { ResolveGuard(){ old=resolving; resolving=true;} ~ResolveGuard(){resolving=old;} bool old; };
}
Runtime& Runtime::get(){ static Runtime r; return r; }
const std::filesystem::path& Runtime::resourceDirectory() const noexcept { return mResourceDirectory; }
bool Runtime::launcherContext() const {
    int fd=open("/proc/self/cmdline",O_RDONLY); if(fd<0) return false; char cmd[256]{}; auto n=read(fd,cmd,sizeof(cmd)-1); close(fd); if(n<=0) return false;
    return std::strcmp(cmd,"org.levimc.launcher")==0 || std::strcmp(cmd,"org.levimc.launcher:minecraft")==0 || std::strcmp(cmd,"com.mojang.minecraftpe")==0;
}
bool Runtime::resolveSignatures(){
    std::lock_guard lock(resolveMutex); if(resolved) return true; ResolveGuard g; bool ok=inventorysorter::memory::resolveAll(); resolved=ok; return ok;
}
void Runtime::wireEvents(){
    if(wired) return; wired=true;
    inventorysorter::events::bus().subscribe<inventorysorter::events::FrameEvent>([](auto&){ ModuleRegistry::get().onFrame(); });
    inventorysorter::events::bus().subscribe<inventorysorter::events::ScreenStateEvent>([](auto& e){
        if(e.phase==inventorysorter::events::ScreenPhase::Opened) ++containerDepth; else if(containerDepth>0) --containerDepth;
        ModuleRegistry::get().setKeybindBlocked(containerDepth>0);
    });
}
bool Runtime::install(){
    std::lock_guard lock(installMutex); if(installed) return true; if(!resolved && !resolveSignatures()) return false; if(!gamehooks::install()) return false;
    registerAllModules(); wireEvents(); ModuleRegistry::get().initialize(); inventorysorter::config::ConfigManager::get().load(); registerModulesWithLauncher(); installed=true; return true;
}
void Runtime::minecraftLoaded(){ if(resolveSignatures() && enabled) install(); }
bool Runtime::load(pl::mod::ModContext& context){
    mResourceDirectory=context.resourceDir(); inventorysorter::config::ConfigManager::get().setConfigPath((context.configDir()/"config.json").string()); if(!launcherContext()) return true;
    void* minecraft=dlopen("libminecraftpe.so",RTLD_NOW|RTLD_NOLOAD); if(minecraft){resolveSignatures(); dlclose(minecraft); return true;}
    auto lib=inventorysorter::hooks::openLibrary("libdl.so"); if(lib){ auto sym=inventorysorter::hooks::symbol(lib,"dlopen"); if(sym) dlopenHook=inventorysorter::hooks::install(reinterpret_cast<void*>(sym),reinterpret_cast<void*>(dlopenDetour),reinterpret_cast<void**>(&dlopenOriginal)); inventorysorter::hooks::closeLibrary(lib); }
    return true;
}
bool Runtime::enable(pl::mod::ModContext&){ enabled=true; if(!launcherContext()) return true; void* minecraft=dlopen("libminecraftpe.so",RTLD_NOW|RTLD_NOLOAD); if(!minecraft) return true; resolveSignatures(); dlclose(minecraft); install(); return true; }
bool Runtime::disable(pl::mod::ModContext&){ enabled=false; inventorysorter::config::ConfigManager::get().flush(); return true; }
bool Runtime::unload(pl::mod::ModContext&){ enabled=false; inventorysorter::config::ConfigManager::get().flush(); return true; }
}
