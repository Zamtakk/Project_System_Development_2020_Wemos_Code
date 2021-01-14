#define WALL

#ifdef CHAIR
#include "Chair.hpp"
#endif

#ifdef SIMULATED_DEVICE
#include "SimulatedDevice.hpp"
#endif

#ifdef BED
#include "Bed.hpp"
#endif

#ifdef DOOR
#include "Door.hpp"
#endif

#ifdef WIB
#include "WIB.hpp"
#endif

#ifdef WALL
#include "Wall.hpp"
#endif