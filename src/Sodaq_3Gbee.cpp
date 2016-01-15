#include <Arduino.h>

#if defined(ARDUINO_ARCH_SAMD)
static inline void wait_while_wdt_syncbusy() __attribute__((always_inline));
static inline void wait_while_wdt_syncbusy()
{
  while (WDT->STATUS.reg & WDT_STATUS_SYNCBUSY) {
  }
}

static inline void wdt_reset() __attribute__((always_inline));
static inline void wdt_reset()
{
  // Reset count
  WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
  wait_while_wdt_syncbusy();
}
#elif defined(ARDUINO_ARCH_AVR)
#include <avr/wdt.h>
#else
#error "Only AVR or SAMD supported"
#endif

#include "Sodaq_3Gbee.h"

#define DEBUG

#define STR_AT "AT"
#define STR_RESPONSE_OK "OK"
#define STR_RESPONSE_ERROR "ERROR"
#define STR_RESPONSE_CME_ERROR "+CME ERROR:"
#define STR_RESPONSE_CMS_ERROR "+CMS ERROR:"
#define STR_RESPONSE_SOCKET_PROMPT "@"
#define STR_RESPONSE_SMS_PROMPT ">"
#define STR_RESPONSE_FILE_PROMPT ">"

#define DEBUG_STR_ERROR "[ERROR]: "

#define NIBBLE_TO_HEX_CHAR(i) ((i <= 9) ? ('0' + i) : ('A' - 10 + i))
#define HIGH_NIBBLE(i) ((i >> 4) & 0x0F)
#define LOW_NIBBLE(i) (i & 0x0F)

#define HEX_CHAR_TO_NIBBLE(c) ((c >= 'A') ? (c - 'A' + 0x0A) : (c - '0'))
#define HEX_PAIR_TO_BYTE(h, l) ((HEX_CHAR_TO_NIBBLE(h) << 4) + HEX_CHAR_TO_NIBBLE(l))

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#ifdef DEBUG
#define debugPrintLn(...) { if (this->_diagStream) this->_diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (this->_diagStream) this->_diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

#define BLOCK_TIMEOUT -1
#define DEFAULT_PROFILE "0"
#define HIGH_BAUDRATE 57600
#define MAX_SOCKET_BUFFER 512
#define DEFAULT_HTTP_SEND_TMP_FILENAME "http_tmp_put_0"
#define DEFAULT_HTTP_RECEIVE_TMP_FILENAME "http_last_response_0"
#define DEFAULT_FTP_TMP_FILENAME "ftp_tmp_file"
#define CTRL_Z (static_cast<char>(0x1A))

#define NOW (uint32_t)millis()

static inline bool is_timedout(uint32_t from, uint32_t nr_ms) __attribute__((always_inline));
static inline bool is_timedout(uint32_t from, uint32_t nr_ms)
{
    return (millis() - from) > nr_ms;
}


// A specialized class to switch on/off the 3Gbee module
// The VCC3.3 pin is switched by the Autonomo BEE_VCC pin
// The DTR pin is the actual ON/OFF pin, it is A13 on Autonomo, D20 on Tatu
class Sodaq_3GbeeOnOff : public Sodaq_OnOffBee
{
public:
    Sodaq_3GbeeOnOff();
    void init(int vcc33Pin, int onoffPin, int statusPin);
    void on();
    void off();
    bool isOn();
private:
    int8_t _vcc33Pin;
    int8_t _onoffPin;
    int8_t _statusPin;
};

static Sodaq_3GbeeOnOff sodaq_3gbee_onoff;

Sodaq_3Gbee sodaq_3gbee;

Sodaq_3Gbee::Sodaq_3Gbee()
{
    _openTCPsocket = -1;
}

bool Sodaq_3Gbee::startsWith(const char* pre, const char* str)
{
    return (strncmp(pre, str, strlen(pre)) == 0);
}

ResponseTypes Sodaq_3Gbee::readResponse(char* buffer, size_t size,
        CallbackMethodPtr parserMethod, void* callbackParameter, void* callbackParameter2,
        size_t* outSize, uint32_t timeout)
{
    ResponseTypes response = ResponseNotFound;
    uint32_t from = NOW;

    do {
        // 250ms,  how many bytes at which baudrate?
        int count = readLn(buffer, size, 250);
        wdt_reset();
        
        if (count > 0) {
            if (outSize) {
                *outSize = count;
            }

            debugPrint("[rdResp]: ");
            debugPrint(buffer);
            debugPrint(" (");
            debugPrint(count);
            debugPrintLn(")");

            // handle unsolicited codes
            // TODO +UUPSDD
            int param1, param2;
            if (sscanf(buffer, "+UUSORD: %d,%d", &param1, &param2) == 2) {
                debugPrint("Unsolicited: Socket ");
                debugPrint(param1);
                debugPrint(": ");
                debugPrint(param2);
                debugPrintLn("bytes pending");
                if (static_cast<uint16_t>(param1) < ARRAY_SIZE(_socketPendingBytes)) {
                    _socketPendingBytes[param1] = param2;
                }
            }
            else if (sscanf(buffer, "+UUSOCL: %d", &param1) == 1) {
                if (static_cast<uint16_t>(param1) < ARRAY_SIZE(_socketPendingBytes)) {
                    debugPrint("Unsolicited: Socket ");
                    debugPrint(param1);
                    debugPrint(": ");
                    debugPrintLn("closed by remote");

                    _socketClosedBit[param1] = true;
                }
            }
            else if (sscanf(buffer, "+UUHTTPCR: 0, %d, %d", &param1, &param2) == 2) {
                int requestType = _httpModemIndexToRequestType(static_cast<uint16_t>(param1));
                if (requestType >= 0) {
                    debugPrint("HTTP Result for request type ");
                    debugPrint(requestType);
                    debugPrint(": ");
                    debugPrintLn(param2);

                    if (param2 == 0) {
                        _httpRequestSuccessBit[requestType] = TriBoolFalse;
                    }
                    else if (param2 == 1) {
                        _httpRequestSuccessBit[requestType] = TriBoolTrue;
                    }
                }
            }
            else if (sscanf(buffer, "+UUFTPCR: %d, %d", &param1, &param2) == 2) {
                debugPrint("FTP Result for command ");
                debugPrint(param1);
                debugPrint(": ");
                debugPrintLn(param2);

                ftpCommandURC[0] = static_cast<uint8_t>(param1);
                ftpCommandURC[1] = static_cast<uint8_t>(param2);
            }

            if (startsWith(STR_AT, buffer)) {
                continue; // skip echoed back command
            }
            
            if (startsWith(STR_RESPONSE_OK, buffer)) {
                return ResponseOK;
            }
            
            if (startsWith(STR_RESPONSE_ERROR, buffer) ||
                    startsWith(STR_RESPONSE_CME_ERROR, buffer) ||
                    startsWith(STR_RESPONSE_CMS_ERROR, buffer)) {
                return ResponseError;
            }
            
            if (startsWith(STR_RESPONSE_SOCKET_PROMPT, buffer) ||
                    startsWith(STR_RESPONSE_SMS_PROMPT, buffer) ||
                    startsWith(STR_RESPONSE_FILE_PROMPT, buffer)) {
                return ResponsePrompt;
            }

            if (parserMethod) {
                ResponseTypes parserResponse = parserMethod(response, buffer, count, callbackParameter, callbackParameter2);
                if (parserResponse != ResponseEmpty) {
                    return parserResponse;
                }
            }

            // at this point, the parserMethod has ran and there is no override response from it, 
            // so if there is some other response recorded, return that
            // (otherwise continue iterations until timeout)
            if (response != ResponseNotFound) {
                debugPrintLn("** response != ResponseNotFound");
                return response;
            }
        }

        delay(10);
    } while (!is_timedout(from, timeout));

    if (outSize) {
        *outSize = 0;
    }

    debugPrintLn("[rdResp]: timed out");
    return ResponseTimeout;
}

bool Sodaq_3Gbee::setSimPin(const char* simPin)
{
    write("AT+CPIN=\"");
    write(simPin);
    writeLn("\"");

    return (readResponse() == ResponseOK);
}

// Returns true if the modem is connected to the network and has an activated data connection.
bool Sodaq_3Gbee::isConnected()
{
    uint8_t value = 0;

    writeLn("AT+UPSND=" DEFAULT_PROFILE ",8");
    if (readResponse<uint8_t, uint8_t>(_upsndParser, &value, NULL) == ResponseOK) {
        return (value == 1);
    }

    return false;
}

bool Sodaq_3Gbee::waitForFtpCommandResult(uint8_t ftpCommandIndex, uint32_t timeout)
{
    uint32_t start = millis();

    ftpCommandURC[0] = 0xFF; // set to unused value to clear (0 is used)
    ftpCommandURC[1] = 0;
    while (ftpCommandURC[0] != ftpCommandIndex && !is_timedout(start, timeout)) {
        isAlive();
        delay(5);
    }

    return (ftpCommandURC[0] == ftpCommandIndex) && (ftpCommandURC[1] == 1);
}

bool Sodaq_3Gbee::changeFtpDirectory(const char* directory)
{
    write("AT+UFTPC=8,\"");
    write(directory);
    writeLn("\"");

    if ((readResponse() != ResponseOK) || (!waitForFtpCommandResult(8))) {
        return false;
    }

    if (strcmp("..", directory)) {
        ftpDirectoryChangeCounter--;
    }
    else {
        ftpDirectoryChangeCounter++;
    }

    return true;
}

void Sodaq_3Gbee::resetFtpDirectoryIfNeeded()
{
    uint8_t maxTries = 2 * ftpDirectoryChangeCounter;
    while (ftpDirectoryChangeCounter > 0 && maxTries > 0) {
        changeFtpDirectory("..");
        maxTries--;
    }
}

void Sodaq_3Gbee::cleanupTempFiles()
{
    deleteFile(DEFAULT_FTP_TMP_FILENAME);
    deleteFile(DEFAULT_HTTP_RECEIVE_TMP_FILENAME);
    deleteFile(DEFAULT_HTTP_SEND_TMP_FILENAME);
}

// Returns true if the modem replies to "AT" commands without timing out.
bool Sodaq_3Gbee::isAlive()
{
    writeLn(STR_AT);

    return (readResponse(NULL, 450) == ResponseOK);
}

// Sets the apn, apn username and apn password to the modem.
bool Sodaq_3Gbee::setAPN(const char* apn, const char* username, const char* password)
{
    write("AT+UPSD=" DEFAULT_PROFILE ",1,\"");
    write(apn);
    writeLn("\"");

    if (readResponse() != ResponseOK) {
        return false;
    }

    write("AT+UPSD=" DEFAULT_PROFILE ",2,\"");
    write(username);
    writeLn("\"");

    if (readResponse() != ResponseOK) {
        return false;
    }

    write("AT+UPSD=" DEFAULT_PROFILE ",3,\"");
    write(password);
    writeLn("\"");

    if (readResponse() != ResponseOK) {
        return false;
    }

    return true;
}

// Initializes the modem instance. Sets the modem stream and the on-off power pins.
void Sodaq_3Gbee::init(Stream& stream, int8_t vcc33Pin, int8_t onoffPin, int8_t statusPin)
{
    debugPrintLn("[init] started.");

    initBuffer(); // safe to call multiple times

    setModemStream(stream);

    sodaq_3gbee_onoff.init(vcc33Pin, onoffPin, statusPin);
    _onoff = &sodaq_3gbee_onoff;
}

// Turns on and initializes the modem, then connects to the network and activates the data connection.
bool Sodaq_3Gbee::connect(const char* simPin, const char* apn, const char* username, const char* password,
        AuthorizationTypes authorization)
{
    if (!on()) {
        return false;
    }

    ResponseTypes response = ResponseEmpty;

    // echo off
    writeLn("AT E0");
    for (uint8_t i = 0; i < 5; i++) {
        response = readResponse();
        if (response == ResponseOK) {
            break;
        }
    }
    if (response != ResponseOK) {
        return false;
    }

    // if supported by target application, change the baudrate
    if (_baudRateChangeCallbackPtr) {
        writeLn("AT+IPR=" STR(HIGH_BAUDRATE));
        if (readResponse() != ResponseOK) {
            return false;
        }

        _baudRateChangeCallbackPtr(HIGH_BAUDRATE);
        delay(1000); // wait for eveyrhing to be stable again
    }

    // verbose error messages
    writeLn("AT+CMEE=2");
    if (readResponse() != ResponseOK) {
        return false;
    }

    // Switch sockets to hex mode
    writeLn("AT+UDCONF=1,1"); // second param is 1=ON, 0=OFF
    if (readResponse() != ResponseOK) {
        return false;
    }

    // enable network identification LED
    writeLn("AT+UGPIOC=16,2");
    if (readResponse() != ResponseOK) {
        return false;
    }

    // SIM check
    bool simOK = false;
    for (uint8_t i = 0; i < 5; i++) {
        SimStatuses simStatus = getSimStatus();
        if (simStatus == SimNeedsPin) {
            if (!(*simPin) || !setSimPin(simPin)) {
                debugPrintLn("[ERROR]: SIM needs a PIN but none was provided, or setting it failed!");
                return false;
            }
        }
        else if (simStatus == SimReady) {
            simOK = true;
            break;
        }

        delay(20);
    }

    if (!simOK) {
        return false;
    }

    // enable auto network registration
    writeLn("AT+COPS=0");
    if (readResponse(NULL, 400000) != ResponseOK) {
        return false;
    }

    // NOTE: Default +URAT = 1,2 i.e. prefer UMTS, fallback to GPRS

    // cleanup tmp files
    cleanupTempFiles();

    // set SMS to text mode
    writeLn("AT+CMGF=1");
    if (readResponse() != ResponseOK) {
        return false;
    }

    // TODO check GPRS attach? (AT+CGATT=1 should be OK)
    
    // check if connected and disconnect
    if (isConnected()) {
        if (!disconnect()) {
            debugPrintLn("[ERROR] Modem seems to be already connected and failed to disconnect!");
            return false;
        }
    }

    if (!setAPN(apn, username, password)) {
        return false;
    }

    // DHCP
    writeLn("AT+UPSD=" DEFAULT_PROFILE ",7,\"0.0.0.0\"");
    if (readResponse() != ResponseOK) {
        return false;
    }

    // go through all authentication methods to check against the selected one or 
    // to autodetect (first successful)
    for (uint8_t i = NoAuthorization; i < AutoDetectAutorization; i++) {
        if ((authorization == AutoDetectAutorization) || (authorization == i)) {
            // Set Authentication
            write("AT+UPSD=" DEFAULT_PROFILE ",6,");
            writeLn(i);
            if (readResponse() != ResponseOK) {
                return false;
            }

            // connect using default profile
            writeLn("AT+UPSDA=" DEFAULT_PROFILE ",3");
            if (readResponse(NULL, 200000) == ResponseOK) {
                return true;
            }
        }
    }

    return false;
}

// Disconnects the modem from the network.
bool Sodaq_3Gbee::disconnect()
{
    // TODO also turn off the modem?
    writeLn("AT+UPSDA=" DEFAULT_PROFILE ",4");

    return (readResponse(NULL, 40000) == ResponseOK);
}

ResponseTypes Sodaq_3Gbee::_cregParser(ResponseTypes& response, const char* buffer, size_t size,
        int* networkStatus, uint8_t* dummy)
{
    if (!networkStatus) {
        return ResponseError;
    }

    if (sscanf(buffer, "+CREG: %*d,%d", networkStatus) == 1) {
        return ResponseEmpty;
    }

    return ResponseError;
}

// Returns the current status of the network.
NetworkRegistrationStatuses Sodaq_3Gbee::getNetworkStatus()
{
    writeLn("AT+CREG?"); // TODO ? +CGREG

    int networkStatus;
    if (readResponse<int, uint8_t>(_cregParser, &networkStatus, NULL) == ResponseOK) {
        switch (networkStatus) {
            case 0: return NoNetworkRegistrationStatus;
            case 1: return Home;
            case 2: return NoNetworkRegistrationStatus;
            case 3: return Denied;
            case 4: return UnknownNetworkRegistrationStatus;
            default: return Denied; // rest of statuses are actually registered, but they do not support data
        }
    }

    return UnknownNetworkRegistrationStatus;
}

// Returns the network technology the modem is currently registered to.
NetworkTechnologies Sodaq_3Gbee::getNetworkTechnology()
{
    writeLn("AT+COPS?");

    int networkTechnology;
    if (readResponse<int, uint8_t>(_copsParser, &networkTechnology, NULL) == ResponseOK) {
        switch (networkTechnology) {
            case 0: return GSM;
            case 1: return GSM;
            case 2: return UTRAN;
            case 3: return EDGE;
            case 4: return HSDPA;
            case 5: return HSUPA;
            case 6: return HSDPAHSUPA;
            case 7: return LTE;
        }
    }

    return UnknownNetworkTechnology;
}

ResponseTypes Sodaq_3Gbee::_csqParser(ResponseTypes& response, const char* buffer, size_t size,
        int* rssi, int* ber)
{
    if (!rssi || !ber) {
        return ResponseError;
    }

    if (sscanf(buffer, "+CSQ: %d,%d", rssi, ber) == 2) {
        return ResponseEmpty;
    }

    return ResponseError;
}

// Gets the Received Signal Strength Indication in dBm and Bit Error Rate.
// Returns true if successful.
bool Sodaq_3Gbee::getRSSIAndBER(int8_t* rssi, uint8_t* ber)
{
    static char berValues[] = { 49, 43, 37, 25, 19, 13, 7, 0 }; // 3GPP TS 45.008 [20] subclause 8.2.4
    
    writeLn("AT+CSQ");

    int rssiRaw = 0;
    int berRaw = 0;

    if (readResponse<int, int>(_csqParser, &rssiRaw, &berRaw) == ResponseOK) {
        *rssi = ((rssiRaw == 99) ? 0 : -113 + 2 * rssiRaw);
        *ber = ((berRaw == 99 || static_cast<unsigned>(berRaw) >= sizeof(berValues)) ? 0 : berValues[berRaw]);
        
        return true;
    }

    return false;
}

ResponseTypes Sodaq_3Gbee::_copsParser(ResponseTypes& response, const char* buffer, size_t size,
        char* operatorNameBuffer, size_t* operatorNameBufferSize)
{
    if (!operatorNameBuffer || !operatorNameBufferSize) {
        return ResponseError;
    }

    // TODO maybe limit length of string in format? needs converting int to string though
    if (sscanf(buffer, "+COPS: %*d,%*d,\"%[^\"]\",%*d", operatorNameBuffer) == 1) {
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes Sodaq_3Gbee::_copsParser(ResponseTypes& response, const char* buffer, size_t size,
        int* networkTechnology, uint8_t* dummy)
{
    if (!networkTechnology) {
        return ResponseError;
    }

    if (sscanf(buffer, "+COPS: %*d,%*d,\"%*[^\"]\",%d", networkTechnology) == 1) {
        return ResponseEmpty;
    }

    return ResponseError;
}

// Gets the Operator Name.
// Returns true if successful.
bool Sodaq_3Gbee::getOperatorName(char* buffer, size_t size)
{
    if (size > 0) {
        buffer[0] = 0;
    }

    writeLn("AT+COPS?");

    return (readResponse<char, size_t>(_copsParser, buffer, &size) == ResponseOK);
}

ResponseTypes Sodaq_3Gbee::_cnumParser(ResponseTypes& response, const char* buffer, size_t size,
        char* numberBuffer, size_t* numberBufferSize)
{
    if (!numberBuffer || !numberBufferSize) {
        return ResponseError;
    }

    // TODO test with a sim that has a number
    // TODO limit?
    if (sscanf(buffer, "+CNUM: \"%*[^\"]\",\"%[^\"]\",%*d", numberBuffer) == 1) { // TODO is it "My Number"?
        return ResponseEmpty;
    }

    return ResponseError;

}

// Gets Mobile Directory Number.
// Returns true if successful.
bool Sodaq_3Gbee::getMobileDirectoryNumber(char* buffer, size_t size)
{
    if (size < 7 + 1) {
        return false;
    }

    if (size > 0) {
        buffer[0] = 0;
    }

    writeLn("AT+CNUM");

    return (readResponse<char, size_t>(_cnumParser, buffer, &size) == ResponseOK);
}

ResponseTypes Sodaq_3Gbee::_nakedStringParser(ResponseTypes& response, const char* buffer,
        size_t size, char* stringBuffer, size_t* stringBufferSize)
{
    if (!stringBuffer || !stringBufferSize) {
        return ResponseError;
    }

    if (*stringBufferSize > 0)
    {
        stringBuffer[0] = 0;
        strncat(stringBuffer, buffer, *stringBufferSize - 1);

        return ResponseEmpty;
    }

    return ResponseError;
}

// Gets International Mobile Equipment Identity.
// Should be provided with a buffer of at least 16 bytes.
// Returns true if successful.
bool Sodaq_3Gbee::getIMEI(char* buffer, size_t size)
{
    if (size < 15 + 1) {
        return false;
    }

    if (size > 0) {
        buffer[0] = 0;
    }

    writeLn("AT+CGSN");

    return (readResponse<char, size_t>(_nakedStringParser, buffer, &size) == ResponseOK);
}

ResponseTypes Sodaq_3Gbee::_ccidParser(ResponseTypes& response, const char* buffer, size_t size,
        char* ccidBuffer, size_t* ccidBufferSize)
{
    if (!ccidBuffer || !ccidBufferSize) {
        return ResponseError;
    }

    // TODO limit?
    if (sscanf(buffer, "+CCID: %s", ccidBuffer) == 1) {
        return ResponseEmpty;
    }

    return ResponseError;
}

// Gets Integrated Circuit Card ID.
// Should be provided with a buffer of at least 21 bytes.
// Returns true if successful.
bool Sodaq_3Gbee::getCCID(char* buffer, size_t size)
{
    if (size < 20 + 1) {
        return false;
    }

    if (size > 0) {
        buffer[0] = 0;
    }

    writeLn("AT+CCID");

    return (readResponse<char, size_t>(_ccidParser, buffer, &size) == ResponseOK);
}

// Gets the International Mobile Station Identity.
// Should be provided with a buffer of at least 16 bytes.
// Returns true if successful.
bool Sodaq_3Gbee::getIMSI(char* buffer, size_t size)
{
    if (size < 15 + 1) {
        return false;
    }

    if (size > 0) {
        buffer[0] = 0;
    }

    writeLn("AT+CIMI");

    return (readResponse<char, size_t>(_nakedStringParser, buffer, &size) == ResponseOK);
}

ResponseTypes Sodaq_3Gbee::_cpinParser(ResponseTypes& response, const char* buffer, size_t size,
        SimStatuses* parameter, uint8_t* dummy)
{
    if (!parameter) {
        return ResponseError;
    }

    char status[16];
    if (sscanf(buffer, "+CPIN: %" STR(sizeof(status)-1) "s", status) == 1) {
        if (startsWith("READY", status)) {
            *parameter = SimReady;
        }
        else {
            *parameter = SimNeedsPin;
        }

        return ResponseEmpty;
    }
    
    return ResponseError;
}

// Returns the current SIM status.
SimStatuses Sodaq_3Gbee::getSimStatus()
{
    SimStatuses simStatus;

    writeLn("AT+CPIN?");
    if (readResponse<SimStatuses, uint8_t>(_cpinParser, &simStatus, NULL) == ResponseOK) {
        return simStatus;
    }

    return SimMissing;
}

// Returns the local IP Address.
IP_t Sodaq_3Gbee::getLocalIP()
{
    IP_t ip = NO_IP_ADDRESS;

    writeLn("AT+UPSND=" DEFAULT_PROFILE ",0");
    if (readResponse<IP_t, uint8_t>(_upsndParser, &ip, NULL) == ResponseOK) {
        return ip;
    }

    return NO_IP_ADDRESS;
}

ResponseTypes Sodaq_3Gbee::_udnsrnParser(ResponseTypes& response, const char* buffer, size_t size,
        IP_t* ipResult, uint8_t* dummy)
{
    if (!ipResult) {
        return ResponseError;
    }

    int o1, o2, o3, o4;

    if (sscanf(buffer, "+UDNSRN: \"" IP_FORMAT "\"", &o1, &o2, &o3, &o4) == 4) {
        *ipResult = TUPLE_TO_IP(o1, o2, o3, o4);
        
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes Sodaq_3Gbee::_upsndParser(ResponseTypes& response, const char* buffer, size_t size,
        IP_t* ipResult, uint8_t* dummy)
{
    if (!ipResult) {
        return ResponseError;
    }

    int o1, o2, o3, o4;

    if (sscanf(buffer, "+UPSND: " DEFAULT_PROFILE ", 0, \"" IP_FORMAT "\"", &o1, &o2, &o3, &o4) == 4) {
        *ipResult = TUPLE_TO_IP(o1, o2, o3, o4);

        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes Sodaq_3Gbee::_upsndParser(ResponseTypes& response, const char* buffer, size_t size,
        uint8_t* thirdParam, uint8_t* dummy)
{
    if (!thirdParam) {
        return ResponseError;
    }

    int value;

    if (sscanf(buffer, "+UPSND: %*d,%*d,%d", &value) == 1) {
        *thirdParam = value;

        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes Sodaq_3Gbee::_usocrParser(ResponseTypes& response, const char* buffer, size_t size,
        uint8_t* socket, uint8_t* dummy)
{
    if (!socket) {
        return ResponseError;
    }

    int value;

    if (sscanf(buffer, "+USOCR: %d", &value) == 1) {
        *socket = value;

        return ResponseEmpty;
    }

    return ResponseError;
}

// Returns the IP of the given host (nslookup).
IP_t Sodaq_3Gbee::getHostIP(const char* host)
{
    IP_t ip = NO_IP_ADDRESS;

    write("AT+UDNSRN=0,\"");
    write(host);
    writeLn("\"");
    if (readResponse<IP_t, uint8_t>(_udnsrnParser, &ip, NULL) == ResponseOK) {
        return ip;
    }

    return NO_IP_ADDRESS;
}

// Creates a new socket for the given protocol, optionally bound to the given localPort.
// Returns the index of the socket created or -1 in case of error.
int Sodaq_3Gbee::createSocket(Protocols protocol, uint16_t localPort)
{
    uint8_t protocolIndex;
    if (protocol == TCP) {
        protocolIndex = 6;
    }
    else if (protocol == UDP) {
        protocolIndex = 17;
    }
    else {
        return SOCKET_FAIL;
    }

    write("AT+USOCR=");
    if (localPort > 0) {
        write(protocolIndex);
        write(",");
        writeLn(static_cast<uint32_t>(localPort));
    }
    else {
        writeLn(protocolIndex);
    }

    uint8_t socket;
    if (readResponse<uint8_t, uint8_t>(_usocrParser, &socket, NULL) == ResponseOK) {
        return socket;
    }

    return SOCKET_FAIL;
}

size_t Sodaq_3Gbee::ipToString(IP_t ip, char* buffer, size_t size)
{
    return snprintf(buffer, size, IP_FORMAT, IP_TO_TUPLE(ip));
}

bool Sodaq_3Gbee::isValidIPv4(const char* str)
{
    uint8_t segs = 0; // Segment count
    uint8_t chcnt = 0; // Character count within segment
    uint8_t accum = 0; // Accumulator for segment

    if (!str) {
        return false;
    }

    // Process every character in string
    while (*str != '\0') {
        // Segment changeover
        if (*str == '.') {
            // Must have some digits in segment
            if (chcnt == 0) {
                return false;
            }

            // Limit number of segments
            if (++segs == 4) {
                return false;
            }

            // Reset segment values and restart loop
            chcnt = accum = 0;
            str++;
            continue;
        }

        // Check numeric
        if ((*str < '0') || (*str > '9')) {
            return false;
        }

        // Accumulate and check segment
        if ((accum = accum * 10 + *str - '0') > 255) {
            return false;
        }

        // Advance other segment specific stuff and continue loop
        chcnt++;
        str++;
    }

    // Check enough segments and enough characters in last segment
    if (segs != 3) {
        return false;
    }

    if (chcnt == 0) {
        return false;
    }

    // Address OK

    return true;
}

// Requests a connection to the given host and port, on the given socket.
// Returns true if successful.
bool Sodaq_3Gbee::connectSocket(uint8_t socket, const char* host, uint16_t port)
{
    bool usePassedHost;
    char ipBuffer[16];

    if (isValidIPv4(host)) {
        usePassedHost = true;
    }
    else {
        usePassedHost = false;
        if (ipToString(getHostIP(host), ipBuffer, sizeof(ipBuffer)) <= 0) {
            return false;
        }
    }

    _socketClosedBit[socket] = false;
    write("AT+USOCO=");
    write(socket);
    write(",\"");
    write(usePassedHost ? host : ipBuffer);
    write("\",");
    writeLn(static_cast<uint32_t>(port));

    return (readResponse(NULL, 30000) == ResponseOK);
}

// Sends the given buffer through the given socket.
// Returns true if successful.
bool Sodaq_3Gbee::socketSend(uint8_t socket, const uint8_t* buffer, size_t size)
{
    // TODO see if we should keep an array of sockets so that the UDP-specific 
    // commands can be used instead, without first initializing the UDP socket
    // OR maybe query the socket type? AT+USOCTL=0,0  => +USOCTL:0,0,6 (socket #0 is TCP)

    // TODO +USOCTL=1 check last error, (11: queue full)

    write("AT+USOWR=");
    write(socket);
    write(",");
    write(static_cast<uint32_t>(size));
    write(",\"");
    for (size_t i = 0; i < size; ++i) {
        write(static_cast<char>(NIBBLE_TO_HEX_CHAR(HIGH_NIBBLE(buffer[i]))));
        write(static_cast<char>(NIBBLE_TO_HEX_CHAR(LOW_NIBBLE(buffer[i]))));
        // TODO segment and check queue full?
    }

    writeLn("\"");

    return (readResponse(NULL, 10000) == ResponseOK);
}

ResponseTypes Sodaq_3Gbee::_usordParser(ResponseTypes& response, const char* buffer, size_t size,
        char* resultBuffer, uint8_t* dummy)
{
    if (!resultBuffer) {
        return ResponseError;
    }

    int socket, count;
    if ((sscanf(buffer, "+USORD: %d,%d,", &socket, &count) == 2)
        && (buffer[size - count*2 - 2] == '\"')
        && (buffer[size - 1] == '\"')
        && (count < MAX_SOCKET_BUFFER)) {
        memcpy(resultBuffer, &buffer[size - 1 - count*2], count*2);
        resultBuffer[count*2] = 0;

        return ResponseEmpty;
    }

    return ResponseError;
}

// Reads data from the given socket into the given buffer.
// Returns the number of bytes written to the buffer.
// NOTE: if the modem hasn't reported available data, it blocks for up to 10 seconds waiting.
size_t Sodaq_3Gbee::socketReceive(uint8_t socket, uint8_t* buffer, size_t size)
{
    if (socket >= ARRAY_SIZE(_socketPendingBytes)) {
        return 0;
    }

    // if there are no data available yet, block for some seconds while checking
    uint32_t start = millis();
    while (_socketPendingBytes[socket] == 0 && isAlive() && !is_timedout(start, 10000)) {
        delay(5);
    }

    size_t pending = _socketPendingBytes[socket];
    size_t count = (pending > size) ? size : pending;

    if (pending == 0) {
        return 0;
    }

    // bound the count, as the socket bytes are in hex string (so 2 * bytes)
    if (count > MAX_SOCKET_BUFFER/2) {
        count = MAX_SOCKET_BUFFER/2;
    }

    write("AT+USORD=");
    write(socket);
    write(",");
    writeLn(static_cast<uint32_t>(count));

    char resultBuffer[MAX_SOCKET_BUFFER];
    if (readResponse<char, uint8_t>(_usordParser, resultBuffer, NULL) == ResponseOK) {
        // stop at the first string termination char, or if output buffer is over, or if payload buffer is over
        size_t outputIndex = 0;
        size_t inputIndex = 0;

        while (outputIndex < count
            && resultBuffer[inputIndex] != 0
            && resultBuffer[inputIndex + 1] != 0) {
            buffer[outputIndex] = HEX_PAIR_TO_BYTE(resultBuffer[inputIndex], resultBuffer[inputIndex + 1]);

            inputIndex += 2;
            outputIndex++;
        }

        _socketPendingBytes[socket] -= count;
        return count;
    }

    return 0;
}

// Closes the given socket.
// Returns true if successful.
bool Sodaq_3Gbee::closeSocket(uint8_t socket)
{
    write("AT+USOCL=");
    writeLn(socket);

    return (readResponse(NULL, 20000) == ResponseOK);
}

// Blocks waiting for the given socket to be reported closed.
// This method should be called only after closeSocket() or when the remote is expected to close the socket.
// Times out after 60 seconds.
void Sodaq_3Gbee::waitForSocketClose(uint8_t socket, uint32_t timeout)
{
    debugPrint("[waitForSocketClose]: ");
    debugPrintLn(socket);

    uint32_t start = millis();

    while (isAlive() && (!_socketClosedBit[socket]) && (!is_timedout(start, timeout))) {
        delay(5);
    };
}

// ==== TCP

bool Sodaq_3Gbee::openTCP(const char *apn, const char *apnuser, const char *apnpwd,
            const char *server, int port, bool transMode)
{
    // TODO Verify this
    bool retval = false;
    if (on()) {
        if (sodaq_3gbee.connect(NULL, apn, apnuser, apnpwd, NoAuthorization)) {
            // IP_t ip = sodaq_3gbee.getHostIP(server);
            _openTCPsocket = sodaq_3gbee.createSocket(TCP);
            // TODO Use ip instead of hostname
            if (sodaq_3gbee.connectSocket(_openTCPsocket, server, port)) {
                retval = true;
            }
        }
    }
    return retval;
}

void Sodaq_3Gbee::closeTCP(bool switchOff)
{
    // TODO Verify this
    if (_openTCPsocket >= 0) {
        closeSocket(_openTCPsocket);
        //waitForSocketClose(_openTCPsocket);
        _openTCPsocket = -1;
    }
    if (switchOff) {
        off();
    }
}

bool Sodaq_3Gbee::sendDataTCP(const uint8_t *data, size_t data_len)
{
    // TODO Verify this
    bool retval = false;
    if (_openTCPsocket >= 0) {
        if (socketSend(_openTCPsocket, data, data_len)) {
            retval = true;
        }
    }
    return retval;
}

bool Sodaq_3Gbee::receiveDataTCP(uint8_t *data, size_t data_len, uint16_t timeout)
{
    // TODO Verify this
    bool retval = false;
    if (_openTCPsocket >= 0) {
        size_t nrbytes = socketReceive(_openTCPsocket, data, data_len);
        if (nrbytes == data_len) {
            retval = true;
        }
    }
    return retval;
}

// ==== HTTP

// Creates an HTTP request using the (optional) given buffer and 
// (optionally) returns the received data.
// endpoint should include the initial "/".
size_t Sodaq_3Gbee::httpRequest(const char* url, uint16_t port,
        const char* endpoint, HttpRequestTypes requestType,
        char* responseBuffer, size_t responseSize,
        const char* sendBuffer, size_t sendSize)
{
    // TODO maybe return error <0 ?

    // reset http profile 0
    writeLn("AT+UHTTP=0");
    if (readResponse() != ResponseOK) {
        return 0;
    }

    deleteFile(DEFAULT_HTTP_RECEIVE_TMP_FILENAME); // cleanup the file first (if exists)

    if (requestType > HttpRequestTypesMAX) {
        debugPrintLn(DEBUG_STR_ERROR "Unknown request type!");
        return 0;
    }

    // set server domain
    write("AT+UHTTP=0,");
    write(isValidIPv4(url) ? "0,\"" : "1,\"");
    write(url);
    writeLn("\"");
    if (readResponse() != ResponseOK) {
        return 0;
    }

    // set port
    if (port != 80) {
        write("AT+UHTTP=0,5,");
        writeLn(static_cast<uint32_t>(port));

        if (readResponse() != ResponseOK) {
            return 0;
        }
    }

    // before starting the actual http request, create any files needed in the fs of the modem
    // that way there is a chance to abort sending the http req command in case of an fs error
    if (requestType == PUT || requestType == POST) {
        if (!sendBuffer || sendSize == 0) {
            debugPrintLn(DEBUG_STR_ERROR "There is no sendBuffer or sendSize set!");
            return 0;
        }

        deleteFile(DEFAULT_HTTP_SEND_TMP_FILENAME); // cleanup the file first (if exists)

        if (!writeFile(DEFAULT_HTTP_SEND_TMP_FILENAME, sendBuffer, sendSize)) {
            debugPrintLn(DEBUG_STR_ERROR "Could not create the http tmp file!");
            return 0;
        }
    }

    // reset the success bit before calling a new request
    _httpRequestSuccessBit[requestType] = TriBoolUndefined;

    write("AT+UHTTPC=0,");
    write(static_cast<uint8_t>(_httpRequestTypeToModemIndex(requestType)));
    write(",\"");
    write(endpoint);
    write("\",\"\""); // empty filename = default = "http_last_response_0" (DEFAULT_HTTP_RECEIVE_TMP_FILENAME)

    // NOTE: a file that includes the buffer to send has been created already
    if (requestType == PUT) {
        writeLn(",\"" DEFAULT_HTTP_SEND_TMP_FILENAME "\""); // param1: file from filesystem to send
    }
    else if (requestType == POST) {
        write(",\"" DEFAULT_HTTP_SEND_TMP_FILENAME "\""); // param1: file from filesystem to send
        writeLn(",1"); // param2: content type, 1=text/plain
        // TODO consider making the content type a parameter
    } else {
        writeLn("");
    }

    if (readResponse() != ResponseOK) {
        return 0;
    }

    // check for success while checking URCs
    uint32_t start = millis();
    while ((_httpRequestSuccessBit[requestType] == TriBoolUndefined) && !is_timedout(start, 30000)) {
        isAlive();
        delay(5);
    }

    if (_httpRequestSuccessBit[requestType] == TriBoolTrue) {
        if (responseBuffer && responseSize > 0) {
            return readFile(DEFAULT_HTTP_RECEIVE_TMP_FILENAME, responseBuffer, responseSize);
        }
    }
    else if (_httpRequestSuccessBit[requestType] == TriBoolFalse) {
        debugPrintLn(DEBUG_STR_ERROR "An error occured with the http request!");
        return 0;
    }
    else {
        debugPrintLn(DEBUG_STR_ERROR "Timed out waiting for a response for the http request!");
        return 0;
    }

    return 0;
}

ResponseTypes Sodaq_3Gbee::_ulstfileParser(ResponseTypes& response, const char* buffer, size_t size,
        uint32_t* filesize, uint8_t* dummy)
{
    if (!filesize) {
        return ResponseError;
    }

    if (sscanf(buffer, "+ULSTFILE: %lu", filesize) == 1) {
        return ResponseEmpty;
    }

    return ResponseError;
}

// maps the given requestType to the index the modem recognizes, -1 if error
int8_t Sodaq_3Gbee::_httpRequestTypeToModemIndex(HttpRequestTypes requestType)
{
    static uint8_t mapping[] = {
        4, // POST (0)
        1, // GET (1)
        0, // HEAD (2)
        2, // DELETE (3)
        3, // PUT (4)
    };

    return (requestType < sizeof(mapping)) ? mapping[requestType] : -1;
}

// HttpRequestTypes if successful, -1 if not
int8_t Sodaq_3Gbee::_httpModemIndexToRequestType(uint8_t modemIndex)
{
    static uint8_t mapping[] = {
        HEAD,
        GET,
        DELETE,
        PUT,
        POST,
    };

    return (modemIndex < sizeof(mapping)) ? mapping[modemIndex] : -1;
}

// no string termination
size_t Sodaq_3Gbee::readFile(const char* filename, char* buffer, size_t size)
{
    // TODO escape filename characters { '"', ',', }
    
    //sanity check
    if (!buffer || size == 0) {
        return 0;
    }
    
    // first, make sure the buffer is sufficient
    write("AT+ULSTFILE=2,\"");
    write(filename);
    writeLn("\"");

    uint32_t filesize = 0;

    if ((readResponse<uint32_t, uint8_t>(_ulstfileParser, &filesize, NULL) != ResponseOK) || (filesize > size)) {
        debugPrintLn(DEBUG_STR_ERROR "The buffer is not big enough to store the file or the file was not found!");

        return 0;
    }

    write("AT+URDFILE=\"");
    write(filename);
    writeLn("\"");

    // override normal parsing process and explicitly read characters here 
    // to be able to also read terminator characters within files
    char checkChar = 0;
    size_t len = 0;

    // reply identifier
    len = readBytesUntil(' ', _inputBuffer, _inputBufferSize);
    if (len == 0 || strstr(_inputBuffer, "+URDFILE:") == NULL) {
        debugPrintLn(DEBUG_STR_ERROR "+URDFILE literal is missing!");
        goto error;
    }

    // filename
    len = readBytesUntil(',', _inputBuffer, _inputBufferSize);
    // TODO check filename after removing quotes and escaping chars
    //if (len == 0 || strstr(_inputBuffer, filename)) {
    //    debugPrintLn(DEBUG_STR_ERROR "Filename reported back is not correct!");
    //    return 0;
    //}

    // filesize
    len = readBytesUntil(',', _inputBuffer, _inputBufferSize);
    filesize = 0; // reset the var before reading from reply string
    if (sscanf(_inputBuffer, "%lu", &filesize) != 1) {
        debugPrintLn(DEBUG_STR_ERROR "Could not parse the file size!");
        goto error;
    }
    if (filesize == 0 || filesize > size) {
        debugPrintLn(DEBUG_STR_ERROR "Size error!");
        goto error;
    }

    // opening quote character
    checkChar = timedRead();
    if (checkChar != '"') {
        debugPrintLn(DEBUG_STR_ERROR "Missing starting character (quote)!");
        goto error;
    }

    // actual file buffer, written directly to the provided result buffer
    len = readBytes(buffer, filesize);
    if (len != filesize) {
        debugPrintLn(DEBUG_STR_ERROR "File size error!");
        goto error;
    }

    // closing quote character
    checkChar = timedRead();
    if (checkChar != '"') {
        debugPrintLn(DEBUG_STR_ERROR "Missing termination character (quote)!");
        goto error;
    }

    // read final OK response from modem and return the filesize
    if (readResponse() == ResponseOK) {
        return filesize;
    }

error:
    return 0;
}

bool Sodaq_3Gbee::writeFile(const char* filename, const char* buffer, size_t size)
{
    // TODO escape filename characters
    write("AT+UDWNFILE=\"");
    write(filename);
    write("\",");
    writeLn(static_cast<uint32_t>(size));

    if (readResponse() == ResponsePrompt) {
        for (size_t i = 0; i < size; i++) {
            write(buffer[i]);
        }

        return (readResponse() == ResponseOK);
    }

    return false;
}

bool Sodaq_3Gbee::deleteFile(const char* filename)
{
    // TODO escape filename characters
    write("AT+UDELFILE=\"");
    write(filename);
    writeLn("\"");

    return (readResponse() == ResponseOK);
}

// Opens an FTP connection.
bool Sodaq_3Gbee::openFtpConnection(const char* server, const char* username, const char* password, FtpModes ftpMode)
{
    ftpDirectoryChangeCounter = 0;

    // set server
    write("AT+UFTP=");
    write(isValidIPv4(server) ? "0,\"" : "1,\"");
    write(server);
    writeLn("\"");
    
    if (readResponse() != ResponseOK) {
        return false;
    }

    // set username
    write("AT+UFTP=2,\"");
    write(username);
    writeLn("\"");

    if (readResponse() != ResponseOK) {
        return false;
    }

    // set password
    write("AT+UFTP=3,\"");
    write(password);
    writeLn("\"");

    if (readResponse() != ResponseOK) {
        return false;
    }

    // set passive / active
    write("AT+UFTP=6,");
    writeLn(static_cast<uint8_t>(ftpMode == ActiveMode ? 0 : 1));

    if (readResponse() != ResponseOK) {
        return false;
    }

    // connect
    writeLn("AT+UFTPC=1");

    if ((readResponse() != ResponseOK) || (!waitForFtpCommandResult(1))) {
        return false;
    }

    return true;
}

// Closes the FTP connection.
bool Sodaq_3Gbee::closeFtpConnection()
{
    ftpDirectoryChangeCounter = 0;

    writeLn("AT+UFTPC=0");

    if ((readResponse() != ResponseOK) || (!waitForFtpCommandResult(0))) {
        return false;
    }

    return true;
}

// Opens an FTP file for sending or receiving.
// filename should be limited to 256 characters (excl. null terminator)
// path should be limited to 512 characters (excl. null temrinator)
bool Sodaq_3Gbee::openFtpFile(const char* filename, const char* path)
{
    // keep the filename for subsequent calls to send or receive data
    strncpy(ftpFilename, filename, sizeof(ftpFilename)-1);
    ftpFilename[sizeof(ftpFilename) - 1] = 0; // always null terminated, even when given filename has length > sizeof(ftpFilename)-1

    resetFtpDirectoryIfNeeded();
    if (path) {
        char pathBuffer[512 + 1];
        strncpy(pathBuffer, path, sizeof(pathBuffer)-1);
        pathBuffer[sizeof(pathBuffer) - 1] = '\0'; // secure from overflow

        char* pathPointer = strtok(pathBuffer, "/");
        while (pathPointer != NULL)
        {
            if (!changeFtpDirectory(pathPointer)) {
                // TODO do something here?
            }
            pathPointer = strtok(NULL, "/");
        }
    }

    return true;
}

// Sends the given "buffer" to the (already) open FTP file.
// Returns true if successful.
// Fails immediatelly if there is no open FTP file.
bool Sodaq_3Gbee::ftpSend(const char* buffer, size_t size)
{
    // quick sanity check
    if (ftpFilename[0] == '\0') {
        return false;
    }

    deleteFile(DEFAULT_FTP_TMP_FILENAME); // cleanup

    if (!writeFile(DEFAULT_FTP_TMP_FILENAME, buffer, size)) {
        deleteFile(DEFAULT_FTP_TMP_FILENAME);
        return false;
    }

    write("AT+UFTPC=5,\"" DEFAULT_FTP_TMP_FILENAME "\",\"");
    write(ftpFilename);
    writeLn("\"");

    if ((readResponse() != ResponseOK) || (!waitForFtpCommandResult(5))) {
        return false;
    }

    return true;
}

// Fills the given "buffer" from the (already) open FTP file.
// Returns true if successful.
// Fails immediatelly if there is no open FTP file.
int Sodaq_3Gbee::ftpReceive(char* buffer, size_t size)
{
    // quick sanity check
    if (ftpFilename[0] == '\0') {
        return 0;
    }

    deleteFile(DEFAULT_FTP_TMP_FILENAME); // cleanup

    write("AT+UFTPC=4,\"");
    write(ftpFilename);
    writeLn("\",\"" DEFAULT_FTP_TMP_FILENAME "\"");

    if ((readResponse() != ResponseOK) || (!waitForFtpCommandResult(4))) {
        return 0;
    }

    return readFile(DEFAULT_FTP_TMP_FILENAME, buffer, size);
}

// Closes the open FTP file.
// Returns true if successful.
// Fails immediatelly if there is no open FTP file.
bool Sodaq_3Gbee::closeFtpFile()
{
    resetFtpDirectoryIfNeeded();
    ftpFilename[0] = '\0'; // invalidate the filename
    return true;
}

ResponseTypes Sodaq_3Gbee::_cmglParser(ResponseTypes& response, const char* buffer, size_t size,
        int* indexList, size_t* indexListSize)
{
    if (!indexList || !indexListSize || *indexListSize <= 0) {
        return ResponseError;
    }

    int index;
    if (sscanf(buffer, "+CMGL: %d,", &index) == 1) {
        *(indexList++) = index;
        (*indexListSize)--;

        return ResponseEmpty;
    }

    return ResponseError;
}

// TODO test
// Gets an SMS list according to the given filter and puts the indexes in the "indexList".
// Returns the number of indexes written to the list or -1 in case of error.
int Sodaq_3Gbee::getSmsList(const char* statusFilter, int* indexList, size_t size)
{
    write("AT+CMGL=\"");
    write(statusFilter);
    writeLn("\"");

    size_t sizeParam = size;
    if (readResponse<int, size_t>(_cmglParser, indexList, &sizeParam) == ResponseOK) {
        return size - sizeParam; // the parser method subtracts from the total size when adding an index in the list
    }

    return -1;
}

ResponseTypes Sodaq_3Gbee::_cmgrParser(ResponseTypes& response, const char* buffer, size_t size,
        char* phoneNumber, char* smsBuffer)
{
    if (!phoneNumber || !smsBuffer) {
        return ResponseError;
    }

    if (sscanf(buffer, "+CMGR: \"%*[^\"]\",\"%[^\"]", phoneNumber) == 1) {
        return ResponseEmpty;
    }
    else if ((buffer[size - 2] == '\r') && (buffer[size - 1] == '\n')) {
        memcpy(smsBuffer, buffer, size - 2);
        smsBuffer[size - 2] = '\0';

        return ResponseEmpty;
    }

    return ResponseError;
}

// TODO test
// Reads an SMS from the given index and writes it to the given buffer.
// Returns true if successful.
bool Sodaq_3Gbee::readSms(uint8_t index, char* phoneNumber, char* buffer, size_t size)
{
    write("AT+CMGR=");
    writeLn(index);

    return (readResponse<char, char>(_cmgrParser, phoneNumber, buffer) == ResponseOK);
}

// TODO test
// Deletes the SMS at the given index.
bool Sodaq_3Gbee::deleteSms(uint8_t index)
{
    write("AT+CMGD=");
    writeLn(index);

    return (readResponse() == ResponseOK);
}

// TODO test
// Sends a text-mode SMS.
// Expects a null-terminated buffer.
// Returns true if successful.
bool Sodaq_3Gbee::sendSms(const char* phoneNumber, const char* buffer)
{
    write("AT+CMGS=\"");
    write(phoneNumber);
    writeLn("\"");

    if (readResponse() == ResponsePrompt) {
        for (size_t i = 0; i < strlen(buffer); i++) {
            write(buffer[i]);
        }

        write(CTRL_Z);

        return (readResponse() == ResponseOK);
    }

    return false;
}


Sodaq_3GbeeOnOff::Sodaq_3GbeeOnOff()
{
    _vcc33Pin = -1;
    _onoffPin = -1;
    _statusPin = -1;
}

// Initializes the instance
void Sodaq_3GbeeOnOff::init(int vcc33Pin, int onoffPin, int statusPin)
{
    if (vcc33Pin >= 0) {
      _vcc33Pin = vcc33Pin;
      // First write the output value, and only then set the output mode.
      digitalWrite(_vcc33Pin, LOW);
      pinMode(_vcc33Pin, OUTPUT);
    }

    if (onoffPin >= 0) {
      _onoffPin = onoffPin;
      // First write the output value, and only then set the output mode.
      digitalWrite(_onoffPin, LOW);
      pinMode(_onoffPin, OUTPUT);
    }

    if (statusPin >= 0) {
      _statusPin = statusPin;
      pinMode(_statusPin, INPUT);
    }
}

void Sodaq_3GbeeOnOff::on()
{
    // First VCC 3.3 HIGH
    if (_vcc33Pin >= 0) {
        digitalWrite(_vcc33Pin, HIGH);
    }

    // Wait a little
    // TODO Figure out if this is really needed
    delay(2);
    if (_onoffPin >= 0) {
        digitalWrite(_onoffPin, HIGH);
    }
}

void Sodaq_3GbeeOnOff::off()
{
    if (_vcc33Pin >= 0) {
        digitalWrite(_vcc33Pin, LOW);
    }

    // The GPRSbee is switched off immediately
    if (_onoffPin >= 0) {
        digitalWrite(_onoffPin, LOW);
    }

    // Should be instant
    // Let's wait a little, but not too long
    delay(50);
}

bool Sodaq_3GbeeOnOff::isOn()
{
    if (_statusPin >= 0) {
        bool status = digitalRead(_statusPin);
        return status;
    }

    // No status pin. Let's assume it is on.
    return true;
}
