# indi-altaz-driver

Custom INDI driver(s) for the Arduino-controlled telescope hardware
([TelescopeWatcher_arduino_side](https://github.com/zehevvv/TelescopeWatcher_arduino_side)):
one Arduino Nano drives two Alt-Az stepper motors via DRV8825 drivers and a 28BYJ-48 focus
motor, all over a single USB serial connection (`/dev/ttyACM0` on the Pi 5).

Runs alongside the existing camera relay driver in the same `indiserver` process
(see `start_indi_relay.sh`), so all devices show up in KStars/Ekos over the one connection to
`192.168.4.1:7624`.

## Two INDI devices, one process, one serial port

The driver binary (`indi_altaz_arduino`) exposes **two** INDI devices from a single process:

- **AltAz Arduino** (`AltAzArduino`, `INDI::Telescope`) - owns the actual serial connection.
- **AltAz Arduino Focuser** (`ArduinoFocuser`, `INDI::Focuser`) - shares it.

This is deliberate: only one process/file-descriptor can hold `/dev/ttyACM0` at a time (INDI's
serial connection plugin `flock()`s the port and refuses a second opener with "already used by
another driver or process"). Since the mount and focus motors are the same Arduino on the same
wire, a separate focuser driver process can't coexist with the mount driver process. Instead,
`ArduinoFocuser` uses `CONNECTION_NONE` (no port/baud UI of its own) and reuses the mount
device's already-open file descriptor via `AltAzArduino::sendFocusCommand()` /
`isSerialConnected()`. **The mount device must be connected first** - the focuser's `Connect()`
fails with an explanatory error otherwise. `main.cpp`-equivalent dispatch (`ISGetProperties`,
`ISNewSwitch/Number/Text`) in `indi_altaz_arduino.cpp` routes each incoming command to whichever
of the two device instances it's addressed to, by device name.

## Scope (phase 1)

**Mount:** manual motion control only - no GOTO, Sync, Park, or tracking. `TELESCOPE_CAN_ABORT`
+ `TELESCOPE_CAN_GOTO` are the only capabilities set (`CAN_GOTO` is required for INDI to even
build the motion-pad/slew-rate properties, even though `Goto()` itself just declines - see the
comment in the header). There are no encoders or limit switches on this hardware, so position
is not tracked - `ReadScopeStatus()` reports a fixed placeholder coordinate.

**Focuser:** relative-move only (`FOCUSER_CAN_REL_MOVE`), no absolute position (no encoders -
same reasoning as the mount). Also supports `FOCUSER_CAN_ABORT`, `FOCUSER_CAN_REVERSE`, and
`FOCUSER_HAS_VARIABLE_SPEED` (1-18 RPM, matching the firmware's range). Since the firmware gives
no completion feedback either, `MoveRelFocuser()` estimates move duration from
steps/speed/`STEPS_PER_REVOLUTION` and `ArduinoFocuser::TimerHit()` (polling every 200ms) flips
the property from `Busy` to `Ok` once that estimate elapses - not a measured completion.

## Arduino serial protocol (as implemented by the firmware)

115200 baud, line-based (`\n`-terminated), `NAME=VALUE` format. No position feedback, no
acknowledgment protocol - the firmware only echoes human-readable debug text.

**Mount axes** are routed by a shared selector rather than addressed per-line:

| Command | Meaning |
|---|---|
| `v=0` / `v=1` | Route subsequent `d/s/t/m/e` commands to the firmware's "up/down" or "right/left" motor |
| `d=0\|1` | Direction |
| `s=<N>` | Queue N step pulses (this starts motion; `s=0` stops it) |
| `t=<ms>` | Time per step pulse (speed) |
| `m=<1\|2\|4\|8\|16>` | Microstep resolution (pin pattern for `16` is the DRV8825's actual max-microstep setting on this hardware) |
| `e=<0\|1>` | Keep the driver enabled (holding torque) vs. auto-disable when idle |

**Important wiring gotcha (found empirically, not documented anywhere in the firmware):** the
firmware's variable names are misleading for the actual hardware in use. `v=0`
("`g_motor_up_down`" in the firmware) physically drives **azimuth**, and `v=1`
("`g_motor_right_left`") physically drives **altitude** - the reverse of what the names suggest.
`MoveNS`/`MoveWE` in `indi_altaz_arduino.cpp` account for this swap already; if the hardware is
ever rewired, that mapping (and the separate W/E direction inversion also found empirically)
will need revisiting.

"Continuous move while held" is faked in the driver by queuing a large step count
(`MAX_STEP_QUEUE`) and cutting it short with `s=0` on release.

Motor: 1.8°/step (200 steps/rev), DRV8825 at max microstep (16, physically 1/32 on this
hardware), 1:625 gear reduction -> 2,000,000 microsteps/axis-revolution (~0.65 arcsec
resolution). Tested stall ceiling is ~100,000 steps/sec; slew-rate presets in
`indi_altaz_arduino.h` (currently 10/100/1000/10000us pulse periods) are not calibrated to real
sky rates (no encoders to calibrate against) - adjust `PULSE_US_*` constants to taste.

**Focus motor** (28BYJ-48 via the Arduino `Stepper` library, 2048 steps/rev) is *not* gated by
`v` - these commands always address the focus motor directly regardless of axis selection:

| Command | Meaning |
|---|---|
| `a=0\|1` | Direction (0=counter-clockwise, 1=clockwise) |
| `b=<1-18>` | Target speed in RPM (firmware ramps up to this gradually, ~1 RPM per 20ms loop iteration) |
| `c=<N>` | Queue N steps to move (relative; `c=0` stops immediately) |

## Build

```
sudo apt-get install libnova-dev   # if not already installed
mkdir -p build && cd build
cmake ..
make
```

Produces `build/indi_altaz_arduino`. `start_indi_relay.sh` references this build path
directly rather than an installed copy.
