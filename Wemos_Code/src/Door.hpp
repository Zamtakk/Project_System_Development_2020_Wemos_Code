// Defines

const char DEVICE_TYPE[] = "Door";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

#define DOOR_OPEN_ANGLE 0
#define DOOR_CLOSED_ANGLE 83

// Global variables

bool input0State = false;
bool input1State = false;
bool output0State = false;
bool output1State = false;

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
    updateDigitalI2CInputs(&input0State, DOOR_BUTTON1_CHANGE, &input1State, DOOR_BUTTON2_CHANGE);

    updateDigitalI2COutputs(&output0State, &output1State);

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
    case DEVICEINFO:
    {
        StaticJsonDocument<200> deviceInfoMessage;
        char stringMessage[200];

        deviceInfoMessage["UUID"] = UUID;
        deviceInfoMessage["Type"] = DEVICE_TYPE;
        deviceInfoMessage["command"] = DEVICEINFO;
        deviceInfoMessage["ledStateInside"] = output0State;
        deviceInfoMessage["ledStateOutside"] = output1State;
        deviceInfoMessage["doorOpen"] = doorOpen;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case DOOR_LED1_CHANGE:
    {
        output0State = (bool)message["value"];
        break;
    }

    case DOOR_LED2_CHANGE:
    {
        output1State = (bool)message["value"];
        break;
    }

    case DOOR_SERVO_CHANGE:
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
