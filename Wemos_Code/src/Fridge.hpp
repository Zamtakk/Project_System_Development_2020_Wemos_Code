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

#include "CommandTypes.hpp"

// Defines
#define I2C_SDL 5 //D1
#define I2C_SDA 4 //D2

#define PIN_TEC 14 //D5

#define DEVICE_TYPE "Fridge"

#define INPUT_0 0b00000001

// Global variables
ESP8266WiFiMulti wifi;
WebSocketsClient webSocket;

char UUID[11];

bool websocketConnected = false;

uint16_t analogInput0Value = 0;
uint16_t analogInput1Value = 0;
bool input0State = false;
bool output0State = false;
bool doorState = false;
bool tecState = false;

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
void handleAnalogInput();
void handleTec();

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

    handleAnalogInput();

    handleTec();

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
        deviceInfoMessage["temperatureValueInside"] = analogInput0Value;
        deviceInfoMessage["temperatureValueOutsie"] = analogInput1Value;
        deviceInfoMessage["doorState"] = input0State;
        deviceInfoMessage["tecState"] = tecState;
        deviceInfoMessage["fanState"] = output0State;

        serializeJson(deviceInfoMessage, stringMessage);

        Serial.printf("Sending DEVICEINFO: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);
        break;
    }

    case FRIDGE_FAN_CHANGE:
    {
        output0State = (bool)message["value"];
        break;
    }

    case FRIDGE_TEC_CHANGE:
    {
        tecState = (bool)message["value"];
        break;
    }

    case FRIDGE_SWITCH_CHANGE:
    {
        input0State = (bool)message["value"];
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

    if (input0State != input0State_Previous)
    {
        Serial.printf("Switch state: %d\n", input0State);
        input0State_Previous = input0State;
        sendBoolMessage(FRIDGE_SWITCH_CHANGE, input0State);
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
    @brief Reads the potmeter and sends an update if something changed
*/
void handleAnalogInput()
{
    checkConnectionI2C();

    uint16_t analog0In = 0;
    uint16_t analog0In_avg = 0;
    static uint16_t analog0In_Previous = 0;
    uint16_t analog1In = 0;
    uint16_t analog1In_avg = 0;
    static uint16_t analog1In_Previous = 0;

    for (int i = 0; i < 5; i++)
    {
        Wire.requestFrom(0x36, 4);
        analog0In = (Wire.read() & 0x03) << 8;
        analog0In += Wire.read();
        analog1In = (Wire.read() & 0x03) << 8;
        analog1In += Wire.read();

        analog0In_avg += analog0In;
        analog1In_avg += analog1In;

        delay(5);
    }

    analog0In_avg /= 5;
    analog1In_avg /= 5;

    if (abs(analog0In_avg - analog0In_Previous) > 8)
    {
        analog0In_Previous = analog0In_avg;
        analogInput0Value = analog0In_avg / 1023.0 * 255.0;
        Serial.printf("Forcesensor value: %d\n", analogInput0Value);
        sendIntMessage(FRIDGE_TEMPERATURESENSORINSIDE_CHANGE, analogInput0Value);
    }
    if (abs(analog1In_avg - analog1In_Previous) > 8)
    {
        analog1In_Previous = analog1In_avg;
        analogInput1Value = analog1In_avg / 1023.0 * 255.0;
        Serial.printf("Forcesensor value: %d\n", analogInput1Value);
        sendIntMessage(FRIDGE_TEMPERATURESENSOROUTSIDE_CHANGE, analogInput1Value);
    }
}

/*!
    @brief Checks if the LED state is still the same as the previous and updates the LED accordingly
*/
void handleTec()
{
    checkConnectionI2C();

    static bool tec_Previous = true;

    if (tecState != tec_Previous)
    {
        tec_Previous = tecState;
        analogWrite(PIN_TEC, tecState);
        Serial.printf("Tec changed to %d!\n", tecState);
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