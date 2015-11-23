#include "Sodaq_3Gbee.h"
#include "SD_Mbili_Xbee.h"

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
    modemSerial.begin(sodaq_3gbee.getDefaultBaudrate());

    debugSerial.println("**Bootup**");

    sodaq_3gbee.setDiag(debugSerial); // optional
    sodaq_3gbee.enableBaudrateChange(changeModemBaudrate); // optional
    
    sd_mbili_xbee.off();
    sodaq_3gbee.setSwitchableDevice(sd_mbili_xbee);
    delay(500);

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

//            testDNS();

//            uint8_t socket = sodaq_3gbee.createSocket(TCP);
//            if (sodaq_3gbee.connectSocket(socket, "54.175.103.105", 30000)) {
//                debugSerial.println("socket connected");
//
//                sodaq_3gbee.socketSend(socket, "123456789", 9);
//                for (int i = 0; i < 10; i++) {
//                    sodaq_3gbee.isAlive(); // exploit this to allow URC to be read
//                }
//
//                char receiveBuffer[128];
//                size_t bytesRead = sodaq_3gbee.socketReceive(socket, receiveBuffer, sizeof(receiveBuffer));
//
//                debugSerial.println();
//                debugSerial.println();
//                debugSerial.print("Bytes read: ");
//                debugSerial.println(bytesRead);
//
//                for (size_t i = 0; i < bytesRead; i++)
//                {
//                    debugSerial.print(receiveBuffer[i]);
//                }
//
//                debugSerial.println();
//            }

            char operatorBuffer[16];
            if (sodaq_3gbee.getOperatorName(operatorBuffer, sizeof(operatorBuffer))) {
                debugSerial.println(operatorBuffer);
            }

            debugSerial.println(sodaq_3gbee.getNetworkTechnology());

//            delay(2000);
//            if (sodaq_3gbee.disconnect()) {
//                debugSerial.println("Modem was successfully disconnected.");
//                testDNS();
//            }
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
