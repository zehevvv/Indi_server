#include "indi_altaz_arduino.h"

#include <indicom.h>
#include <connectionplugins/connectionserial.h>

#include <memory>
#include <unistd.h>

static std::unique_ptr<AltAzArduino> altaz_arduino(new AltAzArduino());

AltAzArduino::AltAzArduino()
{
    setVersion(1, 0);
    // TELESCOPE_CAN_GOTO is required for INDI::Telescope to build the motion-pad and slew-rate
    // properties, even though Goto() itself is not implemented - see Goto() below.
    SetTelescopeCapability(TELESCOPE_CAN_ABORT | TELESCOPE_CAN_GOTO, 4);
    setTelescopeConnection(CONNECTION_SERIAL);
}

const char *AltAzArduino::getDefaultName()
{
    return "AltAz Arduino";
}

bool AltAzArduino::initProperties()
{
    INDI::Telescope::initProperties();

    serialConnection->setDefaultBaudRate(Connection::Serial::B_115200);
    serialConnection->setDefaultPort("/dev/ttyACM0");

    addAuxControls();

    return true;
}

bool AltAzArduino::Handshake()
{
    // The Nano resets when the port is opened (DTR toggling the auto-reset line), so give the
    // bootloader + firmware time to come back up before sending anything.
    usleep(2000000);
    drainInput();

    // There's no query/ack protocol on the firmware side (it only prints a "Start" banner once
    // at boot, plus per-command debug echo), so we can't verify the link beyond "port opened
    // and writes succeeded". Push our fixed startup config to both axes.
    bool ok = true;
    ok &= selectAxis(AXIS_ALT);
    ok &= sendLine("m=" + std::to_string(MICROSTEP_MODE));
    ok &= sendLine("e=0");
    ok &= selectAxis(AXIS_AZ);
    ok &= sendLine("m=" + std::to_string(MICROSTEP_MODE));
    ok &= sendLine("e=0");

    if (!ok)
        LOG_ERROR("Handshake: failed to write startup configuration to one or both axes");

    return ok;
}

bool AltAzArduino::ReadScopeStatus()
{
    // No encoders on this hardware - there is no real position to read back. Drain whatever
    // debug text the firmware has echoed so the input buffer doesn't grow unbounded, and report
    // a fixed placeholder position (GOTO/Sync are not supported, so this is display-only).
    drainInput();
    NewRaDec(0, 0);
    return true;
}

bool AltAzArduino::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    // Physically, the "up/down" (v=0) motor drives azimuth and the "right/left" (v=1) motor
    // drives altitude on this mount - the reverse of what the firmware's variable names suggest.
    // NS (altitude) commands are therefore routed to AXIS_AZ (v=1).
    if (command == MOTION_START)
    {
        bool reversed = ReverseMovementSP[REVERSE_NS].getState() == ISS_ON;
        bool up = (dir == DIRECTION_NORTH);
        if (reversed)
            up = !up;

        long pulseUs = pulseUsForRate(IUFindOnSwitchIndex(&SlewRateSP));
        return startAxis(AXIS_AZ, up ? 1 : 0, pulseUs);
    }

    return stopAxis(AXIS_AZ);
}

bool AltAzArduino::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    // See note in MoveNS: WE (azimuth) commands are routed to AXIS_ALT (v=0).
    if (command == MOTION_START)
    {
        bool reversed = ReverseMovementSP[REVERSE_WE].getState() == ISS_ON;
        bool right = (dir == DIRECTION_WEST);
        if (reversed)
            right = !right;

        long pulseUs = pulseUsForRate(IUFindOnSwitchIndex(&SlewRateSP));
        return startAxis(AXIS_ALT, right ? 1 : 0, pulseUs);
    }

    return stopAxis(AXIS_ALT);
}

bool AltAzArduino::Goto(double ra, double dec)
{
    INDI_UNUSED(ra);
    INDI_UNUSED(dec);
    LOG_WARN("This mount has no absolute positioning (no encoders) - GOTO is not supported. Use the directional motion controls instead.");
    return false;
}

bool AltAzArduino::Abort()
{
    bool ok1 = stopAxis(AXIS_ALT);
    bool ok2 = stopAxis(AXIS_AZ);
    return ok1 && ok2;
}

bool AltAzArduino::selectAxis(Axis axis)
{
    return sendLine(std::string("v=") + (axis == AXIS_ALT ? "0" : "1"));
}

bool AltAzArduino::startAxis(Axis axis, int direction, long pulseUs)
{
    if (!selectAxis(axis))
        return false;
    if (!sendLine("d=" + std::to_string(direction)))
        return false;

    // Firmware's "t" parameter is milliseconds per step pulse (internally multiplied by 1000 to
    // get microseconds), so sub-millisecond pulse periods are sent as fractional ms.
    char buf[32];
    snprintf(buf, sizeof(buf), "t=%.4f", pulseUs / 1000.0);
    if (!sendLine(buf))
        return false;

    return sendLine("s=" + std::to_string(MAX_STEP_QUEUE));
}

bool AltAzArduino::stopAxis(Axis axis)
{
    if (!selectAxis(axis))
        return false;
    return sendLine("s=0");
}

long AltAzArduino::pulseUsForRate(int rate) const
{
    switch (rate)
    {
        case SLEW_GUIDE:
            return PULSE_US_GUIDE;
        case SLEW_CENTERING:
            return PULSE_US_CENTERING;
        case SLEW_FIND:
            return PULSE_US_FIND;
        case SLEW_MAX:
        default:
            return PULSE_US_SLEW_MAX;
    }
}

bool AltAzArduino::sendLine(const std::string &line)
{
    std::string full = line + "\n";
    int nbytes_written = 0;
    int rc = tty_write_string(PortFD, full.c_str(), &nbytes_written);
    if (rc != TTY_OK)
    {
        char errmsg[256];
        tty_error_msg(rc, errmsg, sizeof(errmsg));
        LOGF_ERROR("Serial write error on '%s': %s", line.c_str(), errmsg);
        return false;
    }
    return true;
}

void AltAzArduino::drainInput()
{
    char buf[256];
    int nbytes_read = 0;
    while (tty_read(PortFD, buf, sizeof(buf) - 1, 1, &nbytes_read) == TTY_OK && nbytes_read > 0)
    {
        // discard - firmware only sends human-readable debug echo, no structured responses
    }
}

void ISGetProperties(const char *dev)
{
    altaz_arduino->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    altaz_arduino->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    altaz_arduino->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    altaz_arduino->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root)
{
    altaz_arduino->ISSnoopDevice(root);
}
