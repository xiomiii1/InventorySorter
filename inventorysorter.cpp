#include "inventorysorter.hpp"

#include "core/runtime/Events.hpp"
#include <inventorysorter/Signatures.hpp>
#include "core/runtime/Offsets.hpp"
#include <algorithm>
#include <cstddef>

namespace {
using ContainerSlotSelectedFn = std::uint32_t (*)(void*, const std::string&, int);
using ContainerGetItemStackFn = void* (*)(void*, const std::string&, int);
using ItemStackBaseGetDamageValueFn = short (*)(void*);

ContainerSlotSelectedFn g_selectSlot = nullptr;
ContainerGetItemStackFn g_getItemStack = nullptr;
ItemStackBaseGetDamageValueFn g_getDamage = nullptr;
InventorySorterModule* g_instance = nullptr;

void* getStackItem(void* stack) {
    if (!stack) return nullptr;
    auto* base = reinterpret_cast<std::byte*>(stack);
    void* counter = *reinterpret_cast<void**>(base + inventorysorter::offsets::ItemStackBaseItem);
    if (!counter) return nullptr;
    return *reinterpret_cast<void**>(reinterpret_cast<std::byte*>(counter) + inventorysorter::offsets::SharedCounterPointer);
}

std::uint16_t getItemId(void* item) {
    if (!item) return 0;
    return *reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::byte*>(item) + inventorysorter::offsets::ItemId);
}

bool isEmptyStack(void* stack) {
    return getStackItem(stack) == nullptr;
}

bool lessKey(const InventorySorterModule::ItemKey& a, const InventorySorterModule::ItemKey& b, bool sortByDamage) {
    if (a.occupied != b.occupied) return a.occupied > b.occupied;
    if (!a.occupied) return false;
    if (a.itemId != b.itemId) return a.itemId < b.itemId;
    if (sortByDamage && a.damage != b.damage) return a.damage < b.damage;
    return false;
}

} // namespace

InventorySorterModule::InventorySorterModule()
    : Module("InventorySorter", "Adds a Sort Inventory button that sorts the player's inventory using the native container controller.") {
    masterEnabled = true;
    g_instance = this;
}

void InventorySorterModule::onInit() {
    if (m_hooksResolved) return;

    g_selectSlot = reinterpret_cast<ContainerSlotSelectedFn>(
        inventorysorter::memory::resolve(inventorysorter::memory::SignatureId::ContainerScreenControllerOnContainerSlotSelected));
    g_getItemStack = reinterpret_cast<ContainerGetItemStackFn>(
        inventorysorter::memory::resolve(inventorysorter::memory::SignatureId::ContainerScreenControllerGetItemStack));
    g_getDamage = reinterpret_cast<ItemStackBaseGetDamageValueFn>(
        inventorysorter::memory::resolve(inventorysorter::memory::SignatureId::ItemStackBaseGetDamageValue));

    inventorysorter::events::bus().subscribe<inventorysorter::events::ScreenStateEvent>([](auto& event) {
        if (!g_instance || event.screen != inventorysorter::events::ScreenKind::Container) return;
        if (event.phase == inventorysorter::events::ScreenPhase::Opened) {
            g_instance->m_controller = event.controller;
        } else {
            if (g_instance->m_controller == event.controller) {
                g_instance->clearSortState();
                g_instance->m_controller = nullptr;
            }
        }
    });

    m_hooksResolved = g_selectSlot != nullptr && g_getItemStack != nullptr;
}

void InventorySorterModule::onDisable() {
    clearSortState();
    m_sortRequested = false;
}

void InventorySorterModule::loadConfig(const nlohmann::json& j) {
    Module::loadConfig(j);
    if (j.contains("m_includeHotbar")) m_includeHotbar = j["m_includeHotbar"].get<bool>();
    if (j.contains("m_sortByDamage")) m_sortByDamage = j["m_sortByDamage"].get<bool>();
    if (j.contains("m_sortButton") && j["m_sortButton"].get<bool>()) {
        requestSort();
        m_sortButton = false;
    }
}

void InventorySorterModule::saveConfig(nlohmann::json& j) {
    Module::saveConfig(j);
    j["m_sortButton"] = false;
    j["m_includeHotbar"] = m_includeHotbar;
    j["m_sortByDamage"] = m_sortByDamage;
}

void InventorySorterModule::requestSort() {
    if (!enabled || !m_controller || !m_hooksResolved || m_sorting) return;
    m_sortRequested = true;
}

void InventorySorterModule::clearSortState() {
    m_plan.clear();
    m_planIndex = 0;
    m_frameDelay = 0;
    m_sorting = false;
}

bool InventorySorterModule::readCollection(const std::string& collection, int size, std::vector<ItemKey>& out) const {
    if (!m_controller || !g_getItemStack || size <= 0) return false;
    out.assign(static_cast<std::size_t>(size), {});

    for (int i = 0; i < size; ++i) {
        void* stack = g_getItemStack(m_controller, collection, i);
        if (isEmptyStack(stack)) continue;
        auto* item = getStackItem(stack);
        if (!item) continue;

        ItemKey key;
        key.occupied = true;
        key.itemId = getItemId(item);
        if (m_sortByDamage && g_getDamage) key.damage = g_getDamage(stack);
        out[static_cast<std::size_t>(i)] = key;
    }
    return true;
}

void InventorySorterModule::appendSortPlan(const std::string& collection, std::vector<ItemKey> items) {
    if (items.empty()) return;

    for (std::size_t i = 0; i < items.size(); ++i) {
        std::size_t best = i;
        for (std::size_t j = i + 1; j < items.size(); ++j) {
            if (lessKey(items[j], items[best], m_sortByDamage)) best = j;
        }
        if (best == i) continue;

        m_plan.push_back(ClickAction{collection, static_cast<int>(best)});
        m_plan.push_back(ClickAction{collection, static_cast<int>(i)});
        std::swap(items[i], items[best]);
    }
}

void InventorySorterModule::buildSortPlan() {
    clearSortState();
    if (!m_controller || !g_selectSlot || !g_getItemStack) return;

    std::vector<ItemKey> inventory;
    if (!readCollection("inventory_items", 27, inventory)) return;
    appendSortPlan("inventory_items", std::move(inventory));

    if (m_includeHotbar) {
        std::vector<ItemKey> hotbar;
        if (readCollection("hotbar_items", 9, hotbar)) appendSortPlan("hotbar_items", std::move(hotbar));
    }

    m_sorting = !m_plan.empty();
    m_sortRequested = false;
    m_planIndex = 0;
    m_frameDelay = 0;
}

bool InventorySorterModule::executeClick(const ClickAction& action) {
    if (!m_controller || !g_selectSlot || action.index < 0) return false;
    g_selectSlot(m_controller, action.collection, action.index);
    return true;
}

void InventorySorterModule::onFrame() {
    if (!enabled) return;

    if (m_sortRequested && !m_sorting) buildSortPlan();
    if (!m_sorting) return;

    if (m_frameDelay > 0) {
        --m_frameDelay;
        return;
    }

    if (m_planIndex >= m_plan.size()) {
        clearSortState();
        return;
    }

    const auto& first = m_plan[m_planIndex++];
    if (!executeClick(first)) {
        clearSortState();
        return;
    }

    if (m_planIndex < m_plan.size()) {
        const auto& second = m_plan[m_planIndex];
        if (second.collection == first.collection) {
            executeClick(second);
            ++m_planIndex;
        }
    }

    // Let the native controller/UI process the pair before the next swap.
    m_frameDelay = 1;
}
