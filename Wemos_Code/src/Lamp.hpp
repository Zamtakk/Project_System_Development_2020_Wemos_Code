// Defines

const char DEVICE_TYPE[] = "Lamp";

#define NUMBER_OF_LEDS 1

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool input0State = false;

int ledValue = 0;

// Forward Declaration

void handleMessage(JsonObject message);

void handleDigitalInput();
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
    updateDigitalI2CInputs(&input0State, LAMP_MOVEMENTSENSOR_CHANGE);

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
    case DEVICEINFO:
    {
        StaticJsonDocument<200> deviceInfoMessage;
        char stringMessage[200];

        deviceInfoMessage["UUID"] = UUID;
        deviceInfoMessage["Type"] = DEVICE_TYPE;
        deviceInfoMessage["command"] = DEVICEINFO;
        deviceInfoMessage["ledValue"] = ledValue;
        deviceInfoMessage["movementSensorValue"] = input0State;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case LAMP_LED_DIMMER_CHANGE:
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

    if (ledValue != ledValue_Previous || firstTime)
    {
        ledValue_Previous = ledValue;
        setFastLedRGBColor(ledValue, ledValue, ledValue);
        Serial.printf("Lamp updated to %d!\n", ledValue);
    }

    if (firstTime)
        firstTime = false;
}
