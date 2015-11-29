#ifndef _SODAQ_GSM_MODEM_h
#define _SODAQ_GSM_MODEM_h

#include <Arduino.h>
#include <stdint.h>
#include <Stream.h>
#include "Sodaq_Switchable_Device.h"

#define CR "\r"
#define LF "\n"
#define CRLF "\r\n"

#define DEFAULT_TIMEOUT 1000

#define SODAQ_GSM_MODEM_DEFAULT_INPUT_BUFFER_SIZE 128

#ifndef SODAQ_GSM_TERMINATOR
#warning "SODAQ_GSM_TERMINATOR is not set"
#define SODAQ_GSM_TERMINATOR CRLF
#endif

typedef void (*BaudRateChangeCallbackPtr)(uint32_t newBaudrate);

// TODO handle Watchdog, also use a define to turn handling on/off

enum AuthorizationTypes {
    NoAuthorization = 0,
    PAP = 1,
    CHAP = 2,
    AutoDetectAutorization = 3,
};

enum NetworkRegistrationStatuses {
    UnknownNetworkRegistrationStatus,
    Denied,
    NoNetworkRegistrationStatus,
    Home,
    Roaming,
};

enum NetworkTechnologies {
    UnknownNetworkTechnology,
    GSM,
    EDGE,
    UTRAN,
    HSDPA,
    HSUPA,
    HSDPAHSUPA,
    LTE,
};

enum SimStatuses {
    SimStatusUnknown,
    SimMissing,
    SimNeedsPin,
    SimReady,
};

enum Protocols {
    TCP,
    UDP,
};

enum HttpRequestTypes {
    POST,
    GET,
    HEAD,
    DELETE,
    PUT,
};

enum ResponseTypes {
    ResponseNotFound = 0,
    ResponseOK = 1,
    ResponseError = 2,
    ResponsePrompt = 3,
    ResponseTimeout = 4,
    ResponseEmpty = 5,
};

typedef uint32_t IP_t;

#define DEFAULT_READ_MS 5000

#define NO_IP_ADDRESS ((IP_t)0)

#define IP_FORMAT "%d.%d.%d.%d"

#define IP_TO_TUPLE(x) (uint8_t)(((x) >> 24) & 0xFF), \
                       (uint8_t)(((x) >> 16) & 0xFF), \
                       (uint8_t)(((x) >> 8) & 0xFF), \
                       (uint8_t)(((x) >> 0) & 0xFF)

#define TUPLE_TO_IP(o1, o2, o3, o4) ((((IP_t)o1) << 24) | (((IP_t)o2) << 16) | \
                                     (((IP_t)o3) << 8) | (((IP_t)o4) << 0))

#define SOCKET_FAIL -1

class Sodaq_GSM_Modem {
protected:
    // The stream that communicates with the device.
    Stream* _modemStream;

    // The (optional) stream to show debug information.
    Stream* _diagStream;

    // The size of the input buffer. Equals SODAQ_GSM_MODEM_DEFAULT_INPUT_BUFFER_SIZE
    // by default or (optionally) a user-defined value when using USE_DYNAMIC_BUFFER.
    size_t _inputBufferSize;

    // Flag to make sure the buffers are not allocated more than once.
    bool _isBufferInitialized;
    char* _inputBuffer;

    SwitchableDevice* _sd;

    BaudRateChangeCallbackPtr _baudRateChangeCallbackPtr;

    // initializes the input buffer and makes sure it is only initialized once. Safe to call multiple times.
    void initBuffer();

    void setModemStream(Stream& stream);

    // Reads a line from the device stream into the "buffer" starting at the "start" position of the buffer.
    // Returns the number of bytes read.
    size_t readLn(char* buffer, size_t size, size_t start = 0, long timeout = DEFAULT_TIMEOUT);

    // Reads a line from the device stream into the input buffer.
    // Returns the number of bytes read.
    size_t readLn() { return readLn(_inputBuffer, _inputBufferSize); };

    size_t write(const char* buffer);
    size_t write(uint8_t value);
    size_t write(uint32_t value);
    size_t write(char value);

    // write with terminator
    size_t writeLn(const char* buffer);
    size_t writeLn(uint8_t value);
    size_t writeLn(uint32_t value);
    size_t writeLn(char value);

    virtual ResponseTypes readResponse(char* buffer, size_t size, size_t* outSize, uint32_t timeout = DEFAULT_READ_MS) = 0;
public:
    // Constructor
    Sodaq_GSM_Modem();

    void setSwitchableDevice(SwitchableDevice& sd) { _sd = &sd; }

    // Sets the optional "Diagnostics and Debug" stream.
    void setDiag(Stream& stream) { this->_diagStream = &stream; };

    // Sets the size of the input buffer.
    // Needs to be called before init().
    void setInputBufferSize(size_t value) { this->_inputBufferSize = value; };

    virtual bool isAlive() = 0;

    virtual bool init(Stream& stream, const char* simPin = NULL, const char* apn = NULL, const char* username = NULL,
                      const char* password = NULL, AuthorizationTypes authorization = AutoDetectAutorization) = 0;

    virtual uint32_t getDefaultBaudrate() = 0;
    void enableBaudrateChange(BaudRateChangeCallbackPtr callback) { _baudRateChangeCallbackPtr = callback; };

    virtual bool setAPN(const char* apn) = 0;
    virtual bool setAPNUsername(const char* username) = 0;
    virtual bool setAPNPassword(const char* password) = 0;

    virtual bool join(const char* apn = NULL, const char* username = NULL,
                      const char* password = NULL, AuthorizationTypes authorization = AutoDetectAutorization) = 0;

    virtual bool disconnect() = 0;

    virtual NetworkRegistrationStatuses getNetworkStatus() = 0;

    virtual NetworkTechnologies getNetworkTechnology() = 0;

    // Get the Received Signal Strength Indication and Bit Error Rate
    virtual bool getRSSIAndBER(int8_t* rssi, uint8_t* ber) = 0;

    // Get the Operator Name
    virtual bool getOperatorName(char* buffer, size_t size) = 0;

    // Get Mobile Directory Number
    virtual bool getMobileDirectoryNumber(char* buffer, size_t size) = 0;

    // Get International Mobile Equipment Identity
    virtual bool getIMEI(char* buffer, size_t size) = 0;

    // Get Integrated Circuit Card ID
    virtual bool getCCID(char* buffer, size_t size) = 0;

    // Get International Mobile Station Identity
    virtual bool getIMSI(char* buffer, size_t size) = 0;

    // Get SIM status
    virtual SimStatuses getSimStatus() = 0;

    // Get IP Address
    virtual IP_t getLocalIP() = 0;

    // Get Host IP
    virtual IP_t getHostIP(const char* host) = 0;

    // Sockets
    virtual int createSocket(Protocols protocol, uint16_t localPort = 0) = 0;
    virtual bool connectSocket(uint8_t socket, const char* host, uint16_t port) = 0;
    virtual bool socketSend(uint8_t socket, const char* buffer, size_t size) = 0;
    virtual size_t socketReceive(uint8_t socket, char* buffer, size_t size) = 0; // returns number of bytes set to buffer
    virtual bool closeSocket(uint8_t socket) = 0;

    // HTTP
    virtual size_t httpRequest(const char* url, const char* buffer, size_t size, HttpRequestTypes requestType = GET, char* responseBuffer = NULL, size_t responseSize = 0) = 0;

    // Ftp
    virtual bool openFtpConnection(const char* server, const char* username, const char* password) = 0;
    virtual bool closeFtpConnection() = 0;
    virtual bool openFtpFile(const char* filename, const char* path = NULL) = 0;
    virtual bool ftpSend(const char* buffer) = 0;
    virtual int ftpReceive(char* buffer, size_t size) = 0;
    virtual bool closeFtpFile() = 0;

    // Sms
    virtual int getSmsList(const char* statusFilter = "ALL", int* indexList = NULL, size_t size = 0) = 0;
    virtual bool readSms(uint8_t index, char* phoneNumber, char* buffer, size_t size) = 0;
    virtual bool deleteSms(uint8_t index) = 0;
    virtual bool sendSms(const char* phoneNumber, const char* buffer) = 0;
};

#endif
