#ifndef _SODAQ_3GBEE_h
#define _SODAQ_3GBEE_h

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <Stream.h>
#include "Sodaq_3Gbee_Modem/Sodaq_GSM_Modem.h"

// TODO this needs to be set in the compiler directives. Find something else to do
#define SODAQ_GSM_TERMINATOR CRLF

#define DEFAULT_READ_MS 5000

typedef void (*CallbackMethodPtr)(ResponseTypes& response, const char* buffer, size_t size, void* parameter);

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

    bool getRSSI(char* buffer, size_t size);
    bool getBER(char* buffer, size_t size);
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
    size_t readResponse(char* buffer, size_t size, ResponseTypes& response, CallbackMethodPtr parserMethod, void* callbackParameter = NULL, uint32_t timeout = DEFAULT_READ_MS);

    template<class T>
    size_t readResponse(char* buffer, size_t size, ResponseTypes& response, 
        void(*parserMethod)(ResponseTypes& response, const char* buffer, size_t size, T* parameter), T* callbackParameter = NULL, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(buffer, size, response, (CallbackMethodPtr)parserMethod, (void*)callbackParameter, timeout);
    }

    size_t readResponse(char* buffer, size_t size, ResponseTypes& response);
private:
    uint16_t _socketPendingBytes[7]; // TODO add getter

    //size_t parseUnsolicitedCodes(char* buffer, size_t size);
    static bool startsWith(const char* pre, const char* str);
    static size_t ipToStirng(IP_t ip, char* buffer, size_t size);
    static bool isValidIPv4(const char* str);
    bool setSimPin(const char* simPin);
    bool isConnected(); // TODO move/refactor into Sodaq_GSM_Modem
    static void _cpinParser(ResponseTypes& response, const char* buffer, size_t size, SimStatuses* simStatusResult);
    static void _udnsrnParser(ResponseTypes& response, const char* buffer, size_t size, IP_t* ipResult);
    static void _upsndParser(ResponseTypes& response, const char* buffer, size_t size, IP_t* ipResult);
    static void _upsndParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* thirdParam);
    static void _usocrParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* socket);
    static void _usordParser(ResponseTypes& response, const char* buffer, size_t size, char* resultBuffer);
};

extern Sodaq_3Gbee sodaq_3gbee;

#endif
