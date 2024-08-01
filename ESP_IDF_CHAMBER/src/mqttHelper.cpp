
#include <MQTT.h>
#include "mqttHelper.h"
#include <esp_task_wdt.h>
#define MSG_BUFFER_SIZE (50)
// Update these with values suitable for your network.

String ssid = String(APPSSID);
String password = String(APPPASSWORD);
String mqtt_server = String(APPPMQTTSERVER);
String mqtt_user = String(APPPMQTTUSER);
String mqtt_pass = String(APPPMQTTPASSWORD);

WiFiClient espClient;
MQTTClient client(256);
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;
extern char boardID[23];

void setup_wifi()
{

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    printWifiInfo();
}

void reconnect()
{
    startWatchDog();
    Serial.print("Checking Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("Connected!");
    Serial.print("Attempting MQTT connection...");
    // client.setCleanSession(true);
    while (!client.connect(boardID, mqtt_user.c_str(), mqtt_pass.c_str()))
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("Connected!");
    sendConnectionAck();
    client.subscribe(String(APPPMQTTCMDTOPIC) + "/#");
}

void messageReceived(String &topic, String &payload)
{
    Serial.println("Incoming: " + topic + " - " + payload);

    if (topic.equals(String(APPPMQTTCMDTOPIC)) || topic.equals(String(APPPMQTTCMDTOPIC) + "/" + String(boardID)))
    {
        // Check if the message is "RESTART"
        if (payload.equals("RESTART"))
        {
            ESP.restart();
        }
    }
}

void setup_mqtt()
{
    client.begin(mqtt_server.c_str(), 1883, espClient);
    client.onMessage(messageReceived);
    setWill();
}

void mqttLoop()
{
    if (!client.connected())
    {
        reconnect();
    }
    esp_task_wdt_reset();
    client.loop();
}

void setWill()
{
    char dataToSend[72];
    // set will
    snprintf(dataToSend, sizeof(dataToSend), "{\"status\":\"%s\", \"client\": \"%s\", \"appver\": \"%s\"}", "disconnected", boardID, String(APPVERSION));
    client.setWill(String(APPPMQTTSTSTOPIC).c_str(), dataToSend, true, 2);
}

void sendConnectionAck()
{
    char dataToSend[72];
    // send connection ack
    snprintf(dataToSend, sizeof(dataToSend), "{\"status\":\"%s\", \"client\": \"%s\", \"appver\": \"%s\"}", "connected", boardID, String(APPVERSION));
    client.publish(String(APPPMQTTSTSTOPIC), dataToSend, true, 2);
}

void sendDataMQTT(ChamberData& data)
{
    char dataToSend[256];
    snprintf(dataToSend, sizeof(dataToSend), 
             "{\"boardID\":\"%s\",\"tempPV\":%.2f,\"tempSP\":%.2f,\"wetPV\":%.2f,\"wetSP\":%.2f,\"humiPV\":%.2f,\"humiSP\":%.2f}",
             boardID, data.tempPV, data.tempSP, data.wetPV, data.wetSP, data.humiPV, data.humiSP);
    Serial.println(dataToSend);
    client.publish(String(APPPMQTTDATATOPIC), dataToSend);
    //client.publish(cConf.MQTTTopic, dataToSend);
}

void sendStatus(ChamberData& data){

    char dataToSend[256];
    snprintf(dataToSend, sizeof(dataToSend), "{\"nowSTS\":%d}", data.nowSTS);
    Serial.println(dataToSend);
    client.publish(String(APPPMQTTSTATUSTOPIC), dataToSend);
}