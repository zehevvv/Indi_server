#!/bin/bash
# Exit on error
set -e
# Get the absolute path of the directory containing this script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SERVICE_NAME="indi-server-relay"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
echo "Creating systemd service file at ${SERVICE_FILE}..."
# Create the systemd service file dynamically using the script's directory
sudo bash -c "cat <<EOF > ${SERVICE_FILE}
[Unit]
Description=INDI Server Relay Service
After=network.target
[Service]
Type=simple
WorkingDirectory=${DIR}
ExecStart=/bin/bash ${DIR}/start_indi_relay.sh
Restart=always
RestartSec=5
User=$(whoami)
[Install]
WantedBy=multi-user.target
EOF"
echo "Reloading systemd daemon..."
sudo systemctl daemon-reload
echo "Enabling ${SERVICE_NAME} service to start on boot..."
sudo systemctl enable ${SERVICE_NAME}
echo "Starting ${SERVICE_NAME} service..."
sudo systemctl start ${SERVICE_NAME}
echo "Service installation complete. Status:"
sudo systemctl status ${SERVICE_NAME}
