#define BED

#ifdef CHAIR
#include "DeviceChair.hpp"
#endif

#ifdef SIMULATED_DEVICE
#include "SimulatedDevice.hpp"
#endif

#ifdef BED
#include "BedDevice.hpp"
#endif