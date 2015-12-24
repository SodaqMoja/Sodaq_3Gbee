#ifndef _SD_MBILI_XBEE_h
#define _SD_MBILI_XBEE_h

#include "Sodaq_GSM_Modem/Sodaq_Switchable_Device.h"

class SD_Mbili_Xbee : public SwitchableDevice {
public:
    SD_Mbili_Xbee();
protected:
    static void _on();
    static void _off();
};

extern SD_Mbili_Xbee sd_mbili_xbee;

#endif

