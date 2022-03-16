#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include "uptime.h"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

// Uptime update
bool mqtt_connected;
int updateInterval = 10000;
int updateMillis = 0;

void Log()
{
	char p_uptime[255];
	uptime::calculateUptime();
	sprintf(p_uptime, "%i:%i:%i:%i", uptime::getDays(), uptime::getHours(), uptime::getMinutes(), uptime::getSeconds());


	DynamicJsonDocument LogMessage(1024);

	LogMessage["ControllerID"] = client_id;
	LogMessage["Uptime"] = p_uptime;
	LogMessage["Message"] = g_log_message_buffer;

	String message;
	serializeJson(LogMessage, message);

	Serial.println(message);
	mqttClient.publish(MQTT_DEVICE_LOGGER_TOPIC, 2, true, message.c_str());
}

void connectToMqtt()
{
	Serial.println("Connecting to MQTT...");
	mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
	Serial.println("Connected to MQTT.");
	Serial.print("Session present: ");
	Serial.println(sessionPresent);

	mqtt_connected = true;

	mqttClient.publish(MQTT_DEVICE_STATE_TOPIC, 0, true, "Connected");

	int RssI = WiFi.RSSI();
	RssI = isnan(RssI) ? -100 : RssI;
	RssI = min(max(2 * (RssI + 100), 0), 100);

	mqttClient.publish(MQTT_DEVICE_SIG_TOPIC, 0, true, String(RssI).c_str());
	mqttClient.publish(MQTT_DEVICE_MAC_TOPIC, 0, true, WiFi.macAddress().c_str());
	mqttClient.publish(MQTT_DEVICE_IP_TOPIC, 0, true, WiFi.localIP().toString().c_str());

	uptime::calculateUptime();
	sprintf(g_mqtt_message_buffer, "%i:%i:%i:%i", uptime::getDays(), uptime::getHours(), uptime::getMinutes(), uptime::getSeconds());
	mqttClient.publish(MQTT_DEVICE_UPTIME_TOPIC, 0, true, g_mqtt_message_buffer);

	sprintf(g_log_message_buffer, "Device connected");
	Log();

	for (int i = 0; i < topicCount; i++)
	{
		mqttClient.subscribe(topicsToSub[i], 0);
	}
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
	Serial.println("Disconnected from MQTT:");
	mqtt_connected = false;

	if (WiFi.isConnected())
	{
		mqttReconnectTimer.once(1, connectToMqtt);
	}
}

void mqtt_loop()
{
	if (mqtt_connected)
	{
		if (millis() - updateMillis >= updateInterval)
		{
			updateMillis = millis();

			int RssI = WiFi.RSSI();
			RssI = isnan(RssI) ? -100 : RssI;
			RssI = min(max(2 * (RssI + 100), 0), 100);
			mqttClient.publish(MQTT_DEVICE_SIG_TOPIC, 0, true, String(RssI).c_str());

			uptime::calculateUptime();
			sprintf(g_mqtt_message_buffer, "%i:%i:%i:%i", uptime::getDays(), uptime::getHours(), uptime::getMinutes(), uptime::getSeconds());
			mqttClient.publish(MQTT_DEVICE_UPTIME_TOPIC, 0, true, g_mqtt_message_buffer);
		}
	}
}

void SetupTopics()
{
	sprintf(MQTT_DEVICE_STATE_TOPIC, "Status/%s/State", client_id);
	sprintf(MQTT_DEVICE_SIG_TOPIC, "Status/%s/Signal", client_id);
	sprintf(MQTT_DEVICE_MAC_TOPIC, "Status/%s/MAC", client_id);
	sprintf(MQTT_DEVICE_IP_TOPIC, "Status/%s/IP", client_id);
	sprintf(MQTT_DEVICE_UPTIME_TOPIC, "Status/%s/Uptime", client_id);
	sprintf(MQTT_DEVICE_LOGGER_TOPIC, "Status/%s/Logger", client_id);
}

void SetupMQTT()
{
	mqttClient.onConnect(onMqttConnect);
	mqttClient.onDisconnect(onMqttDisconnect);
	mqttClient.setCredentials(mqtt_user, mqtt_pass);
	mqttClient.setServer(MQTT_HOST, MQTT_PORT);

	SetupTopics();
}

