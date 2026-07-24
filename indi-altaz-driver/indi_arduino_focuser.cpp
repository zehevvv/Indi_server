#include "indi_arduino_focuser.h"
#include "indi_altaz_arduino.h"

#include <cmath>

ArduinoFocuser::ArduinoFocuser(AltAzArduino *sharedMount) : m_mount(sharedMount)
{
    setVersion(1, 0);
    setSupportedConnections(CONNECTION_NONE);
    SetCapability(FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT | FOCUSER_CAN_REVERSE | FOCUSER_HAS_VARIABLE_SPEED);
}

const char *ArduinoFocuser::getDefaultName()
{
    return "AltAz Arduino Focuser";
}

bool ArduinoFocuser::initProperties()
{
    INDI::Focuser::initProperties();

    FocusSpeedN[0].min = 1;
    FocusSpeedN[0].max = 18; // firmware's FocusMotor::SetSpeed() clamps to this range (RPM)
    FocusSpeedN[0].value = m_speed;

    FocusRelPosN[0].min = 0;
    FocusRelPosN[0].max = 100000;
    FocusRelPosN[0].step = 100;
    FocusRelPosN[0].value = 1000;

    addAuxControls();

    return true;
}

bool ArduinoFocuser::Connect()
{
    if (!m_mount->isSerialConnected())
    {
        LOG_ERROR("The AltAz Arduino mount device must be connected first - the focuser shares its serial port.");
        return false;
    }

    SetTimer(POLL_MS);
    return true;
}

bool ArduinoFocuser::Disconnect()
{
    // Nothing to close here - the mount device owns the actual serial connection lifecycle.
    return true;
}

bool ArduinoFocuser::SetFocuserSpeed(int speed)
{
    if (speed < 1)
        speed = 1;
    else if (speed > 18)
        speed = 18;

    m_speed = speed;
    return true;
}

IPState ArduinoFocuser::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    bool outward = (dir == FOCUS_OUTWARD);
    if (m_reversed)
        outward = !outward;

    if (!m_mount->sendFocusCommand(std::string("a=") + (outward ? "1" : "0")))
        return IPS_ALERT;
    if (!m_mount->sendFocusCommand("b=" + std::to_string(m_speed)))
        return IPS_ALERT;
    if (!m_mount->sendFocusCommand("c=" + std::to_string(ticks)))
        return IPS_ALERT;

    // No position feedback from the firmware, so completion is estimated from steps/speed and
    // confirmed later in TimerHit() - not measured. +500ms covers the firmware's own ramp-up.
    double minutes = (ticks / STEPS_PER_REVOLUTION) / m_speed;
    long estimatedMs = static_cast<long>(minutes * 60.0 * 1000.0) + 500;
    m_moveDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(estimatedMs);
    m_moving = true;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s = IPS_BUSY;
    IDSetNumber(&FocusRelPosNP, nullptr);

    return IPS_BUSY;
}

bool ArduinoFocuser::ReverseFocuser(bool enabled)
{
    m_reversed = enabled;
    return true;
}

bool ArduinoFocuser::AbortFocuser()
{
    bool ok = m_mount->sendFocusCommand("c=0");
    if (m_moving)
        finishMove();
    return ok;
}

void ArduinoFocuser::finishMove()
{
    m_moving = false;
    FocusRelPosNP.s = IPS_OK;
    IDSetNumber(&FocusRelPosNP, nullptr);
}

void ArduinoFocuser::TimerHit()
{
    if (!isConnected())
        return;

    if (m_moving && std::chrono::steady_clock::now() >= m_moveDeadline)
        finishMove();

    SetTimer(POLL_MS);
}
