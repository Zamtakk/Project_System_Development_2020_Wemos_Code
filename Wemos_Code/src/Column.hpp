// Defines

const char DEVICE_TYPE[] = "Column";

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

bool buttonPressed = false;
bool buzzerOn = false;
bool ledOn = false;
uint16_t smokeValue = 0;

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
    updateDigitalI2CInputs(&buttonPressed, COLUMN_BUTTON_PRESSED);

    updateAnalogI2CInputs(10, 5, 255, &smokeValue, COLUMN_SMOKE_SENSOR_VALUE);

    updateDigitalI2COutputs(&buzzerOn, &ledOn);

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
        deviceInfoMessage["COLUMN_LED_ON"] = ledOn;
        deviceInfoMessage["COLUMN_BUZZER_ON"] = buzzerOn;
        deviceInfoMessage["COLUMN_SMOKE_SENSOR_VALUE"] = smokeValue;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICE_INFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case COLUMN_LED_ON:
    {
        ledOn = (bool)message["value"];
        break;
    }

    case COLUMN_BUZZER_ON:
    {
        buzzerOn = (bool)message["value"];
        break;
    }

    default:
        Serial.printf("[Error] Unsupported command received: %d\n", (int)message["command"]);
        break;
    }
}