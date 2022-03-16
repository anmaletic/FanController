#pragma once
/* ----------------------------------------------- General config ----------------------------------------------- */

/* Serial */
#define     SERIAL_BAUD_RATE    115200							// Speed for USB serial console

/* WiFi */
const char* g_ssid = "Radiona";									// Your WiFi SSID
const char* g_password = "123radiona";							// Your WiFi password
#define     WIFI_ENABLE           true							// Enable WIFI

/* MQTT */
#define MQTT_HOST                 IPAddress(192,168,1,3)
#define MQTT_PORT                 1883
#define MQTT_ENABLE               true							// Enable MQTT
const char* mqtt_user = "junkie";								// Your MQTT username		TestNet
const char* mqtt_pass = "1234rm0HASSIOjunkie";					// Your MQTT password		12344321

char g_mqtt_message_buffer[255];								// General purpose buffer for MQTT messages

char MQTT_DEVICE_STATE_TOPIC[255];
char MQTT_DEVICE_SIG_TOPIC[255];
char MQTT_DEVICE_MAC_TOPIC[255];
char MQTT_DEVICE_IP_TOPIC[255];
char MQTT_DEVICE_UPTIME_TOPIC[255];
char MQTT_DEVICE_LOGGER_TOPIC[255];

const char* MQTT_MODE_TOPIC_CMND = "Home/Floor00/Radiona/FanController/set";
const char* MQTT_MODE_TOPIC_STATE = "Home/Floor00/Radiona/FanController/state";

const int topicCount = 1;

const char* topicsToSub[] =
{
	MQTT_MODE_TOPIC_CMND
};


// General
const char* client_id = "FanController";

char g_log_message_buffer[255];								// General purpose buffer for Log messages