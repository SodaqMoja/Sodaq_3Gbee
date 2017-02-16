#include <Sodaq_3Gbee.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if defined(ARDUINO_SODAQ_AUTONOMO)
#define MySerial SerialUSB
#define MY_BEE_VCC  BEE_VCC
#elif defined(ARDUINO_SODAQ_MBILI)
#define MY_BEE_VCC  -1
#define MySerial Serial
#else
#error "Please select Autonomo or Mbili"
#endif

#define modemSerial Serial1

#define APN "public4.m2minternet.com"
#define APN_USER NULL
#define APN_PASS NULL

//#define APN "aerea.m2m.com"
//#define APN_USER NULL
//#define APN_PASS NULL

#define TEST_DNS
#define TEST_SOCKETS
#define TEST_NETWORK_INFO
#define TEST_SIM_DEVICE_INFO
#define TEST_FILESYSTEM
#define TEST_HTTP
//#define TEST_FTP
//#define TEST_SMS

void printToLen(const char* buffer, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        MySerial.print(buffer[i]);
    }

    MySerial.println();
}

void printIpTuple(uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4)
{
    MySerial.print(o1);
    MySerial.print(".");
    MySerial.print(o2);
    MySerial.print(".");
    MySerial.print(o3);
    MySerial.print(".");
    MySerial.print(o4);
}

void changeModemBaudrate(uint32_t newBaudrate)
{
    MySerial.print("Main: changing baudrate to ");
    MySerial.println(newBaudrate);

    modemSerial.flush();
    modemSerial.end();
    modemSerial.begin(newBaudrate);
}

void setup()
{
    MySerial.begin(57600);
    modemSerial.begin(sodaq_3gbee.getDefaultBaudrate());
    delay(3000);
    MySerial.println("**Bootup**");

    sodaq_3gbee.setDiag(MySerial); // optional
    sodaq_3gbee.enableBaudrateChange(changeModemBaudrate); // optional
    
    delay(500);

#if defined(ARDUINO_SODAQ_AUTONOMO)
    sodaq_3gbee.init(Serial1, BEE_VCC, BEEDTR, BEECTS);
#elif defined(ARDUINO_SODAQ_WDT)
    sodaq_3gbee.init_wdt(Serial1, U2_ON);
#elif defined(ARDUINO_SODAQ_MBILI)
    sodaq_3gbee.init(Serial1, -1, BEEDTR, BEECTS);
#endif

    sodaq_3gbee.setApn(APN, APN_USER, APN_PASS);
    if (sodaq_3gbee.connect()) {
        MySerial.println("Modem connected to the apn successfully.");
        MySerial.println();
        MySerial.print("Local IP: ");
        IP_t localIp = sodaq_3gbee.getLocalIP();
        printIpTuple(IP_TO_TUPLE(localIp));
        MySerial.println();
        MySerial.println();

        delay(1000);
        MySerial.println();

#ifdef TEST_DNS
        {
            IP_t ip;
            MySerial.println("nslookup(\"www.google.com\")");
            ip = sodaq_3gbee.getHostIP("www.google.com");
            printIpTuple(IP_TO_TUPLE(ip));
            MySerial.println();

            delay(1000);
            MySerial.println();

            MySerial.println("nslookup(\"www.sodaq.com\") [== 149.210.181.239]");
            ip = sodaq_3gbee.getHostIP("www.sodaq.com");
            printIpTuple(IP_TO_TUPLE(ip));
            MySerial.println();

            delay(1000);
            MySerial.println();

            // this should fail and show 0.0.0.0
            MySerial.println("nslookup(\"www.odiygsifugvhfdkl.com\") [should fail]");
            ip = sodaq_3gbee.getHostIP("www.odiygsifugvhfdkl.com");
            printIpTuple(IP_TO_TUPLE(ip));
            MySerial.println();
        }
#endif

#ifdef TEST_SOCKETS
        {
            uint8_t socket = sodaq_3gbee.createSocket(TCP);
            if (sodaq_3gbee.connectSocket(socket, "54.175.103.105", 30000)) {
                MySerial.println("socket connected");

                sodaq_3gbee.socketSend(socket, "123456789");

                char receiveBuffer[128];
                memset(receiveBuffer, 0, sizeof(receiveBuffer));
                size_t bytesRead = sodaq_3gbee.socketReceive(socket, (uint8_t *)receiveBuffer, sizeof(receiveBuffer) - 1);

                MySerial.println();
                MySerial.println();
                MySerial.print("Bytes read: ");
                MySerial.println(bytesRead);

                for (size_t i = 0; i < bytesRead; i++)
                {
                    MySerial.print(receiveBuffer[i]);
                }

                MySerial.println();
            }
        }
#endif

#ifdef TEST_NETWORK_INFO
        {
            char operatorBuffer[16];
            if (sodaq_3gbee.getOperatorName(operatorBuffer, sizeof(operatorBuffer))) {
                MySerial.println(operatorBuffer);
            }

            MySerial.println(sodaq_3gbee.getNetworkTechnology());
            MySerial.println(sodaq_3gbee.getNetworkStatus());

            int8_t rssi;
            uint8_t ber;
            for (uint8_t i = 0; i < 20; i++) {
                if (sodaq_3gbee.getRSSIAndBER(&rssi, &ber)) {
                    MySerial.print("RSSI:");
                    MySerial.print(rssi);
                    MySerial.print("dBm\t");
                    MySerial.print("BER:");
                    MySerial.println(ber);
                }
                else {
                    MySerial.println("something went wrong with getting rssi and ber");
                }

                delay(2000);
            }
        }
#endif

#ifdef TEST_SIM_DEVICE_INFO
        {
            char numberBuffer[16];
            if (sodaq_3gbee.getMobileDirectoryNumber(numberBuffer, sizeof(numberBuffer))) {
                MySerial.print("Phone Number: ");
                MySerial.println(numberBuffer);
            }

            char imeiBuffer[16];
            if (sodaq_3gbee.getIMEI(imeiBuffer, sizeof(imeiBuffer))) {
                MySerial.print("IMEI: ");
                MySerial.println(imeiBuffer);
            }

            char ccidBuffer[24];
            if (sodaq_3gbee.getCCID(ccidBuffer, sizeof(ccidBuffer))) {
                MySerial.print("CCID: ");
                MySerial.println(ccidBuffer);
            }

            char imsiBuffer[24];
            if (sodaq_3gbee.getIMSI(imsiBuffer, sizeof(imsiBuffer))) {
                MySerial.print("IMSI: ");
                MySerial.println(imsiBuffer);
            }
        }
#endif

#ifdef TEST_FILESYSTEM
        {
            char writeBuffer[] = "this is, this is, this is a simple\r\ntest!";
            char readBuffer[64];
            sodaq_3gbee.writeFile("test_file", (uint8_t *)writeBuffer, sizeof(writeBuffer));

            size_t count = sodaq_3gbee.readFile("test_file", (uint8_t *)readBuffer, sizeof(readBuffer));
            MySerial.print("count: "); MySerial.println(count);

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
            // defaults to text/plain
            const char postBody[] = "testfield=test&someotherfield=test2";

            char httpBuffer[1024];
            size_t size = sodaq_3gbee.httpRequest("httpbin.org", 80, "/post", POST, httpBuffer, sizeof(httpBuffer), postBody, sizeof(postBody));

            printToLen(httpBuffer, size);
        }
#endif

#ifdef TEST_FTP
        {
            char loremIpsumParagraph[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer tempor vestibulum neque, ac consectetur ligula egestas vestibulum. Nullam at diam id magna hendrerit viverra non ut nunc. Vivamus ex leo, lacinia vel congue vel, lacinia at arcu. Donec eu tincidunt ex, porttitor ultrices ante. Praesent porttitor ultricies vehicula. Aliquam erat volutpat. Interdum et malesuada fames ac ante ipsum primis in faucibus. Etiam rhoncus suscipit urna, ut mattis nisl ullamcorper id. Donec lacus leo, sodales eget faucibus eu, tincidunt sit amet lorem. Vivamus non tellus ex. Vivamus eget est eu felis feugiat lobortis. Donec ultricies ultricies placerat. Aenean condimentum quam ut mi convallis, a ultrices mi imperdiet. Curabitur laoreet eu neque vitae porttitor.";
            sodaq_3gbee.openFtpConnection("server", "user", "pass", PassiveMode);
                
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
                MySerial.println("Success: The file read from the ftp is the same as the one written!");
            }
        }
#endif

#ifdef TEST_SMS
        {
            if (sodaq_3gbee.sendSms("0031630191744", "This is a test SMS!")) {
                MySerial.println("The SMS was sent successfully!");
            }

            int indexes[128];
            int count = sodaq_3gbee.getSmsList("ALL", indexes, sizeof(indexes));
            if (count < 0) {
                MySerial.println("SMS list error!");
            } 
            else {
                for (int i = 0; i < count; i++) {
                    MySerial.println(indexes[i]);
                }
            }

            char phoneNo[16];
            char message[256];
            sodaq_3gbee.readSms(1, phoneNo, message, sizeof(message));
        }
#endif

//        sodaq_3gbee.off();
//            if (sodaq_3gbee.disconnect()) {
//                MySerial.println("Modem was successfully disconnected.");
//            }
    }
    else {
        MySerial.println("Modem failed to connect to the apn!");
    }
}

void loop()
{
    while (MySerial.available())
    {
        modemSerial.write(static_cast<char>(MySerial.read()));
    }

    while (modemSerial.available())
    {
        MySerial.write(static_cast<char>(modemSerial.read()));
    }
}
