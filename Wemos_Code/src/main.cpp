#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(81);

#define USE_SERIAL Serial

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    USE_SERIAL.printf("\nNumber: %u\n", num);
    USE_SERIAL.printf("WStype_t: %u\n", type);
    USE_SERIAL.printf("Payload: (");
    for (uint8_t i = 0; i < length; i++)
    {
        USE_SERIAL.printf("%c", payload[i]);
    }
    USE_SERIAL.printf(")\nLength: %u\n", length);

    switch (type)
    {
    case WStype_DISCONNECTED:
        USE_SERIAL.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
    }
    break;
    case WStype_TEXT:
        USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

        // send message to client
        webSocket.sendTXT(num, "{\"JSON\":\"Shizzle\"}");

        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;
    case WStype_BIN:
        USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
        hexdump(payload, length);

        // send message to client
        // webSocket.sendBIN(num, payload, length);
        break;
    default:
        USE_SERIAL.printf("Uknown event in websocket\n");
        break;
    }

    USE_SERIAL.printf("End of event\n");
}

void setup()
{
    // USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);

    //Serial.setDebugOutput(true);
    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for (uint8_t t = 4; t > 0; t--)
    {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFiMulti.addAP("PJSDV_TEMP", "allhailthemightypi");

    while (WiFiMulti.run() != WL_CONNECTED)
    {
        delay(100);
    }

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop()
{
    webSocket.loop();
}

// #include "ESP8266WiFi.h"

// const char *ssid = "PJSDV_TEMP";
// const char *password = "allhailthemightypi";

// WiFiServer wifiServer(8080);

// void setup()
// {

//     Serial.begin(115200);

//     delay(1000);

//     WiFi.begin(ssid, password);

//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(1000);
//         Serial.println("Connecting..");
//     }

//     Serial.print("Connected to WiFi. IP:");
//     Serial.println(WiFi.localIP());

//     wifiServer.begin();
// }

// void loop()
// {

//     WiFiClient client = wifiServer.available();

//     if (client)
//     {

//         while (client.connected())
//         {

//             while (client.available() > 0)
//             {
//                 char c = client.read();
//                 Serial.write(c);
//                 char reply[] = "What?";
//                 client.write(reply);
//             }

//             delay(10);
//         }

//         client.stop();
//         Serial.println("Client disconnected");
//     }
// }