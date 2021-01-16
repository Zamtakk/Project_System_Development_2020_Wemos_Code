// Defines

const char DEVICE_TYPE[] = "Fridge";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool doorClosed = false;
bool fanOn = false;
uint16_t rawTemperatureSensorInsideValue = 0;
uint16_t rawTemperatureSensorOutsideValue = 0;
bool coolerOn = false;

// Forward Declaration

void handleMessage(JsonObject message);
void UpdateCooler();

// Setup

void setup()
{
    initSerial();

    initI2C();

    generateUUID();

    initWifi();

    initWebsocket(&handleMessage, DEVICE_TYPE);
}

// Loop

void loop()
{
    updateDigitalI2CInputs(&doorClosed, FRIDGE_DOOR_CLOSED);

    updateAnalogI2CInputs(5, 3, 1023, &rawTemperatureSensorInsideValue, FRIDGE_RAW_TEMPERATURE_SENSOR_INSIDE_VALUE/*, &rawTemperatureSensorOutsideValue, FRIDGE_RAW_TEMPERATURE_SENSOR_OUTSIDE_VALUE*/);

    updateDigitalI2COutputs(&fanOn);

    UpdateCooler();

    sendHeartbeat();

    webSocket.loop();
}

// Function definitions

/*!
    @brief Reads the incoming message and activates the actuators or sends devices information back.
*/
void handleMessage(JsonObject message)
{
    switch ((int)message["command"])
    {
    case DEVICE_INFO:
    {
        StaticJsonDocument<200> deviceInfoMessage;
        char stringMessage[200];

        deviceInfoMessage["UUID"] = UUID;
        deviceInfoMessage["Type"] = DEVICE_TYPE;
        deviceInfoMessage["command"] = DEVICE_INFO;
        deviceInfoMessage["FRIDGE_DOOR_CLOSED"] = doorClosed;
        deviceInfoMessage["FRIDGE_FAN_ON"] = fanOn;
        deviceInfoMessage["FRIDGE_RAW_TEMPERATURE_SENSOR_INSIDE_VALUE"] = rawTemperatureSensorInsideValue;
        deviceInfoMessage["FRIDGE_RAW_TEMPERATURE_SENSOR_OUTSIDE_VALUE"] = rawTemperatureSensorOutsideValue;
        deviceInfoMessage["FRIDGE_COOLER_ON"] = coolerOn;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICE_INFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case FRIDGE_FAN_ON:
    {
        fanOn = (bool)message["value"];
        break;
    }

    case FRIDGE_COOLER_ON:
    {
        coolerOn = (bool)message["value"];
        break;
    }

    default:
        Serial.printf("[Error] Unsupported command received: %d\n", (int)message["command"]);
        break;
    }
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void UpdateCooler()
{
    static bool firstTime = true;
    static bool coolingOn_Previous = false;

    if (coolerOn != coolingOn_Previous || firstTime)
    {
        coolingOn_Previous = coolerOn;
        digitalWrite(DIRECT_OUTPUT_PIN, coolerOn);
        Serial.printf("Cooling ON changed to %d!\n", coolerOn);
    }

    if (firstTime)
        firstTime = false;
}
