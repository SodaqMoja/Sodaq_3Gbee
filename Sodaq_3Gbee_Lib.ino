#include "Sodaq_3Gbee.h"

// MBili
#define debugSerial Serial

// Autonomo
// #define debugSerial SerialUSB

#define modemSerial Serial1

//#define APN "public4.m2minternet.com"

#define APN "aerea.m2m.com"
//#define USERNAME NULL
//#define PASSWORD NULL

void setup()
{
    debugSerial.begin(9600);
    modemSerial.begin(9600);

    debugSerial.println("**Bootup**");

    sodaq_3gbee.setDiag(debugSerial); // optional
    
    // sodaq_3gbee.setSwitchableDevice();
    pinMode(BEEDTR, OUTPUT);
    digitalWrite(BEEDTR, LOW);
    delay(1000);
    digitalWrite(BEEDTR, HIGH);

    delay(100);

    if (sodaq_3gbee.init(modemSerial, NULL, APN)) {
        debugSerial.println("Modem initialization was successful.");



        if (sodaq_3gbee.join()) {
            debugSerial.println("Modem connected to the apn successfully.");
        }
        else {
            debugSerial.println("Modem failed to connect to the apn!");
        }
    } else {
        debugSerial.println("Modem initialization failed!");
    }

}

void loop()
{
    while (debugSerial.available())
    {
        modemSerial.write((char)debugSerial.read());
    }

    while (modemSerial.available())
    {
        debugSerial.write((char)modemSerial.read());
    }
}
