#include <inventorysorter/Signatures.hpp>
#include <array>
#include <string>
#include <vector>
#include <pl/memory/Signature.hpp>

namespace inventorysorter::memory {
namespace {
constexpr auto defs = std::array{
    SignatureDefinition{SignatureId::ContainerScreenControllerOpen, "? ? ? A9 ? ? ? F9 FD 03 00 91 F3 03 00 AA ? ? ? 94 ? ? ? F9 E1 03 1F 2A ? ? ? 94"},
    SignatureDefinition{SignatureId::ContainerScreenControllerDtor, "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 56 D0 3B D5 F3 03 00 AA ? ? ? F9 ? ? ? F9 ? ? ? 90 ? ? ? 91 ? ? ? F9 ? ? ? F9 ? ? ? 91 ? ? ? F9 ? ? ? 94"},
    SignatureDefinition{SignatureId::ContainerScreenControllerOnContainerSlotSelected, "? ? ? A9 FD 03 00 91 ? ? ? F9 ? ? ? F9 00 01 3F D6 E0 03 1F 2A ? ? ? A8 C0 03 5F D6 ? ? ? D1"},
    SignatureDefinition{SignatureId::ContainerScreenControllerGetItemStack, "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? 91 54 D0 3B D5 F3 03 00 AA ? ? ? 91 ? ? ? F9 ? ? ? F8 ? ? ? 95 ? ? ? F9 ? ? ? F9 ? ? ? 91"},
    SignatureDefinition{SignatureId::ItemStackBaseGetDamageValue, "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 55 D0 3B D5 ? ? ? F9 ? ? ? F8 ? ? ? F9 ? ? ? B4 ? ? ? F9 ? ? ? B4 E8 03 00 AA"},
};
std::array<std::uintptr_t, static_cast<std::size_t>(SignatureId::Count)> addresses{};
}

bool resolveAll(std::string_view libraryName) {
    std::vector<std::string> patterns;
    patterns.reserve(defs.size());
    for (const auto& d : defs) patterns.emplace_back(d.pattern);
    const auto found = pl::memory::resolveSignatures(patterns, std::string(libraryName).c_str());
    addresses.fill(0);
    bool all = true;
    for (std::size_t i = 0; i < defs.size(); ++i) {
        auto it = found.find(patterns[i]);
        if (it == found.end() || it->second == 0) { all = false; continue; }
        addresses[static_cast<std::size_t>(defs[i].id)] = it->second;
    }
    return all;
}

std::uintptr_t resolve(SignatureId id) {
    const auto i = static_cast<std::size_t>(id);
    return i < addresses.size() ? addresses[i] : 0;
}

void clear() { addresses.fill(0); }
}
