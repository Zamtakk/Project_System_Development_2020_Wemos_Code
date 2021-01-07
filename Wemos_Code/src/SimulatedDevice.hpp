// Include libraries
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <math.h>
#include <EEPROM.h>

#include "CommandTypes.hpp"

// Defines
#define PIN_BUTTON_1 5 //D1
#define PIN_BUTTON_2 4 //D2
#define PIN_POTMETER A0
#define PIN_LED_1 14 //D5
#define PIN_LED_2 12 //D6
#define PIN_LED_3 13 //D7

#define DEVICE_TYPE "SimulatedDevice"

// Global variables
ESP8266WiFiMulti wifi;
WebSocketsClient webSocket;

char UUID[11];

bool websocketConnected = false;

int LED_1_value = 0;
int LED_2_value = 0;
int LED_3_value = 0;

// Forward Declaration

void initIO();
void initWifi();
void initWebsocket();

void websocketEvent(WStype_t type, uint8_t *payload, size_t length);

void handleMessage(JsonObject message);
void sendIntMessage(int command, int value);
void sendStringMessage(int command, char *value);

void handleButtons();
void handlePotmeter();
void handleLEDs();

void generateUUID();

// Setup
void setup()
{
    initIO();

    generateUUID();

    initWifi();

    initWebsocket();
}

void loop()
{
    handleButtons();

    handlePotmeter();

    handleLEDs();

    webSocket.loop();
}

// Function definitions
/*!
    @brief Starts the Serial connection and initializes all the pins
*/
void initIO()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n");

    pinMode(PIN_BUTTON_1, INPUT_PULLUP);
    pinMode(PIN_BUTTON_2, INPUT_PULLUP);

    pinMode(PIN_POTMETER, INPUT);

    pinMode(PIN_LED_1, OUTPUT);
    pinMode(PIN_LED_2, OUTPUT);
    pinMode(PIN_LED_3, OUTPUT);
}

/*!
    @brief Starts the WiFi connection
*/
void initWifi()
{
    wifi.addAP("PJSDV_TEMP", "allhailthemightypi");

    Serial.printf("[WiFi] Connecting to Pi...\n");
    while (wifi.run() != WL_CONNECTED)
    {
        delay(100);
    }

    Serial.print("[WiFi] Connected, IP address: ");
    Serial.println(WiFi.localIP());
}

/*!
    @brief Starts the Websocket Client and connects with the server.
*/
void initWebsocket()
{
    // server address, port and URL
    webSocket.begin("10.0.1.1", 9002, "/");

    // event handler
    webSocket.onEvent(websocketEvent);

    // try ever 5000 again if connection has failed
    webSocket.setReconnectInterval(2000);
}

/*!
    @brief Interrupt that is called when a new message is received by the websocket client
    @param[in] type Event type that is invoked, ex. CONNECTED, DISCONNECTED or new TEXT
    @param[in] payload A pointer to the payload of the message, in this case a JSON message
    @param[in] length Length of the message that the pointer is pointing to.
*/
void websocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        if (websocketConnected)
        {
            Serial.printf("[Websocket] Disconnected!\n");
            websocketConnected = false;
        }
        break;
    case WStype_CONNECTED:
    {
        Serial.printf("[Websocket] Connected!\n");
        websocketConnected = true;

        StaticJsonDocument<200> message;
        char stringMessage[200];

        message["UUID"] = UUID;
        message["Type"] = DEVICE_TYPE;
        message["command"] = REGISTRATION;
        message["value"] = "";

        serializeJson(message, stringMessage);

        Serial.printf("[Websocket] Sending registration: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }
    case WStype_TEXT:
    {
        Serial.printf("[Websocket] New message: %s\n", payload);

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        JsonObject obj = doc.as<JsonObject>();

        handleMessage(obj);
        break;
    }
    default:
        break;
    }
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
    @brief Sends a new message over the Websocket connection with an integer as JSON value
    @param[in] command The command send in the JSON packet, see CommandTypes.hpp
    @param[in] value An integer value that is send with the command as parameter.
*/
void sendIntMessage(int command, int value)
{
    StaticJsonDocument<200> message;
    char stringMessage[200];

    message["UUID"] = UUID;
    message["Type"] = DEVICE_TYPE;
    message["command"] = command;
    message["value"] = value;

    serializeJson(message, stringMessage);

    Serial.printf("Sending message: %s\n", stringMessage);
    webSocket.sendTXT(stringMessage);
}

/*!
    @brief Sends a new message over the Websocket connection with an bool as JSON value
    @param[in] command The command send in the JSON packet, see CommandTypes.hpp
    @param[in] value A boolean value that is send with the command as parameter.
*/
void sendBoolMessage(int command, bool value)
{
    StaticJsonDocument<200> message;
    char stringMessage[200];

    message["UUID"] = UUID;
    message["Type"] = DEVICE_TYPE;
    message["command"] = command;
    message["value"] = value;

    serializeJson(message, stringMessage);

    Serial.printf("Sending message: %s\n", stringMessage);
    webSocket.sendTXT(stringMessage);
}

/*!
    @brief Sends a new message over the Websocket connection with an string as JSON value
    @param[in] command The command send in the JSON packet, see CommandTypes.hpp
    @param[in] value A string value that is send with the command as parameter.
*/
void sendStringMessage(int command, char *value)
{
    StaticJsonDocument<200> message;
    char stringMessage[200];

    message["UUID"] = UUID;
    message["Type"] = DEVICE_TYPE;
    message["command"] = command;
    message["value"] = value;

    serializeJson(message, stringMessage);

    Serial.printf("Sending message: %s\n", stringMessage);
    webSocket.sendTXT(stringMessage);
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

    if (abs(Potmeter_value - Potmeter_previous_value) > 8)
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

/*!
    @brief Read the UUID from EEPROM or generates a new one if none is present
*/
void generateUUID()
{
    uint addr = 0;

    struct
    {
        uint8_t code[3];
        char uuid[11] = "";
    } data;

    EEPROM.begin(512);
    EEPROM.get(addr, data);

    if (data.code[0] == 34 && data.code[1] == 42 && data.code[2] == 16)
    {
        strcpy(UUID, data.uuid);
        Serial.printf("[SETUP] UUID loaded! ID is: %s\n", UUID);
        return;
    }
    else
    {
        randomSeed(ESP.getCycleCount());

        char buffer[1];
        for (int i = 0; i < 10; i++)
        {
            itoa(random(9), buffer, 10);
            UUID[i] = buffer[0];
        }
        UUID[10] = '\0';
        Serial.printf("[SETUP] New UUID generated! New ID is: %s\n", UUID);

        data.code[0] = 34;
        data.code[1] = 42;
        data.code[2] = 16;
        strcpy(data.uuid, UUID);
    }

    EEPROM.put(addr, data);
    EEPROM.commit();
}