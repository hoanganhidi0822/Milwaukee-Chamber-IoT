#include "main.h"
#include "hex.h"

void setup_mqtt();
void reconnect();
void setup_wifi();
void mqttLoop();
void setWill();
void sendConnectionAck();
void sendDataMQTT(ChamberData& data);
void sendStatus(ChamberData& data);