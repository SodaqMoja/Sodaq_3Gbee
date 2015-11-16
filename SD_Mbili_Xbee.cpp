#include "SD_Mbili_Xbee.h"
#include <Arduino.h>

SD_Mbili_Xbee sd_mbili_xbee;

SD_Mbili_Xbee::SD_Mbili_Xbee()
{
    setSwitchMethods(_on, _off);
    pinMode(BEEDTR, OUTPUT);
}

void SD_Mbili_Xbee::_on()
{
    digitalWrite(BEEDTR, HIGH);
}

void SD_Mbili_Xbee::_off()
{
    digitalWrite(BEEDTR, LOW);
}
