# INDI Server
To run the server manually:
1. Run `./install_env.sh` to install all dependencies and set up the virtual environment.
2. Run `./start_indi.sh` to launch the INDI server.
## Running on Boot with Auto-Restart (systemd service)
To install a systemd service that starts the INDI server automatically on boot and auto-restarts it if it goes down:
1. Make the script executable: `chmod +x setup_service.sh`
2. Run it: `./setup_service.sh`
This script dynamically generates `/etc/systemd/system/indi-server.service`, reloads systemd, enables the service to launch at startup, and starts it immediately.
## Real-time Video Streaming
If you want to stream real-time video, run `./start_stream.sh`. You can find the corresponding `ffplay` command inside that file to run on the Windows client side.

