#ifndef _SODAQ_3GBEE_h
#define _SODAQ_3GBEE_h

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <Stream.h>
#include "Sodaq_GSM_Modem/Sodaq_GSM_Modem.h"

// TODO this needs to be set in the compiler directives. Find something else to do
#define SODAQ_GSM_TERMINATOR CRLF

enum TriBoolStates
{
    TriBoolFalse,
    TriBoolTrue,
    TriBoolUndefined
};

typedef TriBoolStates tribool_t;

typedef ResponseTypes(*CallbackMethodPtr)(ResponseTypes& response, const char* buffer, size_t size, void* parameter, void* parameter2);

class Sodaq_3Gbee: public Sodaq_GSM_Modem {
public:
    bool isAlive();
    bool setAPN(const char* apn, const char* username, const char* password);

    uint32_t getDefaultBaudrate() { return 9600; };

    void init(Stream& stream, int8_t vcc33Pin, int8_t onoffPin, int8_t statusPin);
    bool join(const char* simPin, const char* apn, const char* username, const char* password,
        AuthorizationTypes authorization = AutoDetectAutorization);
    bool disconnect();

    NetworkRegistrationStatuses getNetworkStatus();
    NetworkTechnologies getNetworkTechnology();

    bool getRSSIAndBER(int8_t* rssi, uint8_t* ber);
    bool getOperatorName(char* buffer, size_t size);
    bool getMobileDirectoryNumber(char* buffer, size_t size);
    bool getIMEI(char* buffer, size_t size);
    bool getCCID(char* buffer, size_t size);
    bool getIMSI(char* buffer, size_t size);
    SimStatuses getSimStatus();

    IP_t getLocalIP();
    IP_t getHostIP(const char* host);

    int createSocket(Protocols protocol, uint16_t localPort = 0);
    bool connectSocket(uint8_t socket, const char* host, uint16_t port);
    bool socketSend(uint8_t socket, const char* buffer, size_t size);
    size_t socketReceive(uint8_t socket, char* buffer, size_t size);
    bool closeSocket(uint8_t socket);
    void waitForSocketToCloseByRemote(uint8_t socket);

    size_t httpRequest(const char* url, uint16_t port, const char* endpoint, HttpRequestTypes requestType = GET, char* responseBuffer = NULL, size_t responseSize = 0, const char* sendBuffer = NULL, size_t sendSize = 0);

    bool openFtpConnection(const char* server, const char* username, const char* password, FtpModes ftpMode);
    bool closeFtpConnection();
    bool openFtpFile(const char* filename, const char* path = NULL);
    bool ftpSend(const char* buffer, size_t size);
    int ftpReceive(char* buffer, size_t size);
    bool closeFtpFile();
    
    int getSmsList(const char* statusFilter, int* indexList, size_t size);
    bool readSms(uint8_t index, char* phoneNumber, char* buffer, size_t size);
    bool deleteSms(uint8_t index);
    bool sendSms(const char* phoneNumber, const char* buffer);

    size_t readFile(const char* filename, char* buffer, size_t size);
    bool writeFile(const char* filename, const char* buffer, size_t size);
    bool deleteFile(const char* filename);
protected:
    ResponseTypes readResponse(char* buffer, size_t size, CallbackMethodPtr parserMethod,
        void* callbackParameter, void* callbackParameter2 = NULL, size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS);

    // override
    ResponseTypes readResponse(char* buffer, size_t size, size_t* outSize, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
    };
    
    ResponseTypes readResponse(size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
    };

    ResponseTypes readResponse(CallbackMethodPtr parserMethod, void* callbackParameter,
        void* callbackParameter2 = NULL, size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, parserMethod, callbackParameter, callbackParameter2, outSize, timeout);
    };

    template<typename T1, typename T2>
    ResponseTypes readResponse(ResponseTypes(*parserMethod)(ResponseTypes& response, const char* parseBuffer, size_t size, T1* parameter, T2* parameter2),
        T1* callbackParameter, T2* callbackParameter2,
        size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, (CallbackMethodPtr)parserMethod, 
            (void*)callbackParameter, (void*)callbackParameter2, outSize, timeout);
    };
private:
    uint16_t _socketPendingBytes[7]; // TODO add getter
    tribool_t _httpRequestSuccessBit[HttpRequestTypesMAX];
    bool _socketClosedBit[7];
    uint8_t ftpCommandURC[2];
    char ftpFilename[256 + 1]; // always null terminated
    uint8_t ftpDirectoryChangeCounter; // counts how many nested directories were changed, to revert on close

    static bool startsWith(const char* pre, const char* str);
    static size_t ipToString(IP_t ip, char* buffer, size_t size);
    static bool isValidIPv4(const char* str);
    bool setSimPin(const char* simPin);
    bool isConnected(); // TODO move/refactor into Sodaq_GSM_Modem

    // returns true if URC returns 1, false in case URC returns 0 or in case of timeout
    bool waitForFtpCommandResult(uint8_t ftpCommandIndex);
    bool changeFtpDirectory(const char* directory);
    void resetFtpDirectoryIfNeeded();

    void cleanupTempFiles();
    
    static ResponseTypes _cpinParser(ResponseTypes& response, const char* buffer, size_t size, SimStatuses* simStatusResult, uint8_t* dummy);
    static ResponseTypes _udnsrnParser(ResponseTypes& response, const char* buffer, size_t size, IP_t* ipResult, uint8_t* dummy);
    static ResponseTypes _upsndParser(ResponseTypes& response, const char* buffer, size_t size, IP_t* ipResult, uint8_t* dummy);
    static ResponseTypes _upsndParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* thirdParam, uint8_t* dummy);
    static ResponseTypes _usocrParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* socket, uint8_t* dummy);
    static ResponseTypes _usordParser(ResponseTypes& response, const char* buffer, size_t size, char* resultBuffer, uint8_t* dummy);
    static ResponseTypes _copsParser(ResponseTypes& response, const char* buffer, size_t size, char* operatorNameBuffer, size_t* operatorNameBufferSize);
    static ResponseTypes _copsParser(ResponseTypes& response, const char* buffer, size_t size, int* networkTechnology, uint8_t* dummy);
    static ResponseTypes _csqParser(ResponseTypes& response, const char* buffer, size_t size, int* rssi, int* ber);
    static ResponseTypes _cnumParser(ResponseTypes& response, const char* buffer, size_t size, char* numberBuffer, size_t* numberBufferSize);
    static ResponseTypes _nakedStringParser(ResponseTypes& response, const char* buffer, size_t size, char* stringBuffer, size_t* stringBufferSize);
    static ResponseTypes _ccidParser(ResponseTypes& response, const char* buffer, size_t size, char* ccidBuffer, size_t* ccidBufferSize);
    static ResponseTypes _cregParser(ResponseTypes& response, const char* buffer, size_t size, int* networkStatus, uint8_t* dummy);
    static ResponseTypes _ulstfileParser(ResponseTypes& response, const char* buffer, size_t size, uint32_t* filesize, uint8_t* dummy);
    static ResponseTypes _cmgrParser(ResponseTypes& response, const char* buffer, size_t size, char* phoneNumber, char* smsBuffer);
    static ResponseTypes _cmglParser(ResponseTypes& response, const char* buffer, size_t size, int* indexList, size_t* indexListSize);

    static int8_t _httpRequestTypeToModemIndex(HttpRequestTypes requestType);
    static int8_t _httpModemIndexToRequestType(uint8_t modemIndex);
};

extern Sodaq_3Gbee sodaq_3gbee;

#endif
