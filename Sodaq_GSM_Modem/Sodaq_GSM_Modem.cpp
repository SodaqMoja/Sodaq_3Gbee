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

// Constructor
Sodaq_GSM_Modem::Sodaq_GSM_Modem() :
    _modemStream(0),
    _diagStream(0),
    _inputBufferSize(SODAQ_GSM_MODEM_DEFAULT_INPUT_BUFFER_SIZE),
    _inputBuffer(0), 
    _timeout(DEFAULT_TIMEOUT),
    _onoff(0),
    _baudRateChangeCallbackPtr(0)
{
    this->_isBufferInitialized = false;
}

// Turns the modem on and returns true if successful.
bool Sodaq_GSM_Modem::on()
{
    if (!isOn()) {
        if (_onoff) {
            _onoff->on();
        }
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
        debugPrintLn("Error: No Reply from Modem");
        return false;
    }

    return isOn(); // this essentially means isOn() && isAlive()
}

// Turns the modem off and returns true if successful.
bool Sodaq_GSM_Modem::off() const
{
    // No matter if it is on or off, turn it off.
    if (_onoff) {
        _onoff->off();
    }

    return !isOn();
}

// Returns true if the modem is on.
bool Sodaq_GSM_Modem::isOn() const
{
    if (_onoff) {
        return _onoff->isOn();
    }

    // No onoff. Let's assume it is on.
    return true;
}

size_t Sodaq_GSM_Modem::write(const char* buffer)
{
    debugPrint("[write]");
    debugPrint(buffer);
    
    return _modemStream->print(buffer);
}

size_t Sodaq_GSM_Modem::write(uint8_t value)
{
    debugPrint("[write]");
    debugPrint(value);

    return _modemStream->print(value);
}

size_t Sodaq_GSM_Modem::write(uint32_t value)
{
    debugPrint("[write]");
    debugPrint(value);

    return _modemStream->print(value);
}

size_t Sodaq_GSM_Modem::write(char value)
{
    debugPrint("[write]");
    debugPrint(value);

    return _modemStream->print(value);
};

size_t Sodaq_GSM_Modem::writeLn(const char* buffer)
{
    size_t i = write(buffer);
    return i + write(SODAQ_GSM_TERMINATOR);
}

size_t Sodaq_GSM_Modem::writeLn(uint8_t value)
{
    size_t i = write(value);
    return i + write(SODAQ_GSM_TERMINATOR);
}

size_t Sodaq_GSM_Modem::writeLn(uint32_t value)
{
    size_t i = write(value);
    return i + write(SODAQ_GSM_TERMINATOR);
}

size_t Sodaq_GSM_Modem::writeLn(char value)
{
    size_t i = write(value);
    return i + write(SODAQ_GSM_TERMINATOR);
}

// Initializes the input buffer and makes sure it is only initialized once. 
// Safe to call multiple times.
void Sodaq_GSM_Modem::initBuffer()
{
    debugPrintLn("[initBuffer]");

    // make sure the buffers are only initialized once
    if (!_isBufferInitialized) {
        this->_inputBuffer = static_cast<char*>(malloc(this->_inputBufferSize));

        _isBufferInitialized = true;
    }
}

// Sets the modem stream.
void Sodaq_GSM_Modem::setModemStream(Stream& stream)
{
    this->_modemStream = &stream;
}

// Returns a character from the modem stream if read within _timeout ms or -1 otherwise.
int Sodaq_GSM_Modem::timedRead() const
{
    int c;
    uint32_t _startMillis = millis();
    
    do {
        c = _modemStream->read();
        if (c >= 0) {
            return c;
        }
    } while (millis() - _startMillis < _timeout);
    
    return -1; // -1 indicates timeout
}

// Fills the given "buffer" with characters read from the modem stream up to "length"
// maximum characters and until the "terminator" character is found or a character read
// times out (whichever happens first).
// The buffer does not contain the "terminator" character or a null terminator explicitly.
// Returns the number of characters written to the buffer, not including null terminator.
size_t Sodaq_GSM_Modem::readBytesUntil(char terminator, char* buffer, size_t length)
{
    if (length < 1) {
        return 0;
    }

    size_t index = 0;

    while (index < length) {
        int c = timedRead();

        if (c < 0 || c == terminator) {
            break;
        }

        *buffer++ = static_cast<char>(c);
        index++;
    }

    return index; // return number of characters, not including null terminator
}

// Fills the given "buffer" with up to "length" characters read from the modem stream.
// It stops when a character read timesout or "length" characters have been read.
// Returns the number of characters written to the buffer.
size_t Sodaq_GSM_Modem::readBytes(char* buffer, size_t length)
{
    size_t count = 0;

    while (count < length) {
        int c = timedRead();

        if (c < 0) {
            break;
        }
        
        *buffer++ = static_cast<char>(c);
        count++;
    }

    return count;
}

// Reads a line (up to the SODAQ_GSM_TERMINATOR) from the modem stream into the "buffer".
// The buffer is terminated with null.
// Returns the number of bytes read, not including the null terminator.
size_t Sodaq_GSM_Modem::readLn(char* buffer, size_t size, long timeout)
{
    _timeout = timeout;
    size_t len = readBytesUntil(SODAQ_GSM_TERMINATOR[SODAQ_GSM_TERMINATOR_LEN - 1], buffer, size);

    // check if the terminator is more than 1 characters, then check if the first character of it exists 
    // in the calculated position and terminate the string there
    if ((SODAQ_GSM_TERMINATOR_LEN > 1) && (_inputBuffer[len - (SODAQ_GSM_TERMINATOR_LEN - 1)] == SODAQ_GSM_TERMINATOR[0])) {
        len -= SODAQ_GSM_TERMINATOR_LEN - 1;
    }

    // terminate string
    _inputBuffer[len] = 0;

    return len;
}
