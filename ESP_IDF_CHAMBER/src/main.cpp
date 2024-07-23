#include <Arduino.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"
#include <EEPROM.h>
#include <ArduinoJson.h>
#define TAG "RS485_ECHO_APP"

#define ECHO_TEST_TXD   (23)
#define ECHO_TEST_RXD   (22)
#define ECHO_TEST_RTS   (18)
#define ECHO_TEST_CTS   (UART_PIN_NO_CHANGE)
#define BUF_SIZE        (127)
#define BAUD_RATE       (115200)
#define PACKET_READ_TICS        (100 / portTICK_PERIOD_MS)
#define ECHO_TASK_STACK_SIZE    (4096)
#define ECHO_TASK_PRIO          (10)
#define ECHO_UART_PORT          (2)
#define ECHO_READ_TOUT          (3)

const char* ssid = "TTIIoT";
const char* password = "TTiVNIoT2023";

const char* mqtt_server = "10.147.37.59";
const int mqtt_port = 1883;
const char* mqtt_user = "hoanganh";
const char* mqtt_pass = "123";
const char* status_topic = "VNTESTLAB/CHAMBER/STATUS";
const char* daily_total_topic = "VNTESTLAB/CHAMBER/TOTAL";
static bool retained_message_processed = false;
char boardID[23];

#define EEPROM_SIZE 512
#define EEPROM_ADDR_TOTAL_WORKING_TIME 0
#define MQTTpubQos 2
WiFiClient espClient;
PubSubClient client(espClient);

static bool machine_running = false;
static bool last_machine_running = false;
static unsigned long start_time = 0;
static unsigned long total_working_time = 0;
static unsigned long last_update_time = 0;
static unsigned long pre_time = 0;
struct tm timeinfo;

int status = 1;
float temp_pv = 0, temp_sp = 0, humi_pv = 0, humi_sp = 0;

void echo_send(const int port, const char* str, uint8_t length) {
    if (uart_write_bytes(port, str, length) != length) {
        ESP_LOGE(TAG, "Send data critical failure.");
        abort();
    }
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    if (String(topic) == "VNTESTLAB/CHAMBER/STATUS" && !retained_message_processed) {
        // Parse the message to retrieve the retained data
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, message);
        pre_time = doc["working_time"].as<unsigned long>() ;
        Serial.printf("Retrieved pre_time from retained message: %lu\n", pre_time);
        retained_message_processed = true;
        client.unsubscribe(status_topic);
        Serial.println("Unsubscribed from status_topic");
    }
}
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(boardID, mqtt_user, mqtt_pass)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}
int a = 0;
int hexCharToDec(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    } else if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    return -1;
}

int hexStringToDec(char *hexStr) {
    int result = 0;
    while (*hexStr) {
        int value = hexCharToDec(*hexStr);
        if (value == -1) {
            return -1; // Error: Invalid hex character
        }
        result = (result << 4) | value;
        hexStr++;
    }
    return result;
}

void processModbusAscii(const uint8_t *hex_array, size_t len, float *temp_pv, float *temp_sp, float *humi_pv, float *humi_sp, int *status) {
    char ascii_str[len + 1];
    for (size_t i = 0; i < len; i++) {
        ascii_str[i] = (char)hex_array[i];
    }
    ascii_str[len] = '\0';

    char temp_pv_hex[5] = {ascii_str[7], ascii_str[8], ascii_str[9], ascii_str[10], '\0'};
    char temp_sp_hex[5] = {ascii_str[11], ascii_str[12], ascii_str[13], ascii_str[14], '\0'};
    char humi_pv_hex[5] = {ascii_str[23], ascii_str[24], ascii_str[25], ascii_str[26], '\0'};
    char humi_sp_hex[5] = {ascii_str[27], ascii_str[28], ascii_str[29], ascii_str[30], '\0'};
    char status_hex[5] = {ascii_str[43], ascii_str[44], ascii_str[45], ascii_str[46], '\0'};

    *temp_pv = hexStringToDec(temp_pv_hex) / 100.0;
    *temp_sp = hexStringToDec(temp_sp_hex) / 100.0;
    *humi_pv = hexStringToDec(humi_pv_hex) / 10.0;
    *humi_sp = hexStringToDec(humi_sp_hex) / 10.0;
    *status = hexStringToDec(status_hex);
}

void update_mqtt() {
    unsigned long current_time = millis() / 1000;
    unsigned long working_time = 0;

    //-------------------------------------------//

    if (machine_running) {
        working_time = current_time - start_time;
    }
    working_time = working_time + pre_time;
    
    //Serial.println(working_time);
    char payload[300];
    snprintf(payload, sizeof(payload), "{\"boardID\":\"%s\",\"temp_pv\":%.2f,\"temp_sp\":%.2f,\"humi_pv\":%.2f,\"humi_sp\":%.2f,\"status\":\"%d\",\"working_time\":\"%lu\"}",
             boardID, temp_pv, temp_sp, humi_pv, humi_sp, machine_running, working_time);
    
    client.publish(status_topic, payload,true);
    
    if (machine_running != last_machine_running){
        a++;
        if (a == 5 && machine_running == false){
           char daily_total_payload[70];
            total_working_time = working_time;
            //snprintf(daily_total_payload, sizeof(daily_total_payload), "{\"boardID\":\"%s\",\"total_working_time\":%lu}", boardID, total_working_time);
            snprintf(payload, sizeof(payload), "{\"boardID\":\"%s\",\"temp_pv\":%.2f,\"temp_sp\":%.2f,\"humi_pv\":%.2f,\"humi_sp\":%.2f,\"status\":\"%d\",\"working_time\":\"%lu\"}",
                boardID, temp_pv, temp_sp, humi_pv, humi_sp, machine_running, working_time);
            //client.publish(daily_total_topic, daily_total_payload);
            client.publish(status_topic, payload, true);
            delay(100); 
            
        }
    }
    last_machine_running = machine_running;
    
    //timeinfo.tm_hour == 23 && timeinfo.tm_min == 59
    if (timeinfo.tm_min == 59) {
        
        char daily_total_payload[70];
        total_working_time = working_time;
        snprintf(daily_total_payload, sizeof(daily_total_payload), "{\"boardID\":\"%s\",\"total_working_time\":%lu}", boardID, total_working_time);
        client.publish(daily_total_topic, daily_total_payload, true);

        snprintf(payload, sizeof(payload), "{\"boardID\":\"%s\",\"temp_pv\":%.2f,\"temp_sp\":%.2f,\"humi_pv\":%.2f,\"humi_sp\":%.2f,\"status\":\"%d\",\"working_time\":\"%lu\"}",
             boardID, temp_pv, temp_sp, humi_pv, humi_sp, machine_running, 0);
    

        client.publish(status_topic, payload, true);
        client.publish(status_topic, payload, true);
        delay(100);
        total_working_time = 0;
        pre_time = 0; // Reset pre_time for the next day
        a = 0;
        //saveTimeToEEPROM(0);
        ESP.restart();
        
    }
    

    last_update_time = current_time;
}

void echo_task(void *arg) {
    const int uart_num = ECHO_UART_PORT;
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_7_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Starting RS485 application and configuring UART.");

    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_LOGI(TAG, "UART pins set, mode configured, and driver installed.");

    ESP_ERROR_CHECK(uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
    ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX));
    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, ECHO_READ_TOUT));
    uint8_t data[BUF_SIZE];

    while (1) {
        const char* command_data = ":01030000000AF2\r\n";
        echo_send(uart_num, command_data, strlen(command_data));
        int len = uart_read_bytes(uart_num, data, BUF_SIZE, PACKET_READ_TICS);
        
        processModbusAscii(data, len, &temp_pv, &temp_sp, &humi_pv, &humi_sp, &status);

        //------Test Value------//
        /* status = 2;
        temp_pv = -40.22;
        temp_sp = -40.00;

        humi_pv = 13.82;
        humi_sp = 13.00; */
        //----------------------//
        
        
        if (status == 4 && !machine_running) {
            machine_running = true;
            start_time = millis() / 1000;
        } else if (status == 1 && machine_running) {
            unsigned long current_time = millis() / 1000;
            total_working_time += current_time - start_time;
            machine_running = false;
        }

        if(!getLocalTime(&timeinfo)){
            Serial.println("Failed to obtain time") ;
            return;
        }
        unsigned long current_time = millis() / 1000;
        if (current_time - last_update_time >= 59) {
            update_mqtt();
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}


void setup() {
    Serial.begin(115200);
    delay(10);
    snprintf(boardID, 23, "%llX", ESP.getEfuseMac());

    //-------------------Wifi-MQTT-EEPROM--------------------------//
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    reconnect();
    client.subscribe("VNTESTLAB/CHAMBER/STATUS"); // Subscribe to the status topic
    delay(10);

    //---------Set up time synchronization-----------//
    configTime(7 * 3600, 0, "CNSDC01.cn.globaltti.net");
    while (!time(nullptr)) {
        Serial.println("Waiting for time sync...");
        delay(2000);
    }
    Serial.println("Time synced successfully!");
    //-----------------------------------------------//

    xTaskCreatePinnedToCore(
        echo_task,          /* Task function. */
        "echo_task",        /* name of task. */
        ECHO_TASK_STACK_SIZE,      /* Stack size of task */
        NULL,               /* parameter of the task */
        ECHO_TASK_PRIO,     /* priority of the task */
        NULL,               /* Task handle to keep track of created task */
        1);                 /* pin task to core 1 */
        
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
}

