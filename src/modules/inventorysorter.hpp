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

    /*
     * ModMenu action button.
     *
     * This is NOT intended to be a persistent toggle.
     */
    bool m_sortInventory = false;

    /*
     * Configuration options.
     */
    bool m_includeHotbar = true;
    bool m_sortByDamage = true;

    InventorySorterModule();

    void onInit() override;
    void onDisable() override;

    /*
     * Called when the ModMenu
     * "Sort Inventory" button is pressed.
     */
    void triggerSortButton();

    void onFrame() override;

    void loadConfig(
        const nlohmann::json& j
    ) override;

    void saveConfig(
        nlohmann::json& j
    ) override;

private:
    struct ClickAction {
        std::string collection;
        int index = -1;
    };

    /*
     * Sorting state.
     */
    void requestSort();
    void buildSortPlan();
    void clearSortState();

    /*
     * Inventory reading.
     */
    bool readCollection(
        const std::string& collection,
        int size,
        std::vector<ItemKey>& out
    ) const;

    /*
     * Build the native slot-click sequence.
     */
    void appendSortPlan(
        const std::string& collection,
        std::vector<ItemKey> items
    );

    /*
     * Execute one native controller action.
     */
    bool executeClick(
        const ClickAction& action
    );

    /*
     * Current container controller.
     */
    void* m_controller = nullptr;

    /*
     * Pending native slot operations.
     */
    std::vector<ClickAction> m_plan;

    std::size_t m_planIndex = 0;

    /*
     * Delay between native controller actions.
     */
    std::uint32_t m_frameDelay = 0;

    /*
     * True while a sort operation is being executed.
     */
    bool m_sorting = false;

    /*
     * Set when the user presses Sort Inventory.
     */
    bool m_sortRequested = false;

    /*
     * Native signatures successfully resolved.
     */
    bool m_hooksResolved = false;
};
