// Defines

const char DEVICE_TYPE[] = "SimulatedDevice";

#define PIN_BUTTON_1 5 //D1
#define PIN_BUTTON_2 4 //D2
#define PIN_DIMMER A0
#define PIN_LED1 14 //D5
#define PIN_LED2 12 //D6
#define PIN_LED3 13 //D7

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

uint8_t LED1_Value = 0;
uint8_t LED2_Value = 0;
uint8_t LED3_Value = 0;

// Forward Declaration

void initPins();

void handleMessage(JsonObject message);
void handleButtons();
void handlePotmeter();
void handleLEDs();

// Setup

void setup()
{
    initSerial();

    initPins();

    generateUUID();

    initWifi();

    initWebsocket(&handleMessage, DEVICE_TYPE);
}

// Loop

void loop()
{
    handleButtons();

    handlePotmeter();

    handleLEDs();

    sendHeartbeat();

    webSocket.loop();
}

// Function definitions

/*!
    @brief Starts the Serial connection and initializes all the pins
*/
void initPins()
{
    pinMode(PIN_BUTTON_1, INPUT_PULLUP);
    pinMode(PIN_BUTTON_2, INPUT_PULLUP);

    pinMode(PIN_DIMMER, INPUT);

    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    pinMode(PIN_LED3, OUTPUT);
}

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
        deviceInfoMessage["SIMULATED_LED1_VALUE"] = LED1_Value;
        deviceInfoMessage["SIMULATED_LED2_VALUE"] = LED2_Value;
        deviceInfoMessage["SIMULATED_LED3_VALUE"] = LED3_Value;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICE_INFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }
    case SIMULATED_LED1_VALUE:
        if ((int)message["value"] <= 255 && (int)message["value"] >= 0)
        {
            LED1_Value = (int)message["value"];
        }
        break;

    case SIMULATED_LED2_VALUE:
        if ((int)message["value"] <= 255 && (int)message["value"] >= 0)
        {
            LED2_Value = (int)message["value"];
        }
        break;

    case SIMULATED_LED3_VALUE:
        if ((int)message["value"] <= 255 && (int)message["value"] >= 0)
        {
            LED3_Value = (int)message["value"];
        }
        break;

    default:
        Serial.printf("[Error] Unsupported command received: %d\n", (int)message["command"]);
        break;
    }
}

/*!
    @brief Reads all the buttons and sends an update if something changed
*/
void handleButtons()
{
    static bool firstTime = true;
    int button_1_State = 1;
    static int button_1_PreviousState = 1;
    int button_2_State = 1;
    static int button_2_PreviousState = 1;

    button_1_State = digitalRead(PIN_BUTTON_1);
    button_2_State = digitalRead(PIN_BUTTON_2);

    delay(50); // Simple debounce

    if (button_1_State != button_1_PreviousState || firstTime)
    {
        Serial.printf("Buttons 1 state: %d\n", button_1_State);
        button_1_PreviousState = button_1_State;
        sendBoolMessage(SIMULATED_BUTTON1_PRESSED, !button_1_State);
    }

    if (button_2_State != button_2_PreviousState || firstTime)
    {
        Serial.printf("Buttons 2 state: %d\n", button_2_State);
        button_2_PreviousState = button_2_State;
        sendBoolMessage(SIMULATED_BUTTON2_PRESSED, !button_2_State);
    }

    if (firstTime)
        firstTime = false;
}

/*!
    @brief Reads the potmeter and sends an update if something changed
*/
void handlePotmeter()
{
    static bool firstTime = true;
    int dimmerValue = 0;
    static int dimmerValue_Previous = 0;

    for (int i = 0; i < 10; i++)
    {
        dimmerValue += analogRead(PIN_DIMMER);
    }
    dimmerValue /= 10;

    if (abs(dimmerValue - dimmerValue_Previous) > 20 || firstTime)
    {
        dimmerValue_Previous = dimmerValue;
        dimmerValue = dimmerValue / 1023.0 * 255.0;
        Serial.printf("Potmeter value: %d\n", dimmerValue);
        sendIntMessage(SIMULATED_DIMMER_VALUE, dimmerValue);
    }

    if (firstTime)
        firstTime = false;
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void handleLEDs()
{
    static bool firstTime = true;
    static uint8_t LED1_Value_Previous = 0;
    static uint8_t LED2_Value_Previous = 0;
    static uint8_t LED3_Value_Previous = 0;

    if (LED1_Value != LED1_Value_Previous || firstTime)
    {
        LED1_Value_Previous = LED1_Value;
        analogWrite(PIN_LED1, LED1_Value);
        Serial.printf("Led 1 updated to %d!\n", LED1_Value);
    }

    if (LED2_Value != LED2_Value_Previous || firstTime)
    {
        LED2_Value_Previous = LED2_Value;
        analogWrite(PIN_LED2, LED2_Value);
        Serial.printf("Led 2 updated to %d!\n", LED2_Value);
    }

    if (LED3_Value != LED3_Value_Previous || firstTime)
    {
        LED3_Value_Previous = LED3_Value;
        analogWrite(PIN_LED3, LED3_Value);
        Serial.printf("Led 3 updated to %d!\n", LED3_Value);
    }

    if (firstTime)
        firstTime = false;
}