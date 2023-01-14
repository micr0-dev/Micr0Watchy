chmod 0777 /dev/ttyUSB0
python /home/micr0byte/Downloads/esptool-4.4/esptool.py erase_flash
pio run -t upload