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

## Notes

- Make sure the USB interface is actually named `usb0` on your device.
- Adjust the DHCP range if needed for your network.
- If `netplan-eth0` is not found, verify the connection name with `nmcli connection show`.
