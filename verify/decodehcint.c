/*
 * decodehcint.c
 *
 * This program decodes a Harmony Compiler intermediate binary paper tape image.
 * Usage: ./decodehcint <file> (use '-' for stdin)
 * 
 * MIT License:
 * Copyright 2025 Joe Lynch <joeblynch@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// On 2024-01-05 Peter Samson mentioned the CHM PDP-1 CPU runs 6% slower than spec
#define CHM_PDP1_CPU_SPEED_MULTIPLIER 0.94
#define NOTES_BUFFER_SIZE 8192

// TODO: support minor keys too
char *NOTE_NAMES[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
char *REST_NAME = "r";
char *ARTICULATION_NAMES[] = { "normal", "quarter", "half", "staccato", "legato" };

typedef struct {
    uint8_t articulation;
    uint8_t triplet;
    uint8_t pitch;
    uint8_t duration;
    uint8_t note_duration;
    uint8_t note_pitch;
    uint8_t octave;
    uint8_t semi_tone;
    char *note_name;
} note_t;

uint32_t rpb(FILE *fp, uint32_t *gap_frames, uint32_t *inner_frames) {
    uint32_t word = 0;
    int c;

    *gap_frames = 0;
    *inner_frames = 0;

    for (int i = 0; i < 3;) {
        if ((c = fgetc(fp)) == EOF) return EOF;

        if (c & 0200) {
            // rbp skips lines without the 8th bit set, ignores 7th bit
            word = (word << 6) | (c & 077);
            i++;
        } else { // if (!c) { // NOTE: was treating only nulls as blank, to support "zigzag" include non-binary frames
            if (i) {
                (*inner_frames)++;
            } else {
                (*gap_frames)++;
            }
        }
    }

    return word;
}

uint32_t read_next_word(FILE *fp, uint32_t *word, uint32_t *word_count, uint32_t *gap_frames, uint8_t peek) {
    uint32_t inner_frames;
    uint32_t gap_frames_int;
    *word = rpb(fp, &gap_frames_int, &inner_frames);

    if (inner_frames) {
        fprintf(
            stderr,
            "ERROR: %d inner blank frame%s found in word %06o\n",
            inner_frames,
            inner_frames == 1 ? "" : "s",
            *word_count
        );
        exit(1);
    }

    if (gap_frames_int) printf("[%d blank frame%s]\n", gap_frames_int, gap_frames_int == 1 ? "" : "s");
    if (gap_frames)     *gap_frames = gap_frames_int;
    if (*word == EOF)   return EOF;

    if (!peek) {
        printf("%06o: %06o", *word_count, *word);
    }

    (*word_count)++;

    return 0;
}

uint32_t peek_gap(FILE *fp, uint32_t *gap_frames) {
    uint32_t word;
    uint32_t inner_frames;
    long current_pos;

    // save the current file position
    current_pos = ftell(fp);
    if (current_pos == -1) {
        perror("invalid file position while peeking gap");
        return EOF;
    }

    // read the next word to get the gap frames
    word = rpb(fp, gap_frames, &inner_frames);

    // restore the file position
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        perror("fseek");
        return EOF;
    }

    return word == EOF ? EOF : 0;
}

void parse_note(uint32_t word, note_t *note) {
    note->articulation = ((word >> 14) & 014) | ((word & 0060000) >> 13);
    note->triplet = (word & 0100000) >> 15;
    note->pitch = (word >> 7) & 077;
    note->duration = word & 0177;

    note->note_duration = 192 / (note->duration * (note->triplet ? 2 : 3));

    if (note->pitch > 1) {
        // note_pitch is the pitch above the 2 rest pitches, where 0 is C1
        note->note_pitch = note->pitch - 2;
        note->octave = note->note_pitch / 12 + 1;
        note->semi_tone = note->note_pitch % 12;
        note->note_name = NOTE_NAMES[note->semi_tone];
    } else {
        note->note_pitch = 0;
        note->octave = 0;
        note->semi_tone = 0;
        note->note_name = REST_NAME;
    }
}

uint32_t decode_tempo_quarter(uint32_t tempo) {
    // the documentation shows the tempo encoded as 1126/(m*f)
    // Ken Sumrall's hc_midimaker code shows the tempo encoded as 2861/(m*f)
    // Decoding of paper tape by Peter Samson on 2025-01-04 came out to 2859/(m*f)
    // For simplicity, we'll assume f = 1/4 (quarter note), and return m
    return 11436 / (tempo & 0077777);
}

uint32_t add_1s_complement(uint32_t a, uint32_t b) {
    uint32_t sum = a + b;
    // add the carry to the sum and mask off potential overflow
    return ((sum & 0777777) + (sum >> 18)) & 0777777;
}

void verify_checksum(uint32_t expected, uint32_t calculated) {
    if (expected == calculated) {
        puts("\tgood checksum");
    } else {
        printf("\tchecksum mismatch: expected: %06o, calculated: %06o\n", expected, calculated);
        exit(1);
    }
}

char* articulation_name(uint32_t articulation) {
    int articulation_index;
    switch (articulation) {
        case 0:
        case 1:
        case 2:
            articulation_index = articulation;
            break;
        case 4:
            articulation_index = 3;
            break;
        case 8:
            articulation_index = 4;
            break;
        default:
            fprintf(stderr, "ERROR: invalid articulation: %d\n", articulation);
            exit(1);
    }
    return ARTICULATION_NAMES[articulation_index];
}

int read_notes(FILE *fp, uint32_t *word_count, uint32_t *notes, uint32_t *notes_count) {
    uint32_t word;
    uint32_t checksum = 0;
    uint32_t part_word_count = 0;
    uint32_t total_word_count;
    uint32_t tempo;
    note_t note;

    puts("NOTES:");

    while (1) {
        if (read_next_word(fp, &word, word_count, NULL, 0) == EOF) {
            // we should never hit EOF in the notes section, as the bar section should always follow
            return EOF;
        }

        part_word_count++;

        if (part_word_count > 1 && part_word_count < total_word_count + 2) {
            // add the checksum for all note words, excluding the word count and checksum itself
            checksum = add_1s_complement(checksum, word);
            // word count is 2 higher, since we skip the word count, and increment the word count above
            notes[part_word_count - 2] = word;
        }

        if (part_word_count == 1) {
            total_word_count = word;
            *notes_count = total_word_count - 1;    // exclude the checksum word
            printf("\tnotes word count: %d\n", total_word_count);
        } else if (part_word_count == total_word_count + 2) {
            verify_checksum(word, checksum);
            break;
        } else if (word == 0600000) {
            puts("\t/");
        } else if ((word & 0700000) == 0700000) {
            tempo = decode_tempo_quarter(word);
            printf(
                "\ttempo: %d BPM [%d BPM for CHM PDP-1] (assuming 4/4 time) [raw: %d]\n",
                tempo,
                (int)(tempo * CHM_PDP1_CPU_SPEED_MULTIPLIER),
                word & 0077777
            );
        } else {
            parse_note(word, &note);

            if (note.pitch > 1) {
                printf(
                    "\tarticulation: %02o [%s], triplet: %o [%s], ",
                    note.articulation,
                    articulation_name(note.articulation),
                    note.triplet,
                    note.triplet ? "Y" : "N"
                );
            } else {
                printf("\t");
            }

            printf(
                "pitch: %02o [%s%d], duration: %03o [1/%d]\n",
                note.pitch,
                note.note_name,
                note.octave,
                note.duration,
                note.note_duration
            );
        }
    }

    return 0;
}

int read_bars(FILE *fp, uint32_t *word_count, uint32_t *notes, uint32_t notes_count) {
    uint32_t word;
    uint32_t checksum = 0;
    uint32_t gap_frames;
    uint32_t part_word_count = 0;
    uint32_t total_word_count;
    uint8_t tempo;
    note_t note;

    puts("\nBARS:");

    while (1) {
        if (read_next_word(fp, &word, word_count, &gap_frames, 0) == EOF) {
            return EOF;
        }

        part_word_count++;

        if (part_word_count > 1 && part_word_count < total_word_count + 2) {
            // add the checksum for all note words, excluding the word count and checksum itself
            checksum = add_1s_complement(checksum, word);
        }

        if (part_word_count == 1) {
            if (!gap_frames) {
                perror("ERROR: bars part must have blank frames between preceeding notes part");
                exit(1);
            }

            total_word_count = word;
            printf("\tbars word count: %d\n", total_word_count);
        } else if (part_word_count == total_word_count + 2) {
            verify_checksum(word, checksum);

            // peek the next word to see if there are any more voices
            if (peek_gap(fp, &gap_frames) == EOF) {
                if (gap_frames) {
                    printf("[%d blank frame%s]\n", gap_frames, gap_frames == 1 ? "" : "s");
                }
                return EOF;
            }

            // on to the next voice
            break;
        } else if (word == 0600000) {
            printf("\t/\n");
            if (part_word_count != total_word_count + 1) {
                perror("ERROR: found end of bars word (600000) before end of bars word count");
                exit(1);
            }
        } else {
            if (word >= notes_count) {
                fprintf(stderr, "ERROR: note index %d out of range\n", word);
                exit(1);
            }

            // print measure number
            printf("\t%d", part_word_count - 1);

            uint32_t *measure_note = &notes[word];
            uint32_t note_count = 0;
            while (*measure_note != 0600000 && (word + note_count) < notes_count) {
                parse_note(*measure_note, &note);
                printf(" %st%d", note.note_name, note.note_duration);

                measure_note++;
                note_count++;
            }

            printf("/\n");
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file> (use '-' for stdin)\n", argv[0]);
        return 1;
    }

    FILE *fp;
    if (strcmp(argv[1], "-") == 0) {
        fp = stdin;
    } else {
        fp = fopen(argv[1], "rb");
        if (!fp) {
            fprintf(stderr, "could not open file %s\n", argv[1]);
            return 1;
        }
    }

    uint32_t word_count = 0;
    uint32_t notes_count = 0;
    uint8_t voice = 1;

    uint32_t *notes = malloc(NOTES_BUFFER_SIZE * sizeof(uint32_t));
    if (!notes) {
        fprintf(stderr, "could not allocate %lu bytes for notes buffer\n", NOTES_BUFFER_SIZE * sizeof(uint32_t));
        return 1;
    }

    while (voice <= 4) {
        if (voice > 1) puts("\n");
        printf("╔═════════════╗\n");
        printf("║   VOICE %d   ║\n", voice);
        printf("╚═════════════╝\n");

        if (read_notes(fp, &word_count, notes, &notes_count) == EOF) {
            perror("EOF in notes section\n");
            return 1;
        }

        if (read_bars(fp, &word_count, notes, notes_count) == EOF) {
            break;
        }
        voice++;
    }

    printf("\nDATA LENGTH: %dB\n", (int)ceil((float)word_count * 18.0f / 8.0f));

    free(notes);
    if (strcmp(argv[1], "-")) {
        fclose(fp);
    }
    
    return 0;
}
