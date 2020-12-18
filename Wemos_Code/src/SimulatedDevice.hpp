// Include libraries
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <WebSocketsClient.h>

#include <ArduinoJson.h>

#include <Hash.h>

#include "CommandTypes.hpp"

// Defines
#define PIN_BUTTON_1 5 //D1
#define PIN_BUTTON_2 4 //D2
#define PIN_POTMETER A0
#define PIN_LED_1 14 //D5
#define PIN_LED_2 12 //D6
#define PIN_LED_3 13 //D7

#define DEVICE_TYPE "ExampleDevice"

// Global variables
ESP8266WiFiMulti wifi;
WebSocketsClient webSocket;

char UUID[10];

bool websocketConnected = false;

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
    delay(100);
    webSocket.loop();

    handleButtons();
}

// Function declarations
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

void initWebsocket()
{
    // server address, port and URL
    webSocket.begin("10.0.1.1", 9002, "/");

    // event handler
    webSocket.onEvent(websocketEvent);

    // try ever 5000 again if connection has failed
    webSocket.setReconnectInterval(2000);
}

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

void handleMessage(JsonObject message)
{
}

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
        sendIntMessage(BUTTON_1_CHANGE, !button_1_State);
    }

    if (button_2_State != button_2_PreviousState)
    {
        Serial.printf("Buttons 2 state: %d\n", button_2_State);
        button_2_PreviousState = button_2_State;
        sendIntMessage(BUTTON_2_CHANGE, !button_2_State);
    }
}

void handlePotmeter()
{
}

void handleLEDs()
{
}

void generateUUID()
{
    strcpy(UUID, "5794826341");
    // char tempID[50];
    // char buffer[10];
    // int tempIDpos = 0;
    // randomSeed(analogRead(PIN_POTMETER));

    // for(int i = 0; i < 10; i++){
    //     itoa(random(65535), buffer, 10);
    //     for(int i = 0; i < sizeof(buffer); i++){

    //     }
    // }
    // randNumber = random(300);
    // Serial.println(randNumber);
    // delay(50);
}