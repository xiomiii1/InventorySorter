#pragma once
#include <cstdint>
#include <dlfcn.h>
#include <new>
#include <pl/memory/Hook.hpp>

namespace inventorysorter::hooks {
using LibraryHandle = void*;
struct State { void* target; void* detour; };
using Handle = State*;
inline LibraryHandle openLibrary(const char* name) {
    if (!name) return nullptr;
    void* h = dlopen(name, RTLD_NOW | RTLD_NOLOAD);
    return h ? h : dlopen(name, RTLD_NOW);
}
inline void closeLibrary(LibraryHandle h) { if (h) dlclose(h); }
inline std::uintptr_t symbol(LibraryHandle h, const char* name) {
    return (h && name) ? reinterpret_cast<std::uintptr_t>(dlsym(h, name)) : 0;
}
inline Handle install(void* target, void* detour, void** original) {
    if (!target || !detour || pl::memory::hook(target, detour, original) != 0) return nullptr;
    auto* state = new (std::nothrow) State{target, detour};
    if (!state) pl::memory::unhook(target, detour);
    return state;
}
inline void remove(Handle h) {
    if (!h) return;
    pl::memory::unhook(h->target, h->detour);
    delete h;
}
}
