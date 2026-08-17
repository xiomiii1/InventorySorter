#pragma once

#include "Module.hpp"
#include <cstdint>
#include <string>
#include <vector>

class InventorySorterModule final : public Module {
public:
    struct ItemKey {
        bool occupied = false;
        std::uint16_t itemId = 0;
        std::int16_t damage = 0;
    };

    bool m_sortButton = false;
    bool m_includeHotbar = true;
    bool m_sortByDamage = true;

    InventorySorterModule();

    void onInit() override;
    void onDisable() override;
    void onFrame() override;
    void loadConfig(const nlohmann::json& j) override;
    void saveConfig(nlohmann::json& j) override;

private:
    struct ClickAction {
        std::string collection;
        int index = -1;
    };

    void requestSort();
    void buildSortPlan();
    void clearSortState();
    bool readCollection(const std::string& collection, int size, std::vector<ItemKey>& out) const;
    void appendSortPlan(const std::string& collection, std::vector<ItemKey> items);
    bool executeClick(const ClickAction& action);

    void* m_controller = nullptr;
    std::vector<ClickAction> m_plan;
    std::size_t m_planIndex = 0;
    std::uint32_t m_frameDelay = 0;
    bool m_sorting = false;
    bool m_sortRequested = false;
    bool m_hooksResolved = false;
};
