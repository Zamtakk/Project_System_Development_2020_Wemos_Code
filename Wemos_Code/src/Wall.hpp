// Defines

const char DEVICE_TYPE[] = "Wall";

#define NUMBER_OF_LEDS 3

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool curtainOpen = false;
uint16_t LDRValue = 0;
uint16_t dimmerValue = 0;
int ledstripValue = 0;

// Forward Declaration

void handleMessage(JsonObject message);

void updateLedstrip();

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
    updateAnalogI2CInputs(10, 20, 255, &LDRValue, WALL_LDR_VALUE, &dimmerValue, WALL_DIMMER_VALUE);

    updateDigitalI2COutputs(&curtainOpen);

    updateLedstrip();

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
        deviceInfoMessage["WALL_CURTAIN_OPEN"] = curtainOpen;
        deviceInfoMessage["WALL_DIMMER_VALUE"] = dimmerValue;
        deviceInfoMessage["WALL_LDR_VALUE"] = LDRValue;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICE_INFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case WALL_CURTAIN_OPEN:
    {
        curtainOpen = (bool)message["value"];
        break;
    }

    case WALL_LEDSTRIP_VALUE:
    {
        ledstripValue = (int)message["value"];
        break;
    }

    default:
        Serial.printf("[Error] Unsupported command received: %d\n", (int)message["command"]);
        break;
    }
}

/*!
    @brief Checks if the ledstrip state is still the same as the previous and updates the ledstrip accordingly
*/
void updateLedstrip()
{
    static int ledstrip_Previous = 0;

    if (ledstripValue != ledstrip_Previous)
    {
        ledstrip_Previous = ledstripValue;
        setFastLedRGBColor(ledstripValue, ledstripValue, ledstripValue);
        Serial.printf("Ledstrip updated to %d!\n", ledstripValue);
    }
}