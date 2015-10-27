// SODAQ_UbloxModem.h

#ifndef _SODAQ_UBLOXMODEM_h
#define _SODAQ_UBLOXMODEM_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class SODAQ_UbloxModemClass
{
 protected:


 public:
	void init();
};

extern SODAQ_UbloxModemClass SODAQ_UbloxModem;

#endif

