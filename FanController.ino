/*
    Name:       FanController.ino
    Created:	12.03.2022. 11:13:48
    Author:     ANMAL-LAP\Junkie
*/

/**************
 *  +---------------+-------------------------+-------------------------+
 *  | ESP8266       | D1(IO5),    D2(IO4),    | D0(IO16),   D1(IO5),    |
 *  |               | D5(IO14),   D6(IO12),   | D2(IO4),    D5(IO14),   |
 *  |               | D7(IO13),   D8(IO15),   | D6(IO12),   D7(IO13),   |
 *  |               |                         | D8(IO15)                |
 *  +---------------+-------------------------+-------------------------+
**************/

#include "config.h"
#include "async_wifi.h"
#include "DHT.h"
#include <RBDdimmer.h>

#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define outputPin  12
#define zerocross  13
dimmerLamp Fan(outputPin, zerocross); //initialase port for dimmer for ESP8266, ESP32, Arduino due boards


class FanModel
{
private:
    int _speed;
    int _boostStartTime;
    int _boostDuration = 2000;
    bool _boostNeeded = true;
    bool _boostRunning = false;

public:
    int Mode;        //  1 = Automatic, 0 = Manual

    int State;

    int TempCurrent;
    int TempLast;
    int TempTarget;
    int TempCycle;

    int SpeedMin;
    int SpeedMax;
    int Speed;

    void IncreaseSpeed()
    {
        if (Speed + 5 > SpeedMax)
        {
            Speed = SpeedMax;
        }
        else
        {
            Speed += 5;
        }
    }

    void DecreaseSpeed()
    {
        if (Speed - 5 < SpeedMin)
        {
            Speed = SpeedMin;
        }
        else
        {
            Speed -= 5;
        }
    }

    void SetSpeed()
    {
        Fan.setPower(Speed);
    }

    void SetState()
    {
        if (State == 1)
        {
            if (_boostNeeded && !_boostRunning)
            {
                _boostStartTime = millis();
                _speed = 75;

                _boostNeeded = false;
                _boostRunning = true;
            }
            else if (_boostRunning)
            {
                if (millis() - _boostStartTime >= _boostDuration)
                {
                    _speed = SpeedMin;
                    _boostRunning = false;
                }
            }
        }
        else
        {
            _speed = 0;

            _boostNeeded = true;
            _boostRunning = false;
        }
    }
};

FanModel FanControl;

class LoopCycleModel
{
public:
    bool FirstRun = true;
    int Current = 0;
    int Last = 0;
    int Duration = 300000;      //  5min = 300000ms
};

LoopCycleModel LoopCycle;

void InitializeFanControl()
{
    FanControl.Mode = 1;
    FanControl.State = 1;
    FanControl.SpeedMin = 30;
    FanControl.SpeedMax = 100;

    FanControl.TempCycle = 0;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting Fan Controller");

    dht.begin();

    Fan.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 

    SetupMQTT();
    mqttClient.onMessage(onMqttMessage);
    SetupWiFi();

    InitializeFanControl();
}

void loop()
{
    AsyncElegantOTA.loop();
    mqtt_loop();

    FanControl.SetState();

    LoopCycle.Current = millis();

    if (LoopCycle.Current - LoopCycle.Last >= LoopCycle.Duration || LoopCycle.FirstRun)
    {
        if (LoopCycle.FirstRun)
        {
            LoopCycle.FirstRun = false;
        }

        LoopCycle.Last = LoopCycle.Current;

        FanControl.TempCurrent = int(dht.readTemperature()) + 1;

        PublishData();

        if (FanControl.State == 1)
        {
            if (FanControl.Mode == 1)
            {
                TempRegulationAutomatic();
            }
        }
    }

    FanControl.SetSpeed();
}

void TempRegulationAutomatic()
{
    if (FanControl.TempCurrent > FanControl.TempTarget)
    {
        if (FanControl.TempCurrent > FanControl.TempLast)
        {
            FanControl.TempLast = FanControl.TempCurrent;
     
            FanControl.IncreaseSpeed();            
        }
        else if (FanControl.TempCurrent == FanControl.TempLast)
        {
            if (FanControl.TempCycle == 2)
            {
                FanControl.TempLast = FanControl.TempCurrent;

                FanControl.IncreaseSpeed();
            }
            else
            {
                FanControl.TempCycle++;
            }
        }
    }
    else if (FanControl.TempCurrent < FanControl.TempTarget)
    {
        if (FanControl.TempCurrent < FanControl.TempLast)
        {
            FanControl.TempLast = FanControl.TempCurrent;

            FanControl.DecreaseSpeed();
        }
        else if (FanControl.TempCurrent == FanControl.TempLast)
        {
            if (FanControl.TempCycle == 2)
            {
                FanControl.TempLast = FanControl.TempCurrent;

                FanControl.DecreaseSpeed();
            }
            else
            {
                FanControl.TempCycle++;
            }
        }
    }
}

void PublishData()
{
    DynamicJsonDocument JsonMsg(1024);

    JsonMsg["Mode"] = FanControl.Mode;
    JsonMsg["TempTarget"] = FanControl.TempTarget;
    JsonMsg["TempCurrent"] = FanControl.TempCurrent;
    JsonMsg["FanState"] = FanControl.State;
    JsonMsg["FanSpeed"] = FanControl.Speed;

    String message;
    serializeJson(JsonMsg, message);
    mqttClient.publish(MQTT_MODE_TOPIC_STATE, 0, true, message.c_str());
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    Serial.println("Publish received.");
    Serial.print("  topic: ");
    Serial.println(topic);
 
    sprintf(g_log_message_buffer, "Received MQTT Message!");
    Log();

    DynamicJsonDocument JSON_Msg(1024);
    DeserializationError error = deserializeJson(JSON_Msg, payload);
    if (error)
        return;

    FanControl.Mode = JSON_Msg["Mode"];
    FanControl.TempTarget = JSON_Msg["TempTarget"];
    FanControl.State = JSON_Msg["FanState"];
    FanControl.Speed = JSON_Msg["FanSpeed"]; 

    PublishData();
}
