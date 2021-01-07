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
#define PIN_BUTTON 5 //D1
#define PIN_FORCESENSOR A0
#define PIN_LED 14 //D5
#define PIN_VIBRATOR 12 //D6

#define DEVICE_TYPE "Chair"

// Global variables
ESP8266WiFiMulti wifi;
WebSocketsClient webSocket;

char UUID[11];

bool websocketConnected = false;

int LED_value = 0;
int Vibrator_value = 0;

// Forward Declaration

void initIO();
void initWifi();
void initWebsocket();

void websocketEvent(WStype_t type, uint8_t *payload, size_t length);

void handleMessage(JsonObject message);
void sendIntMessage(int command, int value);
void sendStringMessage(int command, char *value);
void sendBoolMessage(int command, bool value);

void handleButton();
void handleForceSensor();
void handleLED();
void handleVibrator();

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
    webSocket.loop();

    handleButton();

    handleForceSensor();

    handleLED();

    handleVibrator();
}

// Function definitions
/*!
    @brief Starts the Serial connection and initializes all the pins
*/
void initIO()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n");

    pinMode(PIN_BUTTON, INPUT_PULLUP);

    pinMode(PIN_FORCESENSOR, INPUT);

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_VIBRATOR, OUTPUT);
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
    switch ((ChairCommands)message["command"])
    {
    case CHAIR_LED_CHANGE:
        if ((int)message["value"] <= 1 && (int)message["value"] >= 0)
        {
            LED_value = (int)message["value"];
        }
        break;

    case CHAIR_VIBRATOR_CHANGE:
        if ((int)message["value"] <= 1 && (int)message["value"] >= 0)
        {
            Vibrator_value = (int)message["value"];
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
    @brief Reads the push button and sends an update when changed
*/
void handleButton()
{
    int button_State = 1;
    static int button_PreviousState = 1;

    button_State = digitalRead(PIN_BUTTON);

    delay(50); // Simple debounce

    if (button_State != button_PreviousState)
    {
        Serial.printf("Buttons state: %d\n", button_State);
        button_PreviousState = button_State;
        sendBoolMessage(CHAIR_BUTTON_CHANGE, !button_State);
    }
}

/*!
    @brief Reads the force sensor and sends an update if something changed
*/
void handleForceSensor()
{
    int ForceSensor_value = 0;
    static int ForceSensor_previous_value = 0;

    ForceSensor_value = analogRead(PIN_FORCESENSOR);

    if (ForceSensor_value % 8 < 5 && abs(ForceSensor_value - ForceSensor_previous_value) > 4)
    {
        ForceSensor_previous_value = ForceSensor_value;
        Serial.printf("Force sensor value: %d\n", ForceSensor_value);
        sendIntMessage(CHAIR_FORCESENSOR_CHANGE, ForceSensor_value);
    }
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void handleLED()
{
    static int LED_previous_value = 0;

    if (LED_value != LED_previous_value)
    {
        LED_previous_value = LED_value;
        digitalWrite(PIN_LED, LED_value);
        Serial.printf("Led updated to %d!\n", LED_value);
    }
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void handleVibrator()
{
    static int Vibrator_previous_value = 0;

    if (Vibrator_value != Vibrator_previous_value)
    {
        Vibrator_previous_value = Vibrator_value;
        digitalWrite(PIN_VIBRATOR, Vibrator_value);
        Serial.printf("Vibrator updated to %d!\n", Vibrator_value);
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