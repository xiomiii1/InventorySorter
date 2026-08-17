#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace inventorysorter::memory {

enum class SignatureId : std::uint8_t {
    ContainerScreenControllerOpen,
    ContainerScreenControllerDtor,
    ContainerScreenControllerOnContainerSlotSelected,
    ContainerScreenControllerGetItemStack,
    ItemStackBaseGetDamageValue,
    Count
};

struct SignatureDefinition {
    SignatureId id;
    std::string_view pattern;
};

bool resolveAll(std::string_view libraryName = "libminecraftpe.so");
std::uintptr_t resolve(SignatureId id);
void clear();

}
