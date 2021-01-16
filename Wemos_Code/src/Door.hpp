// Defines

const char DEVICE_TYPE[] = "Door";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

#define DOOR_OPEN_ANGLE 0
#define DOOR_CLOSED_ANGLE 83

// Global variables

bool buttonInsidePressed = false;
bool buttonOutsidePressed = false;
bool ledInsideOn = false;
bool ledOutsideOn = false;
bool doorOpen = false;

// Forward Declaration

void handleMessage(JsonObject message);
void updateDoorPosition();

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
    updateDigitalI2CInputs(&buttonInsidePressed, DOOR_BUTTON_INSIDE_PRESSED, &buttonOutsidePressed, DOOR_BUTTON_OUTSIDE_PRESSED);

    updateDigitalI2COutputs(&ledInsideOn, &ledOutsideOn);

    updateDoorPosition();

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
        deviceInfoMessage["DOOR_LED_INSIDE_ON"] = ledInsideOn;
        deviceInfoMessage["DOOR_LED_OUTSIDE_ON"] = ledOutsideOn;
        deviceInfoMessage["DOOR_DOOR_OPEN"] = doorOpen;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICE_INFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case DOOR_LED_INSIDE_ON:
    {
        ledInsideOn = (bool)message["value"];
        break;
    }

    case DOOR_LED_OUTSIDE_ON:
    {
        ledOutsideOn = (bool)message["value"];
        break;
    }

    case DOOR_DOOR_OPEN:
    {
        doorOpen = (bool)message["value"];
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
void updateDoorPosition()
{
    static bool firstTime = true;
    static bool doorOpen_Previous = false;

    if (doorOpen != doorOpen_Previous || firstTime)
    {
        doorOpen_Previous = doorOpen;
        if (doorOpen)
        {
            setServoAngle(DOOR_OPEN_ANGLE);
            Serial.printf("Door opened to %d degrees!\n", DOOR_OPEN_ANGLE);
        }
        else
        {
            setServoAngle(DOOR_CLOSED_ANGLE);
            Serial.printf("Door closed to %d degrees!\n", DOOR_CLOSED_ANGLE);
        }
    }

    if (firstTime)
        firstTime = false;
}
