#include "Sodaq_3Gbee.h"
#include "SD_Mbili_Xbee.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// MBili
#define debugSerial Serial

// Autonomo
// #define debugSerial SerialUSB

#define modemSerial Serial1

#define APN "public4.m2minternet.com"

//#define APN "aerea.m2m.com"
//#define USERNAME NULL
//#define PASSWORD NULL

//#define TEST_DNS
//#define TEST_SOCKETS
//#define TEST_NETWORK_INFO
//#define TEST_SIM_DEVICE_INFO
//#define TEST_FILESYSTEM
//#define TEST_HTTP
#define TEST_FTP
#define TEST_SMS

void printToLen(const char* buffer, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        debugSerial.print(buffer[i]);
    }

    debugSerial.println();
}

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

            delay(1000);
            debugSerial.println();

#ifdef TEST_DNS
            {
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
#endif

#ifdef TEST_SOCKETS
            {
                uint8_t socket = sodaq_3gbee.createSocket(TCP);
                if (sodaq_3gbee.connectSocket(socket, "54.175.103.105", 30000)) {
                    debugSerial.println("socket connected");

                    sodaq_3gbee.socketSend(socket, "123456789", 9);
                    for (int i = 0; i < 10; i++) {
                        sodaq_3gbee.isAlive(); // exploit this to allow URC to be read
                    }

                    char receiveBuffer[128];
                    size_t bytesRead = sodaq_3gbee.socketReceive(socket, receiveBuffer, sizeof(receiveBuffer));

                    debugSerial.println();
                    debugSerial.println();
                    debugSerial.print("Bytes read: ");
                    debugSerial.println(bytesRead);

                    for (size_t i = 0; i < bytesRead; i++)
                    {
                        debugSerial.print(receiveBuffer[i]);
                    }

                    debugSerial.println();
                }
            }
#endif

#ifdef TEST_NETWORK_INFO
            {
                char operatorBuffer[16];
                if (sodaq_3gbee.getOperatorName(operatorBuffer, sizeof(operatorBuffer))) {
                    debugSerial.println(operatorBuffer);
                }

                debugSerial.println(sodaq_3gbee.getNetworkTechnology());
                debugSerial.println(sodaq_3gbee.getNetworkStatus());

                int8_t rssi;
                uint8_t ber;
                for (uint8_t i = 0; i < 20; i++) {
                    if (sodaq_3gbee.getRSSIAndBER(&rssi, &ber)) {
                        debugSerial.print("RSSI:");
                        debugSerial.print(rssi);
                        debugSerial.print("dBm\t");
                        debugSerial.print("BER:");
                        debugSerial.println(ber);
                    }
                    else {
                        debugSerial.println("something went wrong with getting rssi and ber");
                    }

                    delay(2000);
                }
            }
#endif

#ifdef TEST_SIM_DEVICE_INFO
            {
                char numberBuffer[16];
                if (sodaq_3gbee.getMobileDirectoryNumber(numberBuffer, sizeof(numberBuffer))) {
                    debugSerial.print("Phone Number: ");
                    debugSerial.println(numberBuffer);
                }

                char imeiBuffer[16];
                if (sodaq_3gbee.getIMEI(imeiBuffer, sizeof(imeiBuffer))) {
                    debugSerial.print("IMEI: ");
                    debugSerial.println(imeiBuffer);
                }

                char ccidBuffer[24];
                if (sodaq_3gbee.getCCID(ccidBuffer, sizeof(ccidBuffer))) {
                    debugSerial.print("CCID: ");
                    debugSerial.println(ccidBuffer);
                }

                char imsiBuffer[24];
                if (sodaq_3gbee.getIMSI(imsiBuffer, sizeof(imsiBuffer))) {
                    debugSerial.print("IMSI: ");
                    debugSerial.println(imsiBuffer);
                }
            }
#endif

#ifdef TEST_FILESYSTEM
            {
                char writeBuffer[] = "this is, this is, this is a simple\r\ntest!";
                char readBuffer[64];
                sodaq_3gbee.writeFile("test_file", writeBuffer, sizeof(writeBuffer));

                size_t count = sodaq_3gbee.readFile("test_file", readBuffer, sizeof(readBuffer));
                debugSerial.print("count: "); debugSerial.println(count);

                printToLen(readBuffer, count);

                sodaq_3gbee.deleteFile("test_file");
            }
#endif

#ifdef TEST_HTTP
            {
                // GET
                char httpBuffer[512];
                size_t size = sodaq_3gbee.httpRequest("httpbin.org", 80, "/ip", GET, httpBuffer, sizeof(httpBuffer));
                printToLen(httpBuffer, size);
            }

            {
                // POST
                char postRawData[] = "testfield=test&someotherfield=test2";
                char postDataHeaders[] = "User-Agent: HTTPTool/1.0\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: " STR(sizeof(postData)-1) "\r\n";
                char postData[sizeof(postRawData) + sizeof(postDataHeaders) - 1];

                memcpy(postData, postDataHeaders, sizeof(postDataHeaders)-1);
                memcpy(postData + sizeof(postDataHeaders) - 1, postRawData, sizeof(postRawData));

                char httpBuffer[1024];
                size_t size = sodaq_3gbee.httpRequest("httpbin.org", 80, "/post", POST, httpBuffer, sizeof(httpBuffer), postData, sizeof(postData));
                printToLen(httpBuffer, size);
            }
#endif

#ifdef TEST_FTP
            {
                char loremIpsumParagraph[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer tempor vestibulum neque, ac consectetur ligula egestas vestibulum. Nullam at diam id magna hendrerit viverra non ut nunc. Vivamus ex leo, lacinia vel congue vel, lacinia at arcu. Donec eu tincidunt ex, porttitor ultrices ante. Praesent porttitor ultricies vehicula. Aliquam erat volutpat. Interdum et malesuada fames ac ante ipsum primis in faucibus. Etiam rhoncus suscipit urna, ut mattis nisl ullamcorper id. Donec lacus leo, sodales eget faucibus eu, tincidunt sit amet lorem. Vivamus non tellus ex. Vivamus eget est eu felis feugiat lobortis. Donec ultricies ultricies placerat. Aenean condimentum quam ut mi convallis, a ultrices mi imperdiet. Curabitur laoreet eu neque vitae porttitor.";
                sodaq_3gbee.openFtpConnection("server", "user", "pass", PASSIVE);
                
                sodaq_3gbee.openFtpFile("test.txt", "/test/test2/test3/");
                sodaq_3gbee.ftpSend(loremIpsumParagraph, sizeof(loremIpsumParagraph));
                sodaq_3gbee.closeFtpFile();

                sodaq_3gbee.openFtpFile("test.txt", "test/test2/test3");
                char buffer[1024];
                buffer[0] = 0;
                sodaq_3gbee.ftpReceive(buffer, sizeof(buffer));
                sodaq_3gbee.closeFtpFile();

                sodaq_3gbee.closeFtpConnection();

                if (strcmp(buffer, loremIpsumParagraph) == 0) {
                    debugSerial.println("Success: The file read from the ftp is the same as the one written!");
                }
            }
#endif

#ifdef TEST_SMS
            {

            }
#endif

            //sd_mbili_xbee.off();

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
        modemSerial.write(static_cast<char>(debugSerial.read()));
    }

    while (modemSerial.available())
    {
        debugSerial.write(static_cast<char>(modemSerial.read()));
    }
}
