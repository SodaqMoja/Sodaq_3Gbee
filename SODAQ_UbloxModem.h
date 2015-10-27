// SODAQ_UbloxModem.h

#ifndef _SODAQ_UBLOXMODEM_h
#define _SODAQ_UBLOXMODEM_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "sodaq_gsm_modem/SODAQ_GSM_Modem.h"

class SODAQ_UbloxModem : public SODAQ_GSM_Modem
{
 protected:


 public:
	void init();
};

extern SODAQ_UbloxModem ubloxModem;

#endif

