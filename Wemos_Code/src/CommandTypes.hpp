#ifndef COMMANDTYPES_HPP
#define COMMANDTYPES_HPP

enum ErrorCodes
{
    NOT_REGISTERED = 0,
    INVALID_FORMAT = 1
};

enum GeneralDeviceCommands
{
    HEARTBEAT = 1000,
    REGISTRATION = 1001,
    DEVICEINFO = 1002
};

enum DeviceStatus
{
    CONNECTED = 2000,
    UNSTABLE = 2001,
    DISCONNECTED = 2002
};

enum SimulatedDevice
{
    BUTTON_1_CHANGE = 3000,
    BUTTON_2_CHANGE = 3001,
    POTMETER_CHANGE = 3002,
    LED_1_CHANGE = 3003,
    LED_2_CHANGE = 3004,
    LED_3_CHANGE = 3005
};

#endif