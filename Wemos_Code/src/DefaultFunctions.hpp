#ifndef DEFAULT_FUNCTIONS_HPP
#define DEFAULT_FUNCTIONS_HPP

#include "CommandTypes.hpp"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <math.h>
#include <EEPROM.h>

#ifndef DEVICE_TYPE
#define DEVICE_TYPE "Unknown"
#endif

#define HEARTBEAT_INTERVAL 500

WebSocketsClient webSocket;
bool websocketConnected = false;
char UUID[11];

void initWifi();
void initWebsocket();
void websocketEvent(WStype_t type, uint8_t *payload, size_t length);

void handleMessage(JsonObject message);
void sendIntMessage(int command, int value);
void sendStringMessage(int command, char *value);

void sendHeartbeat();

void generateUUID();

/*!
    @brief Starts the WiFi connection
*/
void initWifi()
{
    const char *ssid = "PJSDV_TEMP";
    const char *password = "allhailthemightypi";

    Serial.printf("[WiFi] Connecting to Pi...\n");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
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
    @brief Check if the heartbeat interval time has passed and then send a heartbeat
*/
void sendHeartbeat()
{
    static uint32_t lastTime = 0;

    if ((millis() - lastTime) > HEARTBEAT_INTERVAL)
    {
        lastTime = millis();
        if (websocketConnected)
        {
            StaticJsonDocument<200> message;
            char stringMessage[200];

            message["UUID"] = UUID;
            message["Type"] = DEVICE_TYPE;
            message["command"] = HEARTBEAT;

            serializeJson(message, stringMessage);

            webSocket.sendTXT(stringMessage);
        }
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

#endif