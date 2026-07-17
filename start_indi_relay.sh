#!/bin/bash
source /home/israelf/venv_indi_pylibcamera/bin/activate
indiserver -v "pylibcamera Main"@192.168.5.1:7624
