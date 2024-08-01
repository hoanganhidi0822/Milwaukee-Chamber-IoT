#include <HardwareSerial.h>
#include "uart.h"
#include <driver/uart.h>
#include <esp_task_wdt.h>
#include "main.h"
#include <Ticker.h>
#include "mqttHelper.h"
#include "OTAHelper.h"
#include "infoHelper.h"

char boardID[23];
Ticker tickerGetData;
Ticker tickerFirmware;
// Ticker tickerFirmware(checkFirmware, 1800003, 0, MILLIS); // 30 minutes
const char *command = ":01030000000AF2\r\n"; // Get D0001 to D0010;
ChamberData chamberData;
bool newVersionChecked           = false;
static bool machine_running      = false;
static bool last_machine_running = false;

void setup()
{
    Serial.begin(115200); // Initialize Serial Monitor for debugging
    uart_setup(UART_NUM_2, 115200, UART_DATA_7_BITS); //HardwareSerial does not support 7bit data in Arduino Framework, config in ESP-IDF Framework 
    Serial.println("UART2 configured for 7 data bits.");
    tickerGetData.attach(3, getData); //Send data every 30 secs
    tickerFirmware.attach(300, checkFirmware); // Checking firmware every 5 minutes
    snprintf(boardID, 23, "%llX", ESP.getEfuseMac()); //Get unique ESP MAC
    //Wifi controls
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    
    startWatchDog(); //Start watch dog, if cannot connect to the wifi, esp will restart after 60 secs
    setup_wifi();
    setup_mqtt();
    checkDeviceExist();
    checkFirmware();
}

void loop()
{
    mqttLoop();
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    printWifiInfo();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Disconnected Event");
}

void startWatchDog()
{
    // WatchDog
    Serial.println("Start WatchDog");
    esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);               // add current thread to WDT watch
}

void stopWatchDog()
{
    Serial.println("Stop WatchDog");
    esp_task_wdt_delete(NULL);
    esp_task_wdt_deinit();
}

void getData()
{
    uint8_t data[MAX_DATA_LENGTH];
    int len = 0;

    uart_write_bytes(UART_NUM_2, command, strlen(command));

    // Read Data via UART
    while (len < MAX_DATA_LENGTH)
    {
        int bytes_read = uart_read_bytes(UART_NUM_2, data + len, MAX_DATA_LENGTH - len, 100 / portTICK_PERIOD_MS);
        if (bytes_read <= 0)
        {
            break; // No more data or timeout
        }
        len += bytes_read;
        if (data[len - 1] == '\n')
        {
            break; // End of message
        }
    }

    if (1)
    {
        Serial.printf("Received %u bytes: [ ", len);
        for (int i = 0; i < len; i++)
        {
            Serial.printf("0x%.2X ", data[i]);
        }
        Serial.println("]");

        pinMode(2,INPUT);

        convertData(data, len, &chamberData);
        sendDataMQTT(chamberData);

        machine_running = digitalRead(2);
        if (machine_running == HIGH){
            machine_running = 1;
        }else{
            machine_running = 0;
        }
        chamberData.nowSTS = machine_running;
        //machine_running = chamberData.nowSTS;
        if (machine_running != last_machine_running) {
        
            sendStatus(chamberData);

            last_machine_running = machine_running;
        }
    }
    else
    {
        chamberData.tempPV = 0.0;
        chamberData.tempSP = 0.0;
        chamberData.wetPV = 0.0;
        chamberData.wetSP = 0.0;
        chamberData.humiPV = 0.0;
        chamberData.humiSP = 0.0;
        chamberData.nowSTS = 0;
        Serial.println("No data received.");
        sendDataMQTT(chamberData);
        uart_wait_tx_done(UART_NUM_2, 10);
    }
}
void printWifiInfo(){
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("Connected Network Signal Strength (RSSI): ");
    Serial.println(WiFi.RSSI()); /*Print WiFi signal strength*/
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void checkFirmware()
{
  if (WiFi.status() == WL_CONNECTED && WiFi.localIP().toString() != "0.0.0.0")
  {
    stopWatchDog();
    OTACheck(true);
    startWatchDog();
  }
}
