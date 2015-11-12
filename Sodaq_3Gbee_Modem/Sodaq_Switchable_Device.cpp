#include "Sodaq_Switchable_Device.h"

SwitchableDevice::SwitchableDevice()
{
    clearSwitchMethods();
}

void SwitchableDevice::setOnMethod(voidFuncPtr onMethod)
{
    _onMethod = onMethod;
}

void SwitchableDevice::setOffMethod(voidFuncPtr offMethod)
{
    _offMethod = offMethod;
}

void SwitchableDevice::setSwitchMethods(voidFuncPtr onMethod, voidFuncPtr offMethod)
{
    _onMethod = onMethod;
    _offMethod = offMethod;
}

void SwitchableDevice::clearSwitchMethods()
{
    _onMethod = _offMethod = 0;
}

void SwitchableDevice::on()
{
    if (_onMethod != 0) {
        _onMethod();
    }
}

void SwitchableDevice::off()
{
    if (_offMethod != 0) {
        _offMethod();
    }
}
