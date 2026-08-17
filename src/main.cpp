#include "core/Runtime.hpp"
#include <pl/Mod.hpp>

class InventorySorterMod {
public:
    static InventorySorterMod& instance() {
        static InventorySorterMod mod;
        return mod;
    }

    bool load(pl::mod::ModContext& context) { return inventorysorter::core::Runtime::get().load(context); }
    bool enable(pl::mod::ModContext& context) { return inventorysorter::core::Runtime::get().enable(context); }
    bool disable(pl::mod::ModContext& context) { return inventorysorter::core::Runtime::get().disable(context); }
    bool unload(pl::mod::ModContext& context) { return inventorysorter::core::Runtime::get().unload(context); }
};

PL_REGISTER_MOD(InventorySorterMod, InventorySorterMod::instance())
