rm hc_binmaker/ascii2fiodec title/ascii2fiodec hc_binmaker/pdp1 title/pdp1

gcc -o hc_binmaker/ascii2fiodec hc_binmaker/src/ascii2fiodec.c
cp hc_binmaker/ascii2fiodec title/

unzip hc_binmaker/src/simhv36-1.zip -d hc_binmaker/src/simhv36-1
cd hc_binmaker/src/simhv36-1
mkdir BIN
make
cp BIN/pdp1 ../..
cd ../..
rm -rf src/simhv36-1
cp pdp1 ../title/pdp1
cd ..

cd imgbin
python3 -m venv .venv
source .venv/bin/activate
pip install pillow
deactivate
cd ..

gcc -o tweak/tweak tweak/tweak.c

gcc -o verify/decodehcint verify/decodehcint.c