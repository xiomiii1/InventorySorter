#include "Events.hpp"
namespace inventorysorter::events { Bus& bus(){ static Bus b; return b; } }
