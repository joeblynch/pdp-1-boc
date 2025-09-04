python3 ../util/merge.py 
cp ../voices/boc-olson.txt .
./ascii2fiodec -f <boc-olson.txt >boc-olson.fio
rm boc-olson.bin
./pdp1 hc1-4.ini
