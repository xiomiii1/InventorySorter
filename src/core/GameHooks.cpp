#include "GameHooks.hpp"
#include "runtime/Hooks.hpp"
#include "runtime/Events.hpp"
#include <inventorysorter/Signatures.hpp>
#include "../version.hpp"
#include <EGL/egl.h>
#include <array>
#include <atomic>
#include <mutex>

namespace inventorysorter::core::gamehooks {
namespace {
using ContainerFn = void*(*)(void*,void*,void*,void*,void*,void*,void*,void*);
using EglSwapFn = EGLBoolean(*)(EGLDisplay,EGLSurface);
ContainerFn openOriginal=nullptr, closeOriginal=nullptr;
EglSwapFn swapOriginal=nullptr;
std::array<inventorysorter::hooks::Handle,3> handles{};
size_t count=0;
std::mutex mutex;

template<class F> bool hookSig(inventorysorter::memory::SignatureId id, void* detour, F** original){
    auto a=inventorysorter::memory::resolve(id); if(!a) return false;
    auto h=inventorysorter::hooks::install(reinterpret_cast<void*>(a),detour,reinterpret_cast<void**>(original));
    if(!h) return false; handles[count++]=h; return true;
}
void* containerOpen(void* a0,void* a1,void* a2,void* a3,void* a4,void* a5,void* a6,void* a7){
    inventorysorter::events::ScreenStateEvent e{inventorysorter::events::ScreenKind::Container,inventorysorter::events::ScreenPhase::Opened,a0};
    inventorysorter::events::bus().publish(e);
    return openOriginal ? openOriginal(a0,a1,a2,a3,a4,a5,a6,a7) : nullptr;
}
void* containerClose(void* a0,void* a1,void* a2,void* a3,void* a4,void* a5,void* a6,void* a7){
    inventorysorter::events::ScreenStateEvent e{inventorysorter::events::ScreenKind::Container,inventorysorter::events::ScreenPhase::Closed,a0};
    inventorysorter::events::bus().publish(e);
    return closeOriginal ? closeOriginal(a0,a1,a2,a3,a4,a5,a6,a7) : nullptr;
}
EGLBoolean swapDetour(EGLDisplay d,EGLSurface s){
    if(eglGetCurrentContext()!=EGL_NO_CONTEXT){ inventorysorter::events::FrameEvent e; inventorysorter::events::bus().publish(e); }
    return swapOriginal ? swapOriginal(d,s) : EGL_FALSE;
}
}

bool install(){
    std::lock_guard lock(mutex);
    if(count) return true;
    hookSig(inventorysorter::memory::SignatureId::ContainerScreenControllerOpen,reinterpret_cast<void*>(containerOpen),&openOriginal);
    hookSig(inventorysorter::memory::SignatureId::ContainerScreenControllerDtor,reinterpret_cast<void*>(containerClose),&closeOriginal);
    auto egl=inventorysorter::hooks::openLibrary("libEGL.so");
    if(egl){ auto sym=inventorysorter::hooks::symbol(egl,"eglSwapBuffers"); if(sym){ auto h=inventorysorter::hooks::install(reinterpret_cast<void*>(sym),reinterpret_cast<void*>(swapDetour),reinterpret_cast<void**>(&swapOriginal)); if(h) handles[count++]=h; } inventorysorter::hooks::closeLibrary(egl); }
    return openOriginal!=nullptr && closeOriginal!=nullptr;
}
void uninstall(){ std::lock_guard lock(mutex); while(count) inventorysorter::hooks::remove(handles[--count]); }
void* clientInstance(){ return nullptr; }
}
