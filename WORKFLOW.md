## 0. Setup

- Python 3 and GCC required
- run `./build.sh` to build C files and install Python package dependencies

## 1. Transcribe Each Voice

- create "simplified Harmony Compiler" scores ([simulator/scores/](./simulator/scores))
- verify scores with simulator (run `python3 -m http.server` from [public/](./public))
- convert scores to Harmony Compiler DSL (voices/)

## 2. Use Harmony Compiler to Produce Intermediate Tape

- compile music with 1962 Harmony Compiler (`cd hc_binmaker;./update.sh;cd ..`)

See [hc_binmaker/README.md](hc_binmaker/README.md) for more information.

## 3. Verify Intermediate Tape

- decode and verify intermediate tape binary file (`./verify/decodehcint ./hc_binmaker/boc-olson.bin`)

## 4. Add Metadata to Tape Leader and Trailer

- create a MACRO assymbly file with title text to use in the tape leader ([title/title.mac](title/title.mac))
- use MACRO assembler to produce tape leader data (`cd title; ./run.sh;cd ..`)
- dump leader from MACRO to bitmap image (`cd imgbin;source .venv/bin/activate;python imgbin.py ../title/title.bin title.bmp;deactivate;cd ..`)
- manually edit and add any additional metadata to leader (any image editor supporting 1-bit BMP files to edit [imgbin/title.bmp](imgbin/title.bmp))
- create trailer pixel art as BMP ([imgbin/trailer.bmp](imgbin/trailer.bmp))
- covert leader and trailer BMP files to binary tape format (`cd imgbin;source .venv/bin/activate;python imgbin.py title.bmp title.bin;python imgbin.py trailer.bmp trailer.bin;deactivate;cd ..`)

## 5. Create the Paper Tape

- shorten blank tape gaps between vocies to save paper tape: (`./tweak/tweak hc_binmaker/boc-olson.bin output/boc-olson-full.bin`)
- inject leader and trailer binary tape segments into HC intermediate tape (`python3 title/replace.py --title imgbin/title.bin --trailer imgbin/trailer.bin --tape-in output/boc-olson-full.bin --tape-out output/boc-olson.bin;rm output/boc-olson-full.bin`)
- generate SVG of tape file for visual verification (`python3 verify/dumpsvg.py -o output/boc-olson.svg output/boc-olson.bin`)
- punch paper tape file to physical paper tape (output/boc-olson.bin, use CoolTerm with tape punch.CoolTermSettings)
- visually inspect the paper tape against the SVG (output/boc-olson.svg)

## 6. Play the Paper Tape on the PDP-1

- talk to one of us on the PDP-1 demo team at the Computer History Museum, Mountain View, California :-)
