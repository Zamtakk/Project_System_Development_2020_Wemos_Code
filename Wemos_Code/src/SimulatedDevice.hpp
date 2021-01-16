// Defines

const char DEVICE_TYPE[] = "SimulatedDevice";

#define PIN_BUTTON_1 5 //D1
#define PIN_BUTTON_2 4 //D2
#define PIN_POTMETER A0
#define PIN_LED_1 14 //D5
#define PIN_LED_2 12 //D6
#define PIN_LED_3 13 //D7

//Includes

#include "CommandTypes.hpp"
#include "DefaultFunctions.hpp"

// Global variables

int LED_1_value = 0;
int LED_2_value = 0;
int LED_3_value = 0;

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

    pinMode(PIN_POTMETER, INPUT);

    pinMode(PIN_LED_1, OUTPUT);
    pinMode(PIN_LED_2, OUTPUT);
    pinMode(PIN_LED_3, OUTPUT);
}

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
        deviceInfoMessage["led1Value"] = LED_1_value;
        deviceInfoMessage["led2Value"] = LED_2_value;
        deviceInfoMessage["led3Value"] = LED_3_value;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }
    case SIMULATED_LED1_CHANGE:
        if ((int)message["value"] <= 255 && (int)message["value"] >= 0)
        {
            LED_1_value = (int)message["value"];
        }
        break;

    case SIMULATED_LED2_CHANGE:
        if ((int)message["value"] <= 255 && (int)message["value"] >= 0)
        {
            LED_2_value = (int)message["value"];
        }
        break;

    case SIMULATED_LED3_CHANGE:
        if ((int)message["value"] <= 255 && (int)message["value"] >= 0)
        {
            LED_3_value = (int)message["value"];
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
    int button_1_State = 1;
    static int button_1_PreviousState = 1;
    int button_2_State = 1;
    static int button_2_PreviousState = 1;

    button_1_State = digitalRead(PIN_BUTTON_1);
    button_2_State = digitalRead(PIN_BUTTON_2);

    delay(50); // Simple debounce

    if (button_1_State != button_1_PreviousState)
    {
        Serial.printf("Buttons 1 state: %d\n", button_1_State);
        button_1_PreviousState = button_1_State;
        sendBoolMessage(SIMULATED_BUTTON1_CHANGE, !button_1_State);
    }

    if (button_2_State != button_2_PreviousState)
    {
        Serial.printf("Buttons 2 state: %d\n", button_2_State);
        button_2_PreviousState = button_2_State;
        sendBoolMessage(SIMULATED_BUTTON2_CHANGE, !button_2_State);
    }
}

/*!
    @brief Reads the potmeter and sends an update if something changed
*/
void handlePotmeter()
{
    int Potmeter_value = 0;
    static int Potmeter_previous_value = 0;

    for (int i = 0; i < 10; i++)
    {
        Potmeter_value += analogRead(PIN_POTMETER);
    }
    Potmeter_value /= 10;

    if (abs(Potmeter_value - Potmeter_previous_value) > 20)
    {
        Potmeter_previous_value = Potmeter_value;
        Potmeter_value = Potmeter_value / 1023.0 * 255.0;
        Serial.printf("Potmeter value: %d\n", Potmeter_value);
        sendIntMessage(SIMULATED_POTMETER_CHANGE, Potmeter_value);
    }
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void handleLEDs()
{
    static int LED_1_previous_value = 0;
    static int LED_2_previous_value = 0;
    static int LED_3_previous_value = 0;

    if (LED_1_value != LED_1_previous_value)
    {
        LED_1_previous_value = LED_1_value;
        analogWrite(PIN_LED_1, LED_1_value);
        Serial.printf("Led 1 updated to %d!\n", LED_1_value);
    }

    if (LED_2_value != LED_2_previous_value)
    {
        LED_2_previous_value = LED_2_value;
        analogWrite(PIN_LED_2, LED_2_value);
        Serial.printf("Led 2 updated to %d!\n", LED_2_value);
    }

    if (LED_3_value != LED_3_previous_value)
    {
        LED_3_previous_value = LED_3_value;
        analogWrite(PIN_LED_3, LED_3_value);
        Serial.printf("Led 3 updated to %d!\n", LED_3_value);
    }
}