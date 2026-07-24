#pragma once

#include <inditelescope.h>

#include <string>

class AltAzArduino : public INDI::Telescope
{
    public:
        AltAzArduino();
        virtual ~AltAzArduino() = default;

        virtual const char *getDefaultName() override;
        virtual bool initProperties() override;

        // Shared-port access for AltAzFocuser, which drives the same Arduino's focus motor over
        // this same serial connection (only one process/fd can hold the port at a time, so the
        // focuser can't open its own connection - it piggybacks on this one).
        bool isSerialConnected() const
        {
            return PortFD >= 0;
        }
        bool sendFocusCommand(const std::string &line)
        {
            return sendLine(line);
        }

    protected:
        virtual bool Handshake() override;
        virtual bool ReadScopeStatus() override;
        virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
        virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
        virtual bool Abort() override;

        // INDI::Telescope only builds the motion-pad/slew-rate properties when
        // TELESCOPE_CAN_GOTO is set, so that capability bit is declared even though this driver
        // has no real GOTO. Goto() itself just declines - see .cpp.
        virtual bool Goto(double ra, double dec) override;

    private:
        enum Axis { AXIS_ALT = 0, AXIS_AZ = 1 };

        bool sendLine(const std::string &line);
        void drainInput();
        bool selectAxis(Axis axis);
        bool startAxis(Axis axis, int direction, long pulseUs);
        bool stopAxis(Axis axis);
        long pulseUsForRate(int rate) const;

        // Large queued step count used to fake "run until told to stop" motion -
        // the firmware only supports queuing a fixed number of steps, not indefinite run.
        static constexpr long MAX_STEP_QUEUE = 5000000L;

        // Value sent as the firmware's "m" (microstep) parameter. The firmware labels this
        // constant MICROSTEP_SIXTEENTH_STEP, but its MS1/MS2/MS3 pin pattern (all HIGH) is the
        // max-microstep setting on the DRV8825 driver boards actually in use here.
        static constexpr int MICROSTEP_MODE = 16;

        // Pulse period presets in microseconds. With a 1.8deg/200-step motor, this microstep
        // setting, and a 1:625 gear reduction, 1 microstep = 0.00018deg at the axis.
        static constexpr long PULSE_US_GUIDE     = 10000; // 100 steps/s     (~0.018 deg/s at the axis)
        static constexpr long PULSE_US_CENTERING = 1000;  // 1,000 steps/s  (~0.18 deg/s)
        static constexpr long PULSE_US_FIND      = 100;   // 10,000 steps/s (~1.8 deg/s)
        static constexpr long PULSE_US_SLEW_MAX  = 10;    // 100,000 steps/s (~18 deg/s - the tested stall ceiling exactly, no margin)
};
