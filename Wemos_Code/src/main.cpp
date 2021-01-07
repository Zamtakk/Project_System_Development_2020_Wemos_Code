#define DOOR

#ifdef CHAIR
#include "DeviceChair.hpp"
#endif

#ifdef SIMULATED_DEVICE
#include "SimulatedDevice.hpp"
#endif

#ifdef DOOR
#include "DeviceDoor.hpp"
#endif