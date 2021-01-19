// Defines

const char DEVICE_TYPE[] = "Lamp";

#define NUMBER_OF_LEDS 1

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool movementDetected = false;
int ledValue = 0;

// Forward Declaration

void handleMessage(JsonObject message);

void updateLED();

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
    updateDigitalI2CInputs(&movementDetected, LAMP_MOVEMENT_DETECTED);

    updateLED();

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
        deviceInfoMessage["LAMP_LED_VALUE"] = ledValue;
        deviceInfoMessage["LAMP_MOVEMENT_DETECTED"] = movementDetected;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICE_INFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case LAMP_LED_VALUE:
    {
        ledValue = (int)message["value"];
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
void updateLED()
{
    static bool firstTime = true;
    static int ledValue_Previous = 0;

    if (firstTime)
    {
        firstTime = false;
        ledValue_Previous = ledValue;
        return;
    }

    if (ledValue != ledValue_Previous)
    {
        ledValue_Previous = ledValue;
        setFastLedRGBColor(ledValue, ledValue, ledValue);
        Serial.printf("Lamp updated to %d!\n", ledValue);
    }
}
