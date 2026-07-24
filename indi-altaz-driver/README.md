# indi-altaz-driver

Custom INDI telescope driver for the Alt-Az mount: an Arduino Nano
([TelescopeWatcher_arduino_side](https://github.com/zehevvv/TelescopeWatcher_arduino_side))
drives two stepper motors (altitude, azimuth) via DRV8825 drivers, connected to the Pi 5 over
USB as `/dev/ttyACM0`.

Runs alongside the existing camera relay driver in the same `indiserver` process
(see `start_indi_relay.sh`), so both show up in KStars/Ekos over the one connection to
`192.168.4.1:7624`.

## Scope (phase 1)

Manual motion control only - no GOTO, Sync, Park, or tracking. `TELESCOPE_CAN_ABORT` is the
only capability set, so Ekos's Mount tab shows the directional pad (N/S/W/E + 4 slew-rate
presets) and an Abort button. There are no encoders or limit switches on this hardware, so
position is not tracked - `ReadScopeStatus()` reports a fixed placeholder coordinate.

## Arduino serial protocol (as implemented by the firmware)

115200 baud, line-based (`\n`-terminated), `NAME=VALUE` format. Axis commands are routed by a
shared selector rather than addressed per-line:

| Command | Meaning |
|---|---|
| `v=0` / `v=1` | Route subsequent `d/s/t/m/e` commands to the alt (up/down) or az (right/left) motor |
| `d=0\|1` | Direction |
| `s=<N>` | Queue N step pulses (this starts motion; `s=0` stops it) |
| `t=<ms>` | Time per step pulse (speed) |
| `m=<1\|2\|4\|8\|16>` | Microstep resolution (pin pattern for `16` is the DRV8825's actual max-microstep setting on this hardware) |
| `e=<0\|1>` | Keep the driver enabled (holding torque) vs. auto-disable when idle |

No position feedback, no acknowledgment protocol - the firmware only echoes human-readable
debug text. "Continuous move while held" is faked in the driver by queuing a large step count
(`MAX_STEP_QUEUE`) and cutting it short with `s=0` on release.

Motor: 1.8°/step (200 steps/rev), DRV8825 at max microstep (16, physically 1/32 on this
hardware), 1:625 gear reduction -> 2,000,000 microsteps/axis-revolution (~0.65 arcsec
resolution). Tested stall ceiling is ~100,000 steps/sec; slew-rate presets in
`indi_altaz_arduino.h` stay under that with margin and are not calibrated to real sky rates
(no encoders to calibrate against) - adjust `PULSE_US_*` constants to taste.

## Build

```
sudo apt-get install libnova-dev   # if not already installed
mkdir -p build && cd build
cmake ..
make
```

Produces `build/indi_altaz_arduino`. `start_indi_relay.sh` references this build path
directly rather than an installed copy.
