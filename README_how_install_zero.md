# Indi Server Setup Guide

This guide shows the steps needed to prepare a Raspberry Pi / Linux device for an INDIGO/INDI camera setup, configure USB networking, and enable DHCP for the USB OTG interface.

## 1. Install required packages

Run these commands first:

```bash
sudo apt update
sudo apt install python3-picamera2 python3-venv python3-pip indi-bin -y
```

## 2. Create and activate a Python virtual environment

```bash
python3 -m venv camera_venv --system-site-packages
source camera_venv/bin/activate
```

## 3. Install Python dependencies

```bash
python3 -m pip install indiweb indi_pylibcamera lxml astropy
```

## 4. Enable USB OTG gadget support

Edit the firmware configuration file:

```bash
sudo nano /boot/firmware/config.txt
```

Add the following line at the end of the file:

```text
dtoverlay=dwc2
dr_mode=peripheral
```

Then edit the kernel command line:

```bash
sudo nano /boot/firmware/cmdline.txt
```

Find the `rootwait` entry and insert this immediately after it:

```text
modules-load=dwc2,g_ether
```

## 5. Configure the USB network interface

Delete the old config (if exist)

```bash
sudo nmcli connection delete netplan-eth0
```

Set the USB Ethernet interface to use a shared IPv4 address and bring it up:

```bash
sudo nmcli connection add type ethernet \
  con-name "usb-gadget" \
  ifname usb0 \
  ipv4.method manual \
  ipv4.addresses 192.168.5.1/24 \
  ipv4.never-default yes \
  ipv6.method ignore \
  connection.autoconnect yes
sudo nmcli connection up "usb-gadget"
```

## 6. Install and configure `dnsmasq`

```bash
sudo apt update
sudo apt install dnsmasq -y
```

Edit the `dnsmasq` configuration file:

```bash
sudo nano /etc/dnsmasq.d/usb-gadget.conf
```

Add the following lines:

```text
# Listen only on the USB OTG interface
interface=usb0
bind-interfaces
listen-address=192.168.5.1
dhcp-range=192.168.5.10,192.168.5.50,255.255.255.0,12h
dhcp-option=option:router,192.168.5.1
dhcp-option=option:dns-server,192.168.5.1
```

## 7. Restart and enable `dnsmasq`

```bash
sudo systemctl restart dnsmasq
sudo systemctl enable --now dnsmasq
```

## 8. At rspi 5 (not the zero!) set the right driver
Sometimes the rspi 5 that use for relay do problem that it load the cdc-etr drvier and not the rndi, 
Create the next file:
```bash
sudo nano /etc/udev/rules.d/99-rndis-gadget.rules
```
Than copy this line:
```bash
ACTION=="add", ATTR{idVendor}=="0525", ATTR{idProduct}=="a4a2", ATTR{bConfigurationValue}="1"
```
Save the file and exit, than reload the rules
```bash
sudo udevadm control --reload-rules
```
Than reconnect the zero to the rspi that use for relay

## 9. At zero, install the indi server
Install git and clone the current repo:
```bash
sudo apt install git -y
git clone https://github.com/zehevvv/Indi_server.git
cd  Indi_server/
```
Install the enviroment and the service
```bash
./install_env.sh
./setup_service.sh
```
## 10. Install camera arducam462 (optionall)
Run this commands:
```bash
wget -O install_pivariety_pkgs.sh https://github.com/ArduCAM/Arducam-Pivariety-V4L2-Driver/releases/download/install_script/install_pivariety_pkgs.sh
chmod +x install_pivariety_pkgs.sh
./install_pivariety_pkgs.sh -p libcamera_dev
./install_pivariety_pkgs.sh -p libcamera_apps
```

Open configure file:
```bash
sudo nano /boot/firmware/config.txt
```
in the file:
1. set "camera_auto_detect=0"
2. Add dtoverlay=arducam-pivariety under the [all] section
exit the file and set reboot:
```bash
sudo reboot
```
Because the driver is a shit, we need to fix it
```bash
sudo cp /usr/share/libcamera/ipa/rpi/vc4/imx462.json \
  /usr/share/libcamera/ipa/rpi/vc4/arducam-pivariety.json

sudo cp /usr/share/libcamera/ipa/rpi/pisp/imx462.json \
  /usr/share/libcamera/ipa/rpi/pisp/arducam-pivariety.json

sudo systemctl restart indi-server
```

## 11. Auto-load saved camera config on connect (optional, risky)

By default, `indi_pylibcamera` only restores a saved configuration (`CONFIG_SAVE`/`CONFIG_LOAD` in
the INDI Control Panel - e.g. `CCD_FAST_TOGGLE`, gain, exposure format, frame ROI, etc.) when a
client explicitly presses "Load". It does **not** reapply the saved config automatically after a
reconnect or a reboot, so anything you configured and saved reverts to driver defaults every time
you connect unless you remember to click "Load" first.

This patch makes the driver auto-apply the last saved config (whatever profile `APPLY_CONFIG` is
set to, default `CONFIG1`) immediately on every successful camera connect, by reusing the same
code path the "Load" button already triggers.

**This step is optional** - skip it if you're fine manually pressing "Load" after each connect.

**It is also risky**, because it directly edits the installed third-party pip package file rather
than your own code:
- If `indi_pylibcamera` is ever reinstalled or upgraded (`pip install --upgrade indi_pylibcamera`,
  a fresh `camera_venv`, etc.), this edit is silently lost and needs to be reapplied by hand.
- It's not an upstream-supported behavior change - just a local hack, not a config flag.
- Consider proposing it as a PR to [scriptorron/indi_pylibcamera](https://github.com/scriptorron/indi_pylibcamera)
  instead, if you want this to survive upgrades properly.

Edit the **active** venv's copy (check `start_indi.sh` to confirm which `camera_venv` is actually
used - there can be more than one on disk):

```bash
nano ~/Indi_server/camera_venv/lib/python3.13/site-packages/indi_pylibcamera/indi_pylibcamera.py
```

Find `class ConnectionVector`'s `set_byClient` method and change:

```python
        if self.get_OnSwitches()[0] == "CONNECT":
            if self.parent.openCamera():
                self.state = IVectorState.OK
            else:
                self.state = IVectorState.ALERT
        else:
```

to:

```python
        if self.get_OnSwitches()[0] == "CONNECT":
            if self.parent.openCamera():
                self.state = IVectorState.OK
                # Auto-load the last saved configuration (local patch, not upstream default) -
                # otherwise settings like CCD_FAST_TOGGLE only come back if a client manually
                # presses "Load" in the Configuration section after every connect.
                self.parent.knownVectors["CONFIG_PROCESS"].set_byClient({"CONFIG_LOAD": ISwitchState.ON})
            else:
                self.state = IVectorState.ALERT
        else:
```

Then restart the service:

```bash
sudo systemctl restart indi-server
```

If the restart hangs in a "deactivating" state, the old `indi_pylibcamera` Python process may not
be shutting down cleanly (seen with libcamera resource cleanup) - check `ps aux | grep indi` for a
leftover process and `sudo kill -9 <pid>` it, then the service should start normally.

Verify it worked: connect the camera in the INDI Control Panel and confirm the log shows
`loading configuration from .../CONFIG1.json` right after `opening camera`, with no manual "Load"
click.

## Notes

- Make sure the USB interface is actually named `usb0` on your device.
- Adjust the DHCP range if needed for your network.
- If `usb-gadget` is not found, verify the connection name with `nmcli connection show`.
