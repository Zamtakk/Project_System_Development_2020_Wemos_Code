#ifndef DEFAULT_FUNCTIONS_HPP
#define DEFAULT_FUNCTIONS_HPP

#include <ESP8266WiFi.h>

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


#endif