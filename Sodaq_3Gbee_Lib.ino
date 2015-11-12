#include "Sodaq_3Gbee.h"

// MBili
#define debugSerial Serial

// Autonomo
// #define debugSerial SerialUSB

#define modemSerial Serial1

#define APN "public4.m2minternet.com"

#define APN "aerea.m2m.com"
//#define USERNAME NULL
//#define PASSWORD NULL

AuthorizationTypes auth;

void setup()
{
    debugSerial.begin(57600);
    modemSerial.begin(115200);

    sodaq_3gbee.setDiag(debugSerial); // optional
    // sodaq_3gbee.setSwitchableDevice();
    if (sodaq_3gbee.init(modemSerial, NULL, APN)) {
        debugSerial.println("Modem initialization was successful.");
    } else {
        debugSerial.println("Modem initialization failed!");
    }
}

void loop()
{
}
