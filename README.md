# PDP-1 Music

_Playing the 1998 song "Olson" by [Boards of Canada](https://en.wikipedia.org/wiki/Boards_of_Canada) through four light bulbs on the control panel of a [1959 DEC PDP-1 computer](https://en.wikipedia.org/wiki/PDP-1) using [Peter Samson](https://www.computerhistory.org/pdp-1/peter-samson/)'s 1962 [Harmony Compiler](https://computerhistory.org/blog/programming-in-harmony/) in 2025 at the [Computer History Museum](https://computerhistory.org/exhibits/pdp-1/) with 603 bytes of music data._

## Harmony Compiler Documentation
_The following documents were created and provided by Peter Samson._
### Creating Music for the PDP-1
- [Original documentation (version A)](./docs/MusicCompiler-a.pdf)
- [Original documentation (version B)](./docs/MusicCompiler-b.pdf)
- [Music creation workflow](./docs/music-workflow.pdf)
- [Harmony Compiler intermediate tape format](./docs/music_intermediate_format.pdf)

### Playing Music on the PDP-1
- [Music playing instructions](./docs/music_instr16.pdf)
- [Music player operation flowchart](./docs/music_flow16.pdf)

## Olson Music Playback
Audio is produced on the PDP-1 with a clever hack done by Peter Samson as a student at MIT in the early 1960s. The PDP-1 has six "program flags", which are 6 flip-flops wired to six light bulbs on the control panel. A CPU instruction provides the ability to turn these light bulbs on or off via software.

While these bulbs were originally intended to provide program status information to the computer operator, Peter repurposed four of these light bulbs into four square wave generators (or four 1-bit DACs, put another way), by turning the bulbs on and off at audio frequencies.

Four wires are attached to the signal lines for these light bulbs. Resistors are used to downmix these four signals into stereo audio channels and provide impedance matching into a standard stereo amplifier, and combined with capacitors to create low pass filters to cut out the buzz of the computer noise and soften the square waves.

The four light bulbs act as individual music voices. Each voice is transcribed separately using a custom DSL defined for the 1962 Harmony Compiler, and then merged into a single file which is then compiled by the original Harmony Compiler running on a PDP-1 emulator. The resulting paper tape file is then punched to physical paper tape using a tape punch, and then loaded into the real PDP-1 for music playback.

The four voices for Olson can be found in the [voices](./voices/) directory. I use one voice for the melody, and three for the bass drone chords.

## Additional music creation tools

This repo contains the tools that were created for creating music on the PDP-1, including both tools I created, tools created by others, and the original PDP-1 Harmony Compiler. My tooling is fairly hacked together, but may be of some use for others interested in creating their own music.

This software has been tested on macOS. Presumably it should work on Linux and other POSIX environments. For a quick run through the process of running this software, see [WORKFLOW.md](WORKFLOW.md).

## Working with non-standard tuning
The original composition of Olson is tuned about [50 cents above standard tuning](https://reverbmachine.com/blog/boards-of-canada-chord-theory-part-two/#:~:text=The%20original%20recording%20is%20approx.%2050%20cents%20sharper%20than%20concert%20tuning.). In other words, tuned half way between two semitones. (e.g. C and C#)

The Harmony Compiler and related music player is built to use standard tuning. However, through a fortunate fluke, the CPU of PDP-1 at the Computer History Museum runs approximately 6% slower than spec, producing audio frequencies at a 6% lower pitch. As a result, the transcription was transposed up one semitone (e.g. C becomes C#), which combined with this speed discrepancy comes quite close to the original tuning.

---
Personal non-commercial project to research and preserve digital music synthesis history.

Original composition of "Olson" © 1998 Boards of Canada (Marcus Eoin and Michael Sandison) / Warp Records.