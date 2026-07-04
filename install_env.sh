sudo apt update
sudo apt install python3-picamera2 python3-venv python3-pip indi-bin -y
python3 -m venv camera_venv --system-site-packages 
source camera_venv/bin/activate
python3 -m pip install indiweb indi_pylibcamera lxml astropy