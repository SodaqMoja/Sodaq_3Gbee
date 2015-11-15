#include "Sodaq_3Gbee.h"

// MBili
#define debugSerial Serial

// Autonomo
// #define debugSerial SerialUSB

#define modemSerial Serial1

#define APN "public4.m2minternet.com"

//#define APN "aerea.m2m.com"
//#define USERNAME NULL
//#define PASSWORD NULL

#define DNS_TEST

void printIpTuple(uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4)
{
    debugSerial.print(o1);
    debugSerial.print(".");
    debugSerial.print(o2);
    debugSerial.print(".");
    debugSerial.print(o3);
    debugSerial.print(".");
    debugSerial.print(o4);
}

void testDNS()
{
    delay(1000);
    debugSerial.println();

    IP_t ip;
    debugSerial.println("nslookup(\"www.google.com\")");
    ip = sodaq_3gbee.getHostIP("www.google.com");
    printIpTuple(IP_TO_TUPLE(ip));
    debugSerial.println();

    delay(1000);
    debugSerial.println();

    debugSerial.println("nslookup(\"www.sodaq.com\") [== 149.210.181.239]");
    ip = sodaq_3gbee.getHostIP("www.sodaq.com");
    printIpTuple(IP_TO_TUPLE(ip));
    debugSerial.println();

    delay(1000);
    debugSerial.println();

    // this should fail and show 0.0.0.0
    debugSerial.println("nslookup(\"www.odiygsifugvhfdkl.com\") [should fail]");
    ip = sodaq_3gbee.getHostIP("www.odiygsifugvhfdkl.com");
    printIpTuple(IP_TO_TUPLE(ip));
    debugSerial.println();
}

void changeModemBaudrate(uint32_t newBaudrate)
{
    debugSerial.print("Main: changing baudrate to ");
    debugSerial.println(newBaudrate);

    modemSerial.flush();
    modemSerial.end();
    modemSerial.begin(newBaudrate);
}

void setup()
{
    debugSerial.begin(57600);
    modemSerial.begin(9600);

    debugSerial.println("**Bootup**");

    sodaq_3gbee.setDiag(debugSerial); // optional
    sodaq_3gbee.enableBaudrateChange(changeModemBaudrate); // optional
    
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
            debugSerial.println();
            debugSerial.print("Local IP: ");
            IP_t localIp = sodaq_3gbee.getLocalIP();
            printIpTuple(IP_TO_TUPLE(localIp));
            debugSerial.println();
            debugSerial.println();

            testDNS();

            delay(2000);
            if (sodaq_3gbee.disconnect()) {
                debugSerial.println("Modem was successfully disconnected.");
                testDNS();
            }
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
