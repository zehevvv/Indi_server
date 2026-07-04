source /home/israelf/Desktop/Indi_server/.venv/bin/activate
rpicam-vid -t 0 --codec libav --libav-format mpegts --inline --low-latency --width 1280 --height 720 --framerate 30 -o "tcp://0.0.0.0:5001?listen=1"

# on the pc side run from powershell:
# ffplay tcp://192.168.4.1:5001 -fflags nobuffer -flags low_delay -framedrop