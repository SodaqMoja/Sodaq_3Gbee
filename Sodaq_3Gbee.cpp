#include "Sodaq_3Gbee.h"

#define DEBUG

#define STR_AT "AT"

#define DEBUG_STR_ERROR "[ERROR]: "

#ifdef DEBUG
#define debugPrintLn(...) { if (this->_diagStream) this->_diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (this->_diagStream) this->_diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

Sodaq_3Gbee sodaq_3gbee;

size_t Sodaq_3Gbee::readResponse(char* buffer, size_t size, ResponseTypes& response)
{
    return 0;
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
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::setAPNUsername(const char* username)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::setAPNPassword(const char* password)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::init(Stream& stream, const char* simPin, const char* apn, const char* username, const char* password, AuthorizationTypes authorization)
{
    debugPrintLn("[init] started.");

    initBuffer(); // safe to call multiple times

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



    return true;
}

bool Sodaq_3Gbee::join(const char* apn, const char* username, const char* password, AuthorizationTypes authorization)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::disconnect()
{
    return false;
    // TODO: implement
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
    // TODO: implement
}

bool Sodaq_3Gbee::getBER(char* buffer, size_t size)
{
    return false;
    // TODO: implement
}

bool Sodaq_3Gbee::getOperatorName(char* buffer, size_t size)
{
    return false;
    // TODO: implement
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

IP_t Sodaq_3Gbee::getLocalIP()
{
    return 0;
    // TODO: implement
}

IP_t Sodaq_3Gbee::getHostIP(const char* host)
{
    return 0;
    // TODO: implement
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
