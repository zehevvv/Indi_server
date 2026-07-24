#pragma once

#include <indifocuser.h>

#include <chrono>

class AltAzArduino;

class ArduinoFocuser : public INDI::Focuser
{
    public:
        explicit ArduinoFocuser(AltAzArduino *sharedMount);
        virtual ~ArduinoFocuser() = default;

        virtual const char *getDefaultName() override;
        virtual bool initProperties() override;
        virtual void TimerHit() override;

    protected:
        virtual bool Connect() override;
        virtual bool Disconnect() override;

        virtual bool SetFocuserSpeed(int speed) override;
        virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
        virtual bool ReverseFocuser(bool enabled) override;
        virtual bool AbortFocuser() override;

    private:
        void finishMove();

        // The mount device owns the actual serial connection (only one fd/process can hold the
        // port); this driver reuses it rather than opening its own.
        AltAzArduino *m_mount;

        int m_speed = 10; // RPM, matches firmware's SetSpeed() range of 1-18
        bool m_reversed = false;
        bool m_moving = false;
        std::chrono::steady_clock::time_point m_moveDeadline;

        // Firmware constant (28BYJ-48 stepper via the Arduino Stepper library) - needed to
        // estimate move duration for completion tracking, since the firmware gives no feedback.
        static constexpr double STEPS_PER_REVOLUTION = 2048.0;
        static constexpr int POLL_MS = 200;
};
