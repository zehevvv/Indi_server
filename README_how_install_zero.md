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

Set the USB Ethernet interface to use a shared IPv4 address and bring it up:

```bash
sudo nmcli connection modify "netplan-eth0" ipv4.method shared
sudo nmcli connection modify "netplan-eth0" ipv4.addresses 192.168.5.1/24
sudo nmcli connection up "netplan-eth0"
```

## 6. Install and configure `dnsmasq`

```bash
sudo apt update
sudo apt install dnsmasq -y
```

Edit the `dnsmasq` configuration file:

```bash
sudo nano /etc/dnsmasq.conf
```

Add the following lines:

```text
# Listen only on the USB OTG interface
interface=usb0
dhcp-range=192.168.5.10,192.168.5.50,255.255.255.0,12h
dhcp-option=option:router,192.168.5.1
```

## 7. Restart and enable `dnsmasq`

```bash
sudo systemctl restart dnsmasq
sudo systemctl enable dnsmasq
```

## Notes

- Make sure the USB interface is actually named `usb0` on your device.
- Adjust the DHCP range if needed for your network.
- If `netplan-eth0` is not found, verify the connection name with `nmcli connection show`.
