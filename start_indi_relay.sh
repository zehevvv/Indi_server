#!/bin/bash
source /home/israelf/venv_indi_pylibcamera/bin/activate
indiserver -v /home/israelf/Desktop/Indi_server/indi-altaz-driver/build/indi_altaz_arduino "pylibcamera Main"@192.168.5.1:7624
