#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define OUTPUT 1
#define DEBUG 0
#define ERROR 1

#define PRINT_FILE(...) do {\
    fprintf(fout, __VA_ARGS__);\
    if (feof(fout)) {\
        PRINT_ERROR("ERROR: Could not print to output file: %s.\n",\
                    strerror(errno));\
    }\
} while (0)

#if OUTPUT
    #define PRINT_OUTPUT(...) do {\
        PRINT_FILE(__VA_ARGS__);\
        printf(__VA_ARGS__);\
    } while(0)
#else
    #define PRINT_OUTPUT(...) do {\
        PRINT_FILE(__VA_ARGS__);\
    } while(0)
#endif

#if DEBUG
    #define PRINT_DEBUG(...) printf(__VA_ARGS__)
    #define PRINT_ERROR(...) fprintf(stderr, __VA_ARGS__)
#else
    #define PRINT_DEBUG(...) (void)0

    #if ERROR
        #define PRINT_ERROR(...) fprintf(stderr, __VA_ARGS__)
    #else
        #define PRINT_ERROR(...) (void)0
    #endif
#endif


#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

#define SWAP(T, a, b) do {\
    T c = a;\
    a = b;\
    b = c;\
} while(0)

#define return_defer(ret) do {\
    result = ret;\
    goto defer;\
} while(0)

#define SIGN_CHAR(n) ((n < 0) ? '-' : '+')

#define ABS(n) ((n < 0) ? -n : n)

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t current_index;
} InstrBuffer;
int ib_from_file(const char *fp, InstrBuffer *ib);
int ib_readable(InstrBuffer ib);
uint8_t ib_read_byte(InstrBuffer *ib);

int decode_instructions_into_file(InstrBuffer ib, const char *fp);

// NOTE: Antepone the W bit to the register bits
char *reg_encodings[] = {
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di"
};

// NOTE: Antepone the MOD bits to the register bits
char *rm_encodings[] = {
    "[bx + si",
    "[bx + di",
    "[bp + si",
    "[bp + di",
    "[si",
    "[di",
    "[bp",
    "[bx",
};


int main(int argc, char **argv) {
    if (argc == 3) {
        PRINT_DEBUG("Input file: %s\n", argv[1]);
        PRINT_DEBUG("Output file: %s\n", argv[2]);
    } else {
        PRINT_ERROR("ERROR: Input and output file are needed!\n");
        return -1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    InstrBuffer ib;
    if (ib_from_file(input_path, &ib)) return 1;

    decode_instructions_into_file(ib, output_path);

    return 0;
}

int decode_instructions_into_file(InstrBuffer ib, const char *fp) {

    FILE *fout = NULL;
    int result = 0;

    fout = fopen(fp, "wb");
    if (fout == NULL) {
        PRINT_ERROR("ERROR: Could not open file %s: %s.\n",
                    fp, strerror(errno));
        return_defer(errno);
    }

    PRINT_OUTPUT("bits 16\n\n");

    while(ib_readable(ib)) {
        uint8_t b1 = ib_read_byte(&ib);

        uint8_t b1h = (b1 & 0b1111 << 4) >> 4;

        if (b1h == 0b1000) {
            if ((b1 & 0b11<<2) >> 2 == 0b10) {
                // NOTE: Register/memory to/from register
                uint8_t b2 = ib_read_byte(&ib);

                uint8_t d = (b1 & 0b10) >> 1;
                PRINT_DEBUG("\tD: 0x%hhx\n", d);
                uint8_t w = b1 & 1;
                PRINT_DEBUG("\tW: 0x%hhx\n", w);
                uint8_t mod = (b2 & (0b11 << 6)) >> 6;
                PRINT_DEBUG("\tMod: 0x%hhx\n", mod);
                uint8_t reg = (b2 & (0b111 << 3)) >> 3;
                PRINT_DEBUG("\tReg: 0x%hhx\n", reg);
                uint8_t rm = b2 & 0b111;
                PRINT_DEBUG("\tR/M: 0x%hhx\n", rm);

                if (mod == 0b00) {
                    // NOTE: Memory mode, no displacement except for rm = 0b110
                    char *dst = reg_encodings[w << 3 | reg];
                    if (rm == 0b110) {
                        // NOTE: Special case
                        uint16_t d16 = ib_read_byte(&ib) |
                                       ib_read_byte(&ib) << 8;
                        if (d == 1) { // NOTE: Check direction
                            PRINT_OUTPUT("mov %s, [%d]\n", dst, d16);
                        } else {
                            PRINT_OUTPUT("mov [%d], %s\n", d16, dst);
                        }
                    } else {
                        // NOTE: Standard case
                        char *src = rm_encodings[rm];

                        if (d == 1) {
                            PRINT_OUTPUT("mov %s, %s]\n", dst, src);
                        } else {
                            PRINT_OUTPUT("mov %s], %s\n", src, dst);
                        }
                    }

                } else if (mod == 0b01) {
                    // NOTE: Memory mode, 8-bit displacement follows
                    char *dst = reg_encodings[w << 3 | reg];
                    char *src = rm_encodings[rm];
                    int8_t d8 = (int8_t) ib_read_byte(&ib);

                    if (d8 != 0) {
                        if (d == 1) {
                            PRINT_OUTPUT("mov %s, %s %c %hhi]\n",
                                         dst, src, SIGN_CHAR(d8), ABS(d8));
                        } else {
                            PRINT_OUTPUT("mov %s %c %hhi], %s\n",
                                         src, SIGN_CHAR(d8), ABS(d8), dst);
                        }
                    } else { // when the displacement is 0, do not display it
                        if (d == 1) {
                            PRINT_OUTPUT("mov %s, %s]\n", dst, src);
                        } else {
                            PRINT_OUTPUT("mov %s], %s\n", src, dst);
                        }
                    }

                } else if (mod == 0b10) {
                    // NOTE: Memory mode, 16-bit displacement follows
                    char *dst = reg_encodings[w << 3 | reg];
                    char *src = rm_encodings[rm];
                    int16_t d16 = (int16_t) (ib_read_byte(&ib) |
                                             ib_read_byte(&ib) << 8);

                    if (d == 1) {
                        PRINT_OUTPUT("mov %s, %s %c %hi]\n",
                                     dst, src, SIGN_CHAR(d16), ABS(d16));
                    } else {
                        PRINT_OUTPUT("mov %s %c %hi], %s\n",
                                     src, SIGN_CHAR(d16), ABS(d16), dst);
                    }

                } else if (mod == 0b11) {
                    // NOTE: Register mode (no displacement)
                    char *dst = reg_encodings[w << 3 | reg];
                    char *src = reg_encodings[w << 3 | rm];

                    if (d == 0) SWAP(char *, dst, src);

                    PRINT_OUTPUT("mov %s, %s\n", dst, src);
                }

            } else {
                PRINT_ERROR("This opcode is not implemented!");
                // assert(0 && "This opcode is not implemented!");
            }

        } else if (b1h == 0b1100) {
            // NOTE: Immediate to register/memory
            if ((b1 & 0b111 << 1) >> 1 == 0b011) {
                uint8_t b2 = ib_read_byte(&ib);

                uint8_t w = b1 & 1;
                PRINT_DEBUG("\tW: 0x%hhx\n", w);
                uint8_t mod = (b2 & (0b11 << 6)) >> 6;
                PRINT_DEBUG("\tMod: 0x%hhx\n", mod);
                uint8_t rm = b2 & 0b111;
                PRINT_DEBUG("\tR/M: 0x%hhx\n", rm);
                // TODO: Finish to implement, this is part of listing 40
                if (mod == 0b00) {
                    // NOTE: Memory mode, no displacement except for rm = 0b110
                    if (rm == 0b110) {
                        // NOTE: Special case
                        uint16_t d16 = ib_read_byte(&ib) |
                                       ib_read_byte(&ib) << 8;
                        if (w == 1) {
                            int16_t data = (int16_t) (ib_read_byte(&ib) |
                                                      ib_read_byte(&ib) << 8);
                            PRINT_OUTPUT("mov [%d], word %hi\n", d16, data);
                        } else {
                            int8_t data = (int8_t) ib_read_byte(&ib);
                            PRINT_OUTPUT("mov [%d], byte %hhi\n", d16, data);
                        }
                    } else {
                        // NOTE: Standard case
                        char *src = rm_encodings[rm];

                        if (w == 1) {
                            int16_t data = (int16_t) (ib_read_byte(&ib) |
                                                      ib_read_byte(&ib) << 8);
                            PRINT_OUTPUT("mov %s], word %hi\n", src, data);
                        } else {
                            int8_t data = (int8_t) ib_read_byte(&ib);
                            PRINT_OUTPUT("mov %s], byte %hhi\n", src, data);
                        }
                    }

                } else if (mod == 0b01) {
                    // NOTE: Memory mode, 8-bit displacement follows
                    char *src = rm_encodings[rm];
                    int8_t d8 = (int8_t) ib_read_byte(&ib);

                    if (w == 1) {
                        int16_t data = (int16_t) (ib_read_byte(&ib) |
                                                  ib_read_byte(&ib) << 8);

                        if (d8 != 0) {
                            PRINT_OUTPUT("mov %s %c %hhi], word %hi\n",
                                         src, SIGN_CHAR(d8), ABS(d8), data);
                        } else { // if displacement is 0, do not display it
                            PRINT_OUTPUT("mov %s], word %hi\n", src, data);
                        }
                    } else { // w == 0, so only 8 bit data
                        int8_t data = (int8_t) ib_read_byte(&ib);

                        if (d8 != 0) {
                            PRINT_OUTPUT("mov %s %c %hhi], byte %hhi\n",
                                         src, SIGN_CHAR(d8), ABS(d8), data);
                        } else { // if displacement is 0, do not display it
                            PRINT_OUTPUT("mov %s], byte %hhi\n", src, data);
                        }
                    }

                } else if (mod == 0b10) {
                    // NOTE: Memory mode, 16-bit displacement follows
                    char *src = rm_encodings[rm];
                    uint16_t d16 = ib_read_byte(&ib) |
                                   ib_read_byte(&ib) << 8;

                    if (w == 1) {
                        int16_t data = (int16_t) (ib_read_byte(&ib) |
                                                  ib_read_byte(&ib) << 8);
                        PRINT_OUTPUT("mov %s + %hu], word %hi\n",
                                     src, d16, data);
                    } else {
                        int8_t data = (int8_t) ib_read_byte(&ib);
                        PRINT_OUTPUT("mov %s + %hu], byte %hhi\n",
                                     src, d16, data);
                    }

                } else if (mod == 0b11) {
                    // NOTE: Register mode (no displacement)
                    char *src = reg_encodings[w << 3 | rm];

                    if (w == 1) {
                        int16_t data = (int16_t) (ib_read_byte(&ib) |
                                                  ib_read_byte(&ib) << 8);
                        PRINT_OUTPUT("mov %s, word %hi\n", src, data);
                    } else {
                        int8_t data = (int8_t) ib_read_byte(&ib);
                        PRINT_OUTPUT("mov %s, byte %hhi\n", src, data);
                    }
                }
            } else {
                PRINT_ERROR("Unknown \"Immediate to register/memory\" instruction");
                // assert(0 &&
                //        "Unknown \"Immediate to register/memory\" instruction");
            }
        } else if (b1h == 0b1011) {
            // NOTE: Immediate to register
            uint8_t w = (b1 & 1 << 3) >> 3;
            PRINT_DEBUG("\tW: 0x%hhx\n", w);
            uint8_t reg = b1 & 0b111;
            PRINT_DEBUG("\tReg: 0x%hhx\n", reg);

            char *dst = reg_encodings[w << 3 | reg];

            if (w == 0) {
                uint8_t data8 = ib_read_byte(&ib);
                PRINT_OUTPUT("mov %s, %hhu\n", dst, data8);
            } else {
                uint16_t data16 = ib_read_byte(&ib) |
                                  ib_read_byte(&ib) << 8;
                PRINT_OUTPUT("mov %s, %hu\n", dst, data16);
            }
        } else if ((b1 & (0b111111 << 2)) >> 2 == 0b101000) {
            // NOTE: Memory to accumulator (or viceversa)
            // NOTE: Direction decides the following:
            //          - 0: memory to accumulator
            //          - 1: accumulator to memory 
            uint8_t direction = (b1 & 0b10) >> 1;
            uint8_t accumulator_size = (b1 & 0b1);
            uint16_t data16 = ib_read_byte(&ib) |
                              ib_read_byte(&ib) << 8;
            if (direction == 0) {
                PRINT_OUTPUT("mov %s, [%hu]\n",
                             (accumulator_size) ? "ax" : "al",
                             data16);
            } else {
                PRINT_OUTPUT("mov [%hu], %s\n",
                             data16,
                             (accumulator_size) ? "ax" : "al");
            }
        }
    }
    defer:
    if (fout) fclose(fout);
    return result;
}

int ib_from_file(const char *fp, InstrBuffer *ib) {
    int result = 0;
    FILE *fin = NULL;

    {
        if (!ib) {
            PRINT_ERROR("ERROR: InstrBuffer pointer is null.\n");
            return_defer(1);
        }

        fin = fopen(fp, "rb");
        if (fin == NULL) {
            PRINT_ERROR("ERROR: Could not open file %s: %s\n",
                        fp, strerror(errno));
            return_defer(errno);
        }

        if (fseek(fin, 0, SEEK_END)) {
            PRINT_ERROR("ERROR: Could not seek SEEK_END on file %s.\n", fp);
            return_defer(2);
        }
        ib->size = ftell(fin);
        if (ib->size < 0) {
            PRINT_ERROR(
                "ERROR: Could not tell file stream position on file %s: %s\n",
                fp, strerror(errno));
            return_defer(errno);
        }
        if (fseek(fin, 0, SEEK_SET)) {
            PRINT_ERROR("ERROR: Could not seek SEEK_SET on file %s.\n", fp);
            return_defer(3);
        }
        
        ib->buffer = malloc(ib->size);
        if (ib->buffer == NULL) {
            PRINT_ERROR(
                "ERROR: Could not allocate %u bytes to read from file %s.\n",
                ib->size, fp);
            return_defer(4);
        }
        fread(ib->buffer, ib->size, 1, fin);
        if (ferror(fin)) {
            PRINT_ERROR("ERROR: Could not read file %s.\n", fp);
            return_defer(5);
        }

        ib->current_index = 0;

    }

    defer:
    if (fin) fclose(fin);
    return result;
}

int ib_readable(InstrBuffer ib) {
    if (!ib.buffer) return 0;

    if (ib.current_index < ib.size) return 1;
    else return 0;
}

uint8_t ib_read_byte(InstrBuffer *ib) {
    uint8_t b = *(ib->buffer + ib->current_index);
    ++ib->current_index;

    return b;
}

