// Defines

const char DEVICE_TYPE[] = "Chair";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool buttonPressed = false;
bool ledOn = false;
bool vibratorOn = false;
uint16_t pressureValue = 0;

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
    updateDigitalI2CInputs(&buttonPressed, CHAIR_BUTTON_PRESSED);

    updateAnalogI2CInputs(10, 20, 255, &pressureValue, CHAIR_PRESSURE_SENSOR_VALUE);

    updateDigitalI2COutputs(&ledOn, &vibratorOn);

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
        deviceInfoMessage["CHAIR_LED_ON"] = ledOn;
        deviceInfoMessage["CHAIR_VIBRATOR_ON"] = vibratorOn;
        deviceInfoMessage["CHAIR_PRESSURE_SENSOR_VALUE"] = pressureValue;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICE_INFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case CHAIR_LED_ON:
    {
        ledOn = (bool)message["value"];
        break;
    }

    case CHAIR_VIBRATOR_ON:
    {
        vibratorOn = (bool)message["value"];
        break;
    }

    default:
        Serial.printf("[Error] Unsupported command received: %d\n", (int)message["command"]);
        break;
    }
}
