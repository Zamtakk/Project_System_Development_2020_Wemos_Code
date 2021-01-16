// Defines

const char DEVICE_TYPE[] = "Bed";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool input0State = false;
bool output0State = false;
uint16_t analogInput0Value = 0;

// Forward Declaration

void handleMessage(JsonObject message);

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
    updateDigitalI2CInputs(&input0State, BED_BUTTON_CHANGE);

    updateAnalogI2CInputs(5, 8, 255, &analogInput0Value, BED_FORCESENSOR_CHANGE);

    updateDigitalI2COutputs(&output0State);

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
        deviceInfoMessage["ledState"] = output0State;
        deviceInfoMessage["pressureValue"] = analogInput0Value;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case BED_LED_CHANGE:
    {
        output0State = (bool)message["value"];
        break;
    }

    default:
        Serial.printf("[Error] Unsupported command received: %d\n", (int)message["command"]);
        break;
    }
}