// Defines

const char DEVICE_TYPE[] = "Fridge";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool input0State = false;
bool output0State = false;
uint16_t analogInput0Value = 0;
uint16_t analogInput1Value = 0;

int temperatureInside = 0;
int temperatureOutside = 0;
bool doorOpen = false;
bool coolingOn = false;

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
    updateDigitalI2CInputs(&input0State, FRIDGE_SWITCH_CHANGE);

    updateAnalogI2CInputs(5, 3, 1023, &analogInput0Value, FRIDGE_TEMPERATURESENSORINSIDE_CHANGE/*, &analogInput1Value, FRIDGE_TEMPERATURESENSOROUTSIDE_CHANGE*/);

    updateDigitalI2COutputs(&output0State);

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
    case DEVICEINFO:
    {
        StaticJsonDocument<200> deviceInfoMessage;
        char stringMessage[200];

        deviceInfoMessage["UUID"] = UUID;
        deviceInfoMessage["Type"] = DEVICE_TYPE;
        deviceInfoMessage["command"] = DEVICEINFO;
        deviceInfoMessage["temperatureValueInside"] = temperatureInside;
        deviceInfoMessage["temperatureValueOutside"] = temperatureOutside;
        deviceInfoMessage["doorOpen"] = !input0State;
        deviceInfoMessage["coolingOn"] = coolingOn;
        deviceInfoMessage["fanOn"] = output0State;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case FRIDGE_FAN_CHANGE:
    {
        output0State = (bool)message["value"];
        break;
    }

    case FRIDGE_TEC_CHANGE:
    {
        coolingOn = (bool)message["value"];
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

    if (coolingOn != coolingOn_Previous || firstTime)
    {
        coolingOn_Previous = coolingOn;
        digitalWrite(DIRECT_OUTPUT_PIN, coolingOn);
        Serial.printf("Cooling ON changed to %d!\n", coolingOn);
    }

    if (firstTime)
        firstTime = false;
}
