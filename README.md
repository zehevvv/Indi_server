# INDI Server

INDI server for a Raspberry Pi based astrophotography camera setup. A camera is attached
directly to a Raspberry Pi Zero, which runs this INDI server to expose it. Optionally, a
second Pi (e.g. a Raspberry Pi 5) can relay that same INDI server over another network link,
so a client (like KStars/Ekos) can connect without talking to the Zero directly.

## Quick Start

1. Run `./install_env.sh` to install dependencies and set up the virtual environment.
2. Run `./start_indi.sh` to launch the INDI server.

For a full walkthrough of setting up a Zero from scratch (USB networking, camera drivers, etc.),
see [README_how_install_zero.md](README_how_install_zero.md).

## Running on Boot with Auto-Restart (systemd service)

- **On the Pi with the camera attached**: `./setup_service.sh` installs a systemd service
  that runs `start_indi.sh` on boot and auto-restarts it if it goes down.
- **On a relay Pi** (chaining to a remote INDI server instead of a local camera):
  `./setup_service_relay.sh` installs the same kind of service, but for `start_indi_relay.sh`.

Both scripts generate the systemd unit, reload systemd, enable it at boot, and start it immediately.

## Real-time Video Streaming

To stream real-time video, run `./start_stream.sh`. The matching `ffplay` command to run on
the client side is inside that file.
