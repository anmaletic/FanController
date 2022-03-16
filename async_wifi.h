#pragma once
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Ticker.h>

#include "async_mqtt.h"

AsyncWebServer server(80);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;


void connectToWifi()
{
    Serial.println("Connecting to Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.hostname(client_id);
    WiFi.begin(g_ssid, g_password);
}

void startOTAServer()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Hi! I am ESP8266.");
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");
}

void onWifiConnect(const WiFiEventStationModeGotIP& event)
{
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(g_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    startOTAServer();

    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event)
{
    Serial.println("Disconnected from Wi-Fi.");
    mqttReconnectTimer.detach();    // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    wifiReconnectTimer.once(2, connectToWifi);
}

void SetupWiFi()
{
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    connectToWifi();
}