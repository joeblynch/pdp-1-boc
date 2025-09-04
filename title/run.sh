cat title.mac | ./ascii2fiodec -f > title.mac.fio
rm output.bin title.bin
./pdp1 title.ini
python3 strip.py
python3 dump.py