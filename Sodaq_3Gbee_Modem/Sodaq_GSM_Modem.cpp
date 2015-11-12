#include "Sodaq_GSM_Modem.h"

#define DEBUG

#ifdef DEBUG
#define debugPrintLn(...) { if (this->_diagStream) this->_diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (this->_diagStream) this->_diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

Sodaq_GSM_Modem::Sodaq_GSM_Modem() :
    _modemStream(0),
    _diagStream(0),
    _inputBufferSize(SODAQ_GSM_MODEM_DEFAULT_INPUT_BUFFER_SIZE),
    _inputBuffer(0),
    _sd(0)
{
    this->_isBufferInitialized = false;
}

// TODO is the result really needed?
size_t Sodaq_GSM_Modem::write(const char* buffer)
{
    debugPrint("[write] ");
    debugPrint(buffer);
    
    return _modemStream->print(buffer);
};

// TODO is the result really needed?
size_t Sodaq_GSM_Modem::writeLn(const char* buffer)
{
    size_t i = write(buffer);
    return i + write(SODAQ_GSM_TERMINATOR);
}

void Sodaq_GSM_Modem::initBuffer()
{
    debugPrint("[initBuffer]");

    // make sure the buffers are only initialized once
    if (!_isBufferInitialized) {
        this->_inputBuffer = static_cast<char*>(malloc(this->_inputBufferSize));

        _isBufferInitialized = true;
    }
}

// Reads a line from the device stream into the "buffer" and returns it starting at the "start" position of the buffer received, without the terminator.
// Returns the number of bytes read.
size_t Sodaq_GSM_Modem::readLn(char* buffer, size_t size, size_t start)
{
    // TODO custom implement readBytesUntil to distinguise timeout from empty string?
    // TODO custom implement readBytesUntil to watch out about buffer not being enough to reach terminator
    int len = this->_modemStream->readBytesUntil('\n', buffer + start, size);

    // TODO configurable terminator LF/CR vs CRLF
    this->_inputBuffer[start + len - 1] = 0; // bytes until \n always end with \r, so get rid of it (-1)

    return len;
}
