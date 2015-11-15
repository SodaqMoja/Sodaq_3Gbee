#include "Sodaq_3Gbee.h"

#define DEBUG

#define STR_AT "AT"
#define STR_RESPONSE_OK "OK"
#define STR_RESPONSE_ERROR "ERROR\r\n"
#define STR_RESPONSE_CME_ERROR "+CME ERROR:"
#define STR_RESPONSE_CMS_ERROR "+CMS ERROR:"
#define STR_RESPONSE_SOCKET_PROMPT "@"
#define STR_RESPONSE_SMS_PROMPT ">"
#define STR_RESPONSE_FILE_PROMPT ">"

#define DEBUG_STR_ERROR "[ERROR]: "

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#ifdef DEBUG
#define debugPrintLn(...) { if (this->_diagStream) this->_diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (this->_diagStream) this->_diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

#define BLOCK_TIMEOUT -1
#define NOW (uint32_t)millis()
#define TIMEOUT(x, ms) ((uint32_t)ms < (uint32_t)millis()-(uint32_t)x) 
#define DEFAULT_PROFILE "0"
#define HIGH_BAUDRATE 57600

Sodaq_3Gbee sodaq_3gbee;

bool startsWith(const char* pre, const char* str)
{
    return (strncmp(pre, str, strlen(pre)) == 0);
}

size_t Sodaq_3Gbee::readResponse(char* buffer, size_t size, ResponseTypes& response, CallbackMethodPtr parserMethod, void* callbackParameter, uint32_t timeout)
{
    response = ResponseTimeout;
    uint32_t from = NOW;

    do {
        int count = readLn(buffer, size, 0, 2000);
        
        if (count > 0) {

            debugPrint("[readResponse]: ");
            debugPrint(buffer);
            debugPrint(" (");
            debugPrint(count);
            debugPrintLn(")");

            // TODO: handle unsolicited codes here

            if (startsWith(STR_AT, buffer)) {
                continue; // skip echoed back command
            }
            else if (startsWith(STR_RESPONSE_OK, buffer)) {
                response = ResponseOK;
                return 0;
            }
            else if (startsWith(STR_RESPONSE_ERROR, buffer) || startsWith(STR_RESPONSE_CME_ERROR, buffer) || startsWith(STR_RESPONSE_CMS_ERROR, buffer)) {
                response = ResponseError;
            }
            else if (startsWith(STR_RESPONSE_SOCKET_PROMPT, buffer) || startsWith(STR_RESPONSE_SMS_PROMPT, buffer) || startsWith(STR_RESPONSE_FILE_PROMPT, buffer)) {
                response = ResponsePrompt;
            }
            else {
                response = ResponseNotFound;
            }

            if (parserMethod) {
                parserMethod(response, buffer, count, callbackParameter);
            }

        }

        delay(10);
    } while (!TIMEOUT(from, timeout));

    return 0;
}

size_t Sodaq_3Gbee::readResponse(char* buffer, size_t size, ResponseTypes& response)
{
    return readResponse(buffer, size, response, 0);
}

bool Sodaq_3Gbee::setSimPin(const char* simPin)
{
    write("AT+CPIN=\"");
    write(simPin);
    writeLn("\"");

    ResponseTypes response;
    readResponse(_inputBuffer, _inputBufferSize, response);

    return (response == ResponseOK);
}

bool Sodaq_3Gbee::isAlive()
{
    writeLn(STR_AT);
   
    ResponseTypes response;
    readResponse(_inputBuffer, _inputBufferSize, response);

    return (response == ResponseOK);
}

bool Sodaq_3Gbee::setAPN(const char* apn)
{
    write("AT+UPSD=" DEFAULT_PROFILE ",1,\"");
    write(apn);
    writeLn("\"");

    ResponseTypes response;
    readResponse(_inputBuffer, _inputBufferSize, response);

    return (response == ResponseOK);
}

bool Sodaq_3Gbee::setAPNUsername(const char* username)
{
    write("AT+UPSD=" DEFAULT_PROFILE ",2,\"");
    write(username);
    writeLn("\"");

    ResponseTypes response;
    readResponse(_inputBuffer, _inputBufferSize, response);

    return (response == ResponseOK);
}

bool Sodaq_3Gbee::setAPNPassword(const char* password)
{
    write("AT+UPSD=" DEFAULT_PROFILE ",3,\"");
    write(password);
    writeLn("\"");

    ResponseTypes response;
    readResponse(_inputBuffer, _inputBufferSize, response);

    return (response == ResponseOK);
}

bool Sodaq_3Gbee::init(Stream& stream, const char* simPin, const char* apn, const char* username, const char* password, AuthorizationTypes authorization)
{
    debugPrintLn("[init] started.");

    initBuffer(); // safe to call multiple times

    setModemStream(stream);

    if (_sd) {
        _sd->off();
        delay(250);
        _sd->on();
    }

    // wait for power up
    bool timeout = true;
    for (uint8_t i = 0; i < 10; i++) {
        if (isAlive()) {
            timeout = false;
            break;
        }
    }

    if (timeout) {
        debugPrintLn(DEBUG_STR_ERROR "No Reply from Modem");
        return false;
    }

    ResponseTypes response;

    // echo off
    writeLn("AT E0");
    for (uint8_t i = 0; i < 5; i++) {
        readResponse(_inputBuffer, _inputBufferSize, response);
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
        readResponse(_inputBuffer, _inputBufferSize, response);
        if (response != ResponseOK) {
            return false;
        }

        _baudRateChangeCallbackPtr(HIGH_BAUDRATE);
        delay(1000); // wait for eveyrhing to be stable again
    }

    // verbose error messages
    writeLn("AT+CMEE=2");
    readResponse(_inputBuffer, _inputBufferSize, response);
    if (response != ResponseOK) {
        return false;
    }

    // enable network identification LED
    writeLn("AT+UGPIOC=16,2");
    readResponse(_inputBuffer, _inputBufferSize, response);
    if (response != ResponseOK) {
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
    readResponse(_inputBuffer, _inputBufferSize, response);
    if (response != ResponseOK) {
        return false;
    }

    if (*apn) {
        if (!setAPN(apn)) {
            return false;
        }
    }

    if (*username) {
        if (!setAPNUsername(username)) {
            return false;
        }
    }

    if (*password) {
        if (!setAPNPassword(password)) {
            return false;
        }
    }

    // TODO check AT+URAT? to be 1,2 meaning prefer umts, fallback to gprs
    // TODO keep checking network registration until done

    return true;
}

bool Sodaq_3Gbee::join(const char* apn, const char* username, const char* password, AuthorizationTypes authorization)
{
    if (*apn) {
        if (!setAPN(apn)) {
            return false;
        }
    }

    if (*username) {
        if (!setAPNUsername(username)) {
            return false;
        }
    }

    if (*password) {
        if (!setAPNPassword(password)) {
            return false;
        }
    }

    ResponseTypes response;
    // DHCP
    writeLn("AT+UPSD=" DEFAULT_PROFILE ",7,\"0.0.0.0\"");
    readResponse(_inputBuffer, _inputBufferSize, response);
    if (response != ResponseOK) {
        return false;
    }

    // go through all authentication to check against the selected one or 
    // to autodetect (first successful)
    for (uint8_t i = NoAuthorization; i < AutoDetectAutorization; i++) {
        if ((authorization == AutoDetectAutorization) || (authorization == i)) {
            // Set Authentication
            write("AT+UPSD=" DEFAULT_PROFILE ",6,");
            writeLn(i);
            readResponse(_inputBuffer, _inputBufferSize, response);
            if (response != ResponseOK) {
                return false;
            }

            // connect using default profile
            writeLn("AT+UPSDA=" DEFAULT_PROFILE ",3");
            readResponse(_inputBuffer, _inputBufferSize, response, NULL, NULL, 200000);
            if (response == ResponseOK) {
                return true;
            }
        }
    }

    return false;
}

bool Sodaq_3Gbee::disconnect()
{
    return false;
    // TODO: implement UPSDA
}

NetworkRegistrationStatuses Sodaq_3Gbee::getNetworkStatus()
{
    return NoNetworkRegistrationStatus;
    // TODO: implement
}

NetworkTechnologies Sodaq_3Gbee::getNetworkTechnology()
{
    return UnknownNetworkTechnology;
    // TODO: implement
}

bool Sodaq_3Gbee::getRSSI(char* buffer, size_t size)
{
    return false;
    // TODO: implement AT+CSQ, +CSQ: %d,%d <rssi>,<quality> 
    // result: rssi !=99 && -113 + 2*rssi dBm
}

bool Sodaq_3Gbee::getBER(char* buffer, size_t size)
{
    char berValues[] = { 49, 43, 37, 25, 19, 13, 7, 0 }; // 3GPP TS 45.008 [20] subclause 8.2.4

    return false;
    // TODO: implement AT+CSQ, +CSQ: %d,%d <rssi>,<quality> 
    // result: quality != 99 && b < sizeof(berValues), berValues[quality]
}

bool Sodaq_3Gbee::getOperatorName(char* buffer, size_t size)
{
    return false;
    // TODO: implement AT+COPS, "+COPS: %*d,%*d,\"%[^\"]\",%d" last is 0: GSM, 2: UTRAN, 7: LTE
}

bool Sodaq_3Gbee::getMobileDirectoryNumber(char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::getIMEI(char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::getCCID(char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::getIMSI(char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::getMEID(char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

void Sodaq_3Gbee::_cpinParser(ResponseTypes& response, const char* buffer, size_t size, SimStatuses* parameter)
{
    if (response == ResponseError) {
        *parameter = SimMissing;
    } else {
        char status[16];
        if (sscanf(buffer, "+CPIN: %s", &status) == 1) {
            if (startsWith("READY", status)) {
                *parameter = SimReady;
            }
            else {
                *parameter = SimNeedsPin;
            }
        }
    }
}

SimStatuses Sodaq_3Gbee::getSimStatus()
{
    ResponseTypes response;
    SimStatuses simStatus;

    writeLn("AT+CPIN?");
    int count = readResponse(_inputBuffer, _inputBufferSize, response, _cpinParser, &simStatus);

    return simStatus;
}

IP_t Sodaq_3Gbee::getLocalIP()
{
    return 0;
    // TODO: implement AT+UPSND
}

IP_t Sodaq_3Gbee::getHostIP(const char* host)
{
    return 0;
    // TODO: implement AT+UDNSRN
}

int Sodaq_3Gbee::createSocket(Protocols protocol, uint16_t port)
{
    return 0;
    // TODO: implement
}

bool Sodaq_3Gbee::connectSocket(int socket, const char* host, int port)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::socketSend(int socket, const char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

size_t Sodaq_3Gbee::socketReceive(int socket, char* buffer, size_t size)
{
    return 0;
    // TODO: implement
}

bool Sodaq_3Gbee::closeSocket(int socket)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::freeSocket(int socket)
{
    return false;
    // TODO: implement
}

size_t Sodaq_3Gbee::httpRequest(const char* url, const char* buffer, size_t size, HttpRequestTypes requestType, char* responseBuffer, size_t responseSize)
{
    return 0;
    // TODOL implement
}

// ========

bool Sodaq_3Gbee::openFtpConnection(const char* server, const char* username, const char* password)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::closeFtpConnection()
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::openFtpFile(const char* filename, const char* path)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::ftpSend(const char* buffer)
{
    return false;
    // TODO: implement
}

int Sodaq_3Gbee::ftpReceive(char* buffer, size_t size)
{
    return 0;
    // TODO: implement
}

bool Sodaq_3Gbee::closeFtpFile()
{
    return false;
    // TODO: implement
}

int Sodaq_3Gbee::getSmsList(const char* statusFilter, int* indexList, size_t size)
{
    return 0;
    // TODO: implement
}

bool Sodaq_3Gbee::readSms(int index, char* phoneNumber, char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::deleteSms(int index)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::sendSms(const char* phoneNumber, const char* buffer)
{
    return false;
    // TODO: implement
}
