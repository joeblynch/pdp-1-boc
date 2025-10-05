/*
 * tweak.c
 *
 * This program modifies a Harmony Compiler intermediate binary paper tape image to change inter-voice gaps and/or tempo
 * Usage: ./tweak <input file> (use '-' for stdin) <output file> (use '-' for stdout) <tempo> <gap length> (default: 18)
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

// On 2024-01-05 Peter Samson mentioned the CHM PDP-1 CPU runs 6% slower than spec
#define NOTES_BUFFER_SIZE 8192

#define DEFAULT_GAP_LENGTH 18
#define INNER_VOICE_GAP_LENGTH 6

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

void ppb(FILE *fp, uint32_t word) {
    putc(0200 | ((word & 0770000) >> 12), fp);
    putc(0200 | ((word & 0007700) >> 6), fp);
    putc(0200 | ((word & 0000077)), fp);
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

    // if (gap_frames_int) printf("[%d blank frame%s]\n", gap_frames_int, gap_frames_int == 1 ? "" : "s");
    if (gap_frames)     *gap_frames = gap_frames_int;
    if (*word == EOF)   return EOF;

    if (!peek) {
        // printf("%06o: %06o", *word_count, *word);
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

uint32_t add_1s_complement(uint32_t a, uint32_t b) {
    uint32_t sum = a + b;
    // add the carry to the sum and mask off potential overflow
    return ((sum & 0777777) + (sum >> 18)) & 0777777;
}

void verify_checksum(uint32_t expected, uint32_t calculated) {
    if (expected == calculated) {
        puts("good checksum");
    } else {
        printf("checksum mismatch: expected: %06o, calculated: %06o\n", expected, calculated);
        exit(1);
    }
}

int copy_notes(FILE *fp_in, FILE* fp_out, uint32_t tempo) {
    uint32_t word;
    uint32_t checksum = 0;
    uint32_t part_word_count = 0;
    uint32_t total_word_count;
    uint32_t word_count = 0;
    uint32_t new_checksum = 0;

    while (1) {
        if (read_next_word(fp_in, &word, &word_count, NULL, 0) == EOF) {
            // we should never hit EOF in the notes section, as the bar section should always follow
            return EOF;
        }

        part_word_count++;

        if (part_word_count > 1 && part_word_count < total_word_count + 2) {
            // add the checksum for all note words, excluding the word count and checksum itself
            checksum = add_1s_complement(checksum, word);
        }

        uint8_t is_tempo = 0;

        if (part_word_count == 1) {
            total_word_count = word;
            // *notes_count = total_word_count - 1;    // exclude the checksum word
            // printf("\tnotes word count: %d\n", total_word_count);
        } else if (part_word_count == total_word_count + 2) {
            verify_checksum(word, checksum);
            ppb(fp_out, new_checksum);
            break;
        } else if ((word & 0700000) == 0700000 && tempo != -1) {
            uint32_t new_word = 0700000 | (tempo & 0077777);
            ppb(fp_out, new_word);
            new_checksum = add_1s_complement(new_checksum, new_word);
            is_tempo = 1;
            printf("tempo: %d -> %d\n", word & 0077777, tempo & 0077777);
        }

        if (is_tempo == 0) {
            ppb(fp_out, word);
            if (part_word_count > 1 && part_word_count < total_word_count + 2) {
                new_checksum = add_1s_complement(new_checksum, word);
            }
        }
    }

    printf("notes: %d words\n", word_count);

    return 0;
}

int copy_bars(FILE *fp_in, FILE *fp_out) {
    uint32_t word;
    uint32_t checksum = 0;
    uint32_t gap_frames;
    uint32_t part_word_count = 0;
    uint32_t total_word_count;
    uint8_t tempo;
    uint32_t word_count = 0;

    while (1) {
        if (read_next_word(fp_in, &word, &word_count, &gap_frames, 0) == EOF) {
            return EOF;
        }

        part_word_count++;

        if (part_word_count > 1 && part_word_count < total_word_count + 2) {
            // add the checksum for all note words, excluding the word count and checksum itself
            checksum = add_1s_complement(checksum, word);
        }

        if (part_word_count == 1) {
            if (!gap_frames) {
                perror("ERROR: bars part must have blank frames between preceding notes part");
                exit(1);
            }

            total_word_count = word;
            // printf("\tbars word count: %d\n", total_word_count);
        } else if (part_word_count == total_word_count + 2) {
            verify_checksum(word, checksum);
            ppb(fp_out, word);

            // peek the next word to see if there are any more voices
            if (peek_gap(fp_in, &gap_frames) == EOF) {
                return EOF;
            }

            // on to the next voice
            break;
        }

        ppb(fp_out, word);
    }

    printf("bars: %d words\n", word_count);

    return 0;
}

void write_gap(FILE *fp_out, int length) {
    for (int i = 0; i < length; i++) {
        fputc(0, fp_out);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "Usage: %s <input file> (use '-' for stdin) <output file> (use '-' for stdout) <tempo> <gap length> (default: 18)\n", argv[0]);
        return 1;
    }

    FILE *fp_in;
    if (strcmp(argv[1], "-") == 0) {
        fp_in = stdin;
    } else {
        fp_in = fopen(argv[1], "rb");
        if (!fp_in) {
            fprintf(stderr, "could not open file %s\n", argv[1]);
            return 1;
        }
    }

    FILE *fp_out;
    if (strcmp(argv[2], "-") == 0) {
        fp_out = stdout;
    } else {
        fp_out = fopen(argv[2], "wb");
        if (!fp_out) {
            fprintf(stderr, "could not open file %s\n", argv[2]);
            return 1;
        }
    }

    uint32_t tempo = argc == 4 ? (unsigned)atoi(argv[3]) : -1;

    int gap_length;
    if (argc == 5) {
        gap_length = atoi(argv[4]);
    } else {
        gap_length = DEFAULT_GAP_LENGTH;
    }

    uint8_t voice = 1;

    uint32_t *notes = malloc(NOTES_BUFFER_SIZE * sizeof(uint32_t));
    if (!notes) {
        fprintf(stderr, "could not allocate %lu bytes for notes buffer\n", NOTES_BUFFER_SIZE * sizeof(uint32_t));
        return 1;
    }

    uint32_t gap_frames_in;
    if (peek_gap(fp_in, &gap_frames_in) == EOF) {
        perror("empty file?\n");
        return 1;
    }

    printf("[leader: %d frames]\n", gap_frames_in);
    write_gap(fp_out, gap_frames_in);

    while (voice <= 4) {
        if (voice > 1) puts("\n");
        printf("╔═════════════╗\n");
        printf("║   VOICE %d   ║\n", voice);
        printf("╚═════════════╝\n");

        if (copy_notes(fp_in, fp_out, tempo) == EOF) {
            perror("EOF in notes section\n");
            return 1;
        }

        uint32_t inner_gap_length;
        if (peek_gap(fp_in, &inner_gap_length) == EOF) {
            perror("missing bars\n");
            return 1;
        }

        write_gap(fp_out, inner_gap_length);
        printf("inner gap: %d frames\n", inner_gap_length);

        if (copy_bars(fp_in, fp_out) == EOF) {
            break;
        }

        write_gap(fp_out, gap_length);

        voice++;
    }

    uint32_t trailer_length;
    if (peek_gap(fp_in, &trailer_length) != EOF) {
        perror("unexpected data after voice 4\n");
        return 1;
    }

    write_gap(fp_out, trailer_length);
    printf("trailer: %d frames\n", trailer_length);

    free(notes);
    if (strcmp(argv[1], "-")) fclose(fp_in);
    if (strcmp(argv[2], "-")) fclose(fp_out);
    
    return 0;
}
