// Defines

const char DEVICE_TYPE[] = "Chair";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool input0State = false;
bool output0State = false;
bool output1State = false;
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
    updateDigitalI2CInputs(&input0State, CHAIR_BUTTON_CHANGE);

    updateAnalogI2CInputs(10, 20, 255, &analogInput0Value, CHAIR_FORCESENSOR_CHANGE);

    updateDigitalI2COutputs(&output0State, &output1State);

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
        deviceInfoMessage["vibratorState"] = output1State;
        deviceInfoMessage["pressureValue"] = analogInput0Value;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case CHAIR_LED_CHANGE:
    {
        output0State = (bool)message["value"];
        break;
    }

    case CHAIR_VIBRATOR_CHANGE:
    {
        output1State = (bool)message["value"];
        break;
    }

    default:
        Serial.printf("[Error] Unsupported command received: %d\n", (int)message["command"]);
        break;
    }
}
