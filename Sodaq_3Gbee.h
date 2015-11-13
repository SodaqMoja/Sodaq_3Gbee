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

typedef size_t (*CallbackMethodPtr)(ResponseTypes response, const char* buffer, size_t size);

class Sodaq_3Gbee: public Sodaq_GSM_Modem {
public:
    bool isAlive() override;
    bool setAPN(const char* apn) override;
    bool setAPNUsername(const char* username) override;
    bool setAPNPassword(const char* password) override;

    bool init(Stream& stream, const char* simPin = NULL, const char* apn = NULL, const char* username = NULL, 
        const char* password = NULL, AuthorizationTypes authorization = AutoDetectAutorization) override;
    bool join(const char* apn = NULL, const char* username = NULL, const char* password = NULL, 
        AuthorizationTypes authorization = AutoDetectAutorization);
    bool disconnect() override;

    NetworkRegistrationStatuses getNetworkStatus() override;
    NetworkTechnologies getNetworkTechnology() override;

    bool getRSSI(char* buffer, size_t size) override;
    bool getBER(char* buffer, size_t size) override;
    bool getOperatorName(char* buffer, size_t size) override;
    bool getMobileDirectoryNumber(char* buffer, size_t size) override;
    bool getIMEI(char* buffer, size_t size) override;
    bool getCCID(char* buffer, size_t size) override;
    bool getIMSI(char* buffer, size_t size) override;
    bool getMEID(char* buffer, size_t size) override;

    IP_t getLocalIP() override;
    IP_t getHostIP(const char* host) override;

    int createSocket(Protocols protocol, uint16_t port) override;
    bool connectSocket(int socket, const char* host, int port) override;
    bool socketSend(int socket, const char* buffer, size_t size) override;
    size_t socketReceive(int socket, char* buffer, size_t size) override;
    bool closeSocket(int socket) override;
    bool freeSocket(int socket) override;

    size_t httpRequest(const char* url, const char* buffer, size_t size, HttpRequestTypes requestType = GET, char* responseBuffer = NULL, size_t responseSize = 0) override;

    bool openFtpConnection(const char* server, const char* username, const char* password) override;
    bool closeFtpConnection() override;
    bool openFtpFile(const char* filename, const char* path) override;
    bool ftpSend(const char* buffer) override;
    int ftpReceive(char* buffer, size_t size) override;
    bool closeFtpFile() override;
    int getSmsList(const char* statusFilter, int* indexList, size_t size) override;
    bool readSms(int index, char* phoneNumber, char* buffer, size_t size) override;
    bool deleteSms(int index) override;
    bool sendSms(const char* phoneNumber, const char* buffer) override;
protected:
    size_t readResponse(char* buffer, size_t size, ResponseTypes& response, CallbackMethodPtr parserMethod, long timeout = DEFAULT_READ_MS);
    size_t readResponse(char* buffer, size_t size, ResponseTypes& response) override;
private:
        size_t parseUnsolicitedCodes(char* buffer, size_t size);
};

extern Sodaq_3Gbee sodaq_3gbee;

#endif
