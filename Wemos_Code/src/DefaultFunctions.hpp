// Defines

#define I2C_SDL 5 //D1
#define I2C_SDA 4 //D2

#define DIRECT_OUTPUT_PIN 14 //D5

#define HEARTBEAT_INTERVAL 500

#ifndef NUMBER_OF_LEDS
#define NUMBER_OF_LEDS 1
#endif

// Includes

#include "CommandTypes.hpp"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <math.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Servo.h>
#include <FastLED.h>

// Global variables

WebSocketsClient webSocket;
bool websocketConnected = false;
char UUID[11];
const char *deviceType;
void (*handleWebsocketMessage)(JsonObject);

CRGB fastRGB_LEDs[NUMBER_OF_LEDS];

// Forward Declaration

void initSerial();
void initI2C();
void initFastLED();
void initWifi();
void initWebsocket(void (*messageHandler)(JsonObject), const char *deviceType);

void websocketEvent(WStype_t type, uint8_t *payload, size_t length);

void sendIntMessage(int command, int value);
void sendBoolMessage(int command, bool value);
void sendStringMessage(int command, char *value);

void sendHeartbeat();

void generateUUID();

void checkConnectionI2C();

void readDigitalI2CInputs(bool *input0State, bool *input1State = nullptr);
void updateDigitalI2CInputs(bool *input0State, int input0Command, bool *input1State = nullptr, int input1Command = 0);
void readAnalogI2CInputs(int sampleCount, uint16_t *input0Value, uint16_t *input1Value = nullptr);
void updateAnalogI2CInputs(int sampleCount, int stabilityMargin, uint16_t outputScale, uint16_t *input0Value, int input0Command, uint16_t *input1Value = nullptr, int input1Command = 0);
void updateDigitalI2COutputs(bool *output0State, bool *output1State = nullptr);

void setServoAngle(int angle);

void setFastLedRGBColor(int redValue, int greenValue, int blueValue);

//Function definitions

/*!
    @brief Starts the Serial connection
*/
void initSerial()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n");
}

/*!
    @brief Starts the I2C connection
*/
void initI2C()
{
    Wire.begin();
    checkConnectionI2C();
}

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
void initWebsocket(void (*messageHandler)(JsonObject), const char *p_deviceType)
{
    handleWebsocketMessage = messageHandler;

    deviceType = p_deviceType;

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

        StaticJsonDocument<200> message;
        char stringMessage[200];

        message["UUID"] = UUID;
        message["Type"] = deviceType;
        message["command"] = REGISTRATION;
        message["value"] = "";

        serializeJson(message, stringMessage);

        Serial.printf("[Websocket] Sending registration: %s\n", stringMessage);
        webSocket.sendTXT(stringMessage);

        websocketConnected = true;
        break;
    }
    case WStype_TEXT:
    {
        Serial.printf("[Websocket] New message: %s\n", payload);

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        JsonObject obj = doc.as<JsonObject>();

        handleWebsocketMessage(obj);
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
    message["Type"] = deviceType;
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
    message["Type"] = deviceType;
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
    message["Type"] = deviceType;
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
            message["Type"] = deviceType;
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
    @brief Reads the values from the digital I2C board
    @param[out] input0State Pointer for storing the value of input 0
    @param[out] input1State [OPTIONAL] Pointer for storing the value of input 1
*/
void readDigitalI2CInputs(bool *input0State, bool *input1State)
{
    checkConnectionI2C();

    uint8_t digitalIn;
    bool input0, input1;

    Wire.beginTransmission(0x38);
    Wire.write(byte(0x00));
    Wire.endTransmission();
    Wire.requestFrom(0x38, 1);
    digitalIn = Wire.read();

    input0 = (digitalIn & 0b00000001);
    input1 = (digitalIn & 0b00000010);

    if (input0State != nullptr)
    {
        *input0State = input0;
    }

    if (input1State != nullptr)
    {
        *input1State = input1;
    }
}

/*!
    @brief Updates the values from the digital I2C board and sends them over the websocket connection
    @param[out] input0State Pointer for storing the value of input 0
    @param[in] input0Command The command that is send over the websocket connection together with input 0
    @param[out] input1State [OPTIONAL] Pointer for storing the value of input 1
    @param[in] input1Command [OPTIONAL] The command that is send over the websocket connection together with input 1
*/
void updateDigitalI2CInputs(bool *input0State, int input0Command, bool *input1State, int input1Command)
{
    static bool input0_Previous = false;
    static bool input1_Previous = false;
    static bool firstTime = true;

    if (input1State != nullptr)
    {
        readDigitalI2CInputs(input0State, input1State);
    }
    else
    {
        readDigitalI2CInputs(input0State);
    }

    if (firstTime)
    {
        firstTime = false;
        input0_Previous = *input0State;
        if (input1State != nullptr)
            input1_Previous = *input1State;
        return;
    }

    if (input0State != nullptr)
    {
        if (*input0State != input0_Previous || firstTime)
        {
            Serial.printf("Switch 0 state: %d\n", *input0State);
            input0_Previous = *input0State;
            if (input0Command == 0)
            {
                Serial.printf("[ERROR] Invalid command given for switch 0\n");
                return;
            }
            sendBoolMessage(input0Command, *input0State);
        }
    }

    if (input1State != nullptr)
    {
        if (*input1State != input1_Previous || firstTime)
        {
            Serial.printf("Switch 1 state: %d\n", *input1State);
            input1_Previous = *input1State;
            if (input1Command == 0)
            {
                Serial.printf("[ERROR] Invalid command given for switch 1\n");
                return;
            }
            sendBoolMessage(input1Command, *input1State);
        }
    }
}

/*!
    @brief Reads the values from the analog I2C board
    @param[in] sampleCount The amount of samples that should be taken to get a more stable average
    @param[out] input0Value Pointer for storing the value of input 0
    @param[out] input1Value [OPTIONAL] Pointer for storing the value of input 1
*/
void readAnalogI2CInputs(int sampleCount, uint16_t *input0Value, uint16_t *input1Value)
{
    checkConnectionI2C();

    uint16_t analog0In, analog0Avg = 0, analog1In, analog1Avg = 0;

    for (int i = 0; i < sampleCount; i++)
    {
        Wire.requestFrom(0x36, 4);
        analog0In = (Wire.read() & 0x03) << 8;
        analog0In += Wire.read();
        analog1In = (Wire.read() & 0x03) << 8;
        analog1In += Wire.read();

        analog0Avg += analog0In;
        analog1Avg += analog1In;

        delay(5);
    }

    analog0Avg /= sampleCount;
    analog1Avg /= sampleCount;

    if (input0Value != nullptr)
    {
        *input0Value = analog0Avg;
    }

    if (input1Value != nullptr)
    {
        *input1Value = analog1Avg;
    }
}

/*!
    @brief Updates the values from the analog I2C board and sends them over the websocket connection
    @param[in] sampleCount The amount of samples that should be taken to get a more stable average
    @param[in] stabilityMargin The amount that an input can jump around before a new update is send
    @param[in] outputScale The max value that the input should be rescaled to, ex. 255 for scaling 1023 ADC to 255
    @param[out] input0Value Pointer for storing the value of input 0
    @param[in] input0Command The command that is send over the websocket connection together with input 0
    @param[out] input1Value [OPTIONAL] Pointer for storing the value of input 1
    @param[in] input1Command [OPTIONAL] The command that is send over the websocket connection together with input 1
*/
void updateAnalogI2CInputs(int sampleCount, int stabilityMargin, uint16_t outputScale, uint16_t *input0Value, int input0Command, uint16_t *input1Value, int input1Command)
{
    uint16_t input0, input1;
    static uint16_t input0_Previous = 0;
    static uint16_t input1_Previous = 0;
    static bool firstTime = true;

    if (input1Value != nullptr)
    {
        readAnalogI2CInputs(sampleCount, &input0, &input1);
    }
    else
    {
        readAnalogI2CInputs(sampleCount, &input0);
    }

    if (firstTime)
    {
        firstTime = false;

        input0_Previous = input0;
        *input0Value = input0 / 1023.0 * (float)outputScale;

        if (input1Value != nullptr)
        {
            input1_Previous = input1;
            *input1Value = input1 / 1023.0 * (float)outputScale;
        }
    }

    if (input0Value != nullptr)
    {
        if (abs(input0 - input0_Previous) > stabilityMargin)
        {
            input0_Previous = input0;
            *input0Value = input0 / 1023.0 * (float)outputScale;
            Serial.printf("Analog 0 value: %d\n", *input0Value);
            if (input0Command == 0)
            {
                Serial.printf("[ERROR] Invalid command given for switch 0\n");
                return;
            }
            sendIntMessage(input0Command, *input0Value);
        }
    }

    if (input1Value != nullptr)
    {
        if (abs(input1 - input1_Previous) > stabilityMargin)
        {
            input1_Previous = input1;
            *input1Value = input1 / 1023.0 * (float)outputScale;
            Serial.printf("Analog 1 value: %d\n", *input1Value);
            if (input1Command == 0)
            {
                Serial.printf("[ERROR] Invalid command given for switch 1\n");
                return;
            }
            sendIntMessage(input1Command, *input1Value);
        }
    }
}

/*!
    @brief Reads the values from the digital I2C board
    @param[in] input0State Pointer pointing to the value of output 0
    @param[in] input1State [OPTIONAL] Pointer pointing to the value of output 1
*/
void updateDigitalI2COutputs(bool *output0State, bool *output1State)
{
    checkConnectionI2C();

    uint8_t digitalOut = 0;
    static uint8_t digitalOut_Previous = 255;

    digitalOut += *output0State << 4;

    if (output1State != nullptr)
    {
        digitalOut += *output1State << 5;
    }

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
    @brief Sets the angle of the servo connected to the D5 pin
    @param[in] angle The angle the servo should move to
*/
void setServoAngle(int angle)
{
    static bool initComplete = false;
    static Servo servo;

    if (!initComplete)
    {
        servo.attach(DIRECT_OUTPUT_PIN);
        initComplete = true;
    }

    servo.write(angle);
}

/*!
    @brief Sets the color of all connected FastLED RGB LEDs, NUMBER_OF_LEDS must be defined
    @param[in] redValue The value of the red channel on the RGB LEDs
    @param[in] greenValue The value of the green channel on the RGB LEDs
    @param[in] blueValue The value of the blue channel on the RGB LEDs
*/
void setFastLedRGBColor(int redValue, int greenValue, int blueValue)
{
    static bool initComplete = false;

    if (!initComplete)
    {
        FastLED.addLeds<WS2812, DIRECT_OUTPUT_PIN, GRB>(fastRGB_LEDs, NUMBER_OF_LEDS);
        initComplete = true;
    }

    for (int i = 0; i < NUMBER_OF_LEDS; i++)
    {
        fastRGB_LEDs[i] = CRGB(redValue, greenValue, blueValue);
        FastLED.show();
    }
}
