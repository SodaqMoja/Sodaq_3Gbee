/*
 * Copyright (c) 2019 Gabriel Notman.  All rights reserved.
 *
 * This file is part of Sodaq_MQTT.
 *
 * Sodaq_MQTT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or(at your option) any later version.
 *
 * Sodaq_MQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Sodaq_MQTT.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#ifndef SODAQ_MQTT_INTERFACE_H_
#define SODAQ_MQTT_INTERFACE_H_

#include <stdint.h>
#include <stdlib.h>

class Sodaq_MQTT_Interface
{
public:
    virtual bool openMQTT(const char * server, uint16_t port = 1883) = 0;
    virtual bool closeMQTT(bool switchOff=true) = 0;
    virtual bool sendMQTTPacket(uint8_t * pckt, size_t len) = 0;
    virtual size_t receiveMQTTPacket(uint8_t * pckt, size_t size, uint32_t timeout = 20000) = 0;
    virtual size_t availableMQTTPacket() = 0;
    virtual bool isAliveMQTT() = 0;

    virtual void setMQTTClosedHandler(void (*handler)(void)) = 0; 
};

#endif /* SODAQ_MQTT_INTERFACE_H_ */
