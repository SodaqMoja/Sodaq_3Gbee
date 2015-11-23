#ifndef _SODAQ_3GBEE_h
#define _SODAQ_3GBEE_h

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <Stream.h>
#include "Sodaq_3Gbee_Modem/Sodaq_GSM_Modem.h"

// TODO this needs to be set in the compiler directives. Find something else to do
#define SODAQ_GSM_TERMINATOR CRLF

typedef ResponseTypes(*CallbackMethodPtr)(ResponseTypes& response, const char* buffer, size_t size, void* parameter, void* parameter2);

class Sodaq_3Gbee: public Sodaq_GSM_Modem {
public:
    bool isAlive();
    bool setAPN(const char* apn);
    bool setAPNUsername(const char* username);
    bool setAPNPassword(const char* password);

    uint32_t getDefaultBaudrate() { return 9600; };

    bool init(Stream& stream, const char* simPin = NULL, const char* apn = NULL, const char* username = NULL, 
        const char* password = NULL, AuthorizationTypes authorization = AutoDetectAutorization);
    bool join(const char* apn = NULL, const char* username = NULL, const char* password = NULL, 
        AuthorizationTypes authorization = AutoDetectAutorization);
    bool disconnect();

    NetworkRegistrationStatuses getNetworkStatus();
    NetworkTechnologies getNetworkTechnology();

    bool getRSSIAndBER(uint8_t* rssi, uint8_t* ber);
    bool getOperatorName(char* buffer, size_t size);
    bool getMobileDirectoryNumber(char* buffer, size_t size);
    bool getIMEI(char* buffer, size_t size);
    bool getCCID(char* buffer, size_t size);
    bool getIMSI(char* buffer, size_t size);
    bool getMEID(char* buffer, size_t size);
    SimStatuses getSimStatus();

    IP_t getLocalIP();
    IP_t getHostIP(const char* host);

    int createSocket(Protocols protocol, uint16_t localPort = 0);
    bool connectSocket(uint8_t socket, const char* host, uint16_t port);
    bool socketSend(uint8_t socket, const char* buffer, size_t size);
    size_t socketReceive(uint8_t socket, char* buffer, size_t size);
    bool closeSocket(uint8_t socket);

    size_t httpRequest(const char* url, const char* buffer, size_t size, HttpRequestTypes requestType = GET, char* responseBuffer = NULL, size_t responseSize = 0);

    bool openFtpConnection(const char* server, const char* username, const char* password);
    bool closeFtpConnection();
    bool openFtpFile(const char* filename, const char* path);
    bool ftpSend(const char* buffer);
    int ftpReceive(char* buffer, size_t size);
    bool closeFtpFile();
    
    int getSmsList(const char* statusFilter, int* indexList, size_t size);
    bool readSms(int index, char* phoneNumber, char* buffer, size_t size);
    bool deleteSms(int index);
    bool sendSms(const char* phoneNumber, const char* buffer);
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

    static bool startsWith(const char* pre, const char* str);
    static size_t ipToStirng(IP_t ip, char* buffer, size_t size);
    static bool isValidIPv4(const char* str);
    bool setSimPin(const char* simPin);
    bool isConnected(); // TODO move/refactor into Sodaq_GSM_Modem
    static ResponseTypes _cpinParser(ResponseTypes& response, const char* buffer, size_t size, SimStatuses* simStatusResult, uint8_t* dummy);
    static ResponseTypes _udnsrnParser(ResponseTypes& response, const char* buffer, size_t size, IP_t* ipResult, uint8_t* dummy);
    static ResponseTypes _upsndParser(ResponseTypes& response, const char* buffer, size_t size, IP_t* ipResult, uint8_t* dummy);
    static ResponseTypes _upsndParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* thirdParam, uint8_t* dummy);
    static ResponseTypes _usocrParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* socket, uint8_t* dummy);
    static ResponseTypes _usordParser(ResponseTypes& response, const char* buffer, size_t size, char* resultBuffer, uint8_t* dummy);
    static ResponseTypes _copsParser(ResponseTypes& response, const char* buffer, size_t size, char* operatorNameBuffer, size_t* operatorNameBufferSize);
    static ResponseTypes _copsParser(ResponseTypes& response, const char* buffer, size_t size, int* networkTechnology, uint8_t* dummy);
};

extern Sodaq_3Gbee sodaq_3gbee;

#endif
