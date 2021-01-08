// Include libraries
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <math.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Servo.h>

#include "CommandTypes.hpp"

// Defines
#define I2C_SDL 5 //D1
#define I2C_SDA 4 //D2

#define PIN_SERVO 14 //D5

#define DEVICE_TYPE "Door"

#define INPUT_0 0b00000001
#define INPUT_1 0b00000010

// Global variables
ESP8266WiFiMulti wifi;
WebSocketsClient webSocket;
Servo doorServo;

char UUID[11];

bool websocketConnected = false;

bool input0State = false;
bool input1State = false;
bool output0State = false;
bool output1State = false;
bool doorOpen = false;
int servoValue = 0;

// Forward Declaration

void initIO();
void initWifi();
void initWebsocket();

void checkConnectionI2C();

void websocketEvent(WStype_t type, uint8_t *payload, size_t length);

void handleMessage(JsonObject message);
void sendIntMessage(int command, int value);
void sendStringMessage(int command, char *value);

void handleDigitalInput();
void handleDigitalOutput();
void handleServo();

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
    handleDigitalInput();

    handleDigitalOutput();

    handleServo();

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

    doorServo.attach(PIN_SERVO);
    
    Wire.begin();
    checkConnectionI2C();
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
    @brief Update the I2C connection
*/
void checkConnectionI2C()
{
    //Config PCA9554
    //IO0-IO3 as input, IO4-IO7 as output.
    Wire.beginTransmission(0x38);
    Wire.write(byte(0x03));
    Wire.write(byte(0x0F));
    Wire.endTransmission();

    //Config MAX11647
    Wire.beginTransmission(0x36);
    Wire.write(byte(0xA2));
    Wire.write(byte(0x03));
    Wire.endTransmission();
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
        deviceInfoMessage["ledStateInside"] = output0State;
        deviceInfoMessage["ledStateOutside"] = output1State;
        deviceInfoMessage["doorOpen"] = doorOpen;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case DOOR_LED1_CHANGE:
    {
        output0State = (bool)message["value"];
        break;
    }

    case DOOR_LED2_CHANGE:
    {
        output1State = (bool)message["value"];
        break;
    }

    case DOOR_SERVO_CHANGE:
    {
        doorOpen = (bool)message["value"];
        break;
    }

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
void handleDigitalInput()
{
    checkConnectionI2C();

    uint8_t digitalIn = 0;
    static bool input0State_Previous = false;
    static bool input1State_Previous = false;

    Wire.beginTransmission(0x38);
    Wire.write(byte(0x00));
    Wire.endTransmission();
    Wire.requestFrom(0x38, 1);
    digitalIn = Wire.read();

    input0State = (digitalIn & INPUT_0);
    input1State = (digitalIn & INPUT_1);

    if (input0State != input0State_Previous)
    {
        Serial.printf("Switch state: %d\n", input0State);
        input0State_Previous = input0State;
        sendBoolMessage(DOOR_BUTTON1_CHANGE, input0State);
    }

    if (input1State != input1State_Previous)
    {
        Serial.printf("Switch state: %d\n", input1State);
        input1State_Previous = input1State;
        sendBoolMessage(DOOR_BUTTON2_CHANGE, input1State);
    }
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void handleDigitalOutput()
{
    checkConnectionI2C();

    uint8_t digitalOut = 0;
    static uint8_t digitalOut_Previous = 0;

    digitalOut += output0State << 4;
    digitalOut += output1State << 5;

    if (digitalOut != digitalOut_Previous)
    {
        digitalOut_Previous = digitalOut;

        Wire.beginTransmission(0x38);
        Wire.write(byte(0x01));
        Wire.write(byte(digitalOut));
        Wire.endTransmission();

        Serial.printf("Outputs updated!\n");
    }
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void handleServo()
{
    checkConnectionI2C();

    static bool doorOpen_Previous = true;

    if (doorOpen != doorOpen_Previous)
    {
        doorOpen_Previous = doorOpen;
        if (doorOpen)
        {
            doorServo.write(0);
            Serial.printf("Door opened to 0 degrees!\n");
        }
        else
        {
            doorServo.write(83);
            Serial.printf("Door closed to 83 degrees!\n");
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