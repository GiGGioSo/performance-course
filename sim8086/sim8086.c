
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define OUTPUT 1
#define DEBUG 1
#define ERROR 1

#if OUTPUT
    #define PRINT_OUTPUT(...) printf(__VA_ARGS__)
#else
    #define PRINT_OUTPUT(...) 0
#endif

#if DEBUG
    #define PRINT_DEBUG(...) printf(__VA_ARGS__)
    #define PRINT_ERROR(...) fprintf(stderr, __VA_ARGS__)
#else
    #define PRINT_DEBUG(...) 0

    #if ERROR
        #define PRINT_ERROR(...) fprintf(stderr, __VA_ARGS__)
    #else
        #define PRINT_ERROR(...) 0
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

// NOTE: Antepone the W bit to the register bits
unsigned char *registers[] = {
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

int main(int argc, char **argv) {

    if (argc == 3) {
        PRINT_DEBUG("Input file: %s\n", argv[1]);
        PRINT_DEBUG("Output file: %s\n", argv[2]);
    } else {
        PRINT_ERROR("ERROR: Input and output file are needed!\n");
        return -1;
    }

    const unsigned char *input_path = argv[1];
    const unsigned char *output_path = argv[2];

    int result = 0;
    FILE *fin = NULL;
    FILE *fout = NULL;

    {
        fin = fopen(input_path, "rb");
        if (fin == NULL) {
            PRINT_ERROR("ERROR: Could not open file %s: %s\n",
                        input_path, strerror(errno));
            return_defer(errno);
        }

        fout = fopen(output_path, "wb");
        if (fout == NULL) {
            PRINT_ERROR("ERROR: Could not open file %s: %s\n",
                        output_path, strerror(errno));
            return_defer(errno);
        }

        uint8_t buffer[999];
        size_t buffer_size = fread(buffer, 1, 999, fin);
        if (ferror(fin)) {
            PRINT_ERROR("ERROR: Could not read file %s: %s\n",
                        input_path, strerror(errno));
            return_defer(errno);
        } else if (feof(fin)) {
            PRINT_DEBUG("End of file successfully reached!\n");
        }

        PRINT_DEBUG("%zu bytes read!\n", buffer_size);

        uint8_t instructions[buffer_size / 2][3];

        for(size_t i = 0; i < ARRAY_LENGTH(instructions); ++i) {

            instructions[i][0] = buffer[i*2];
            instructions[i][1] = buffer[i*2 + 1];
            instructions[i][2] = 0;

        }

        for(size_t i = 0; i < ARRAY_LENGTH(instructions); ++i) {

            uint16_t instr = instructions[i][0] << 8 | instructions[i][1];

            PRINT_DEBUG("Instruction %zd: %hx\n", i, instr);

            uint8_t opcode = (instr & (0b111111 << 10)) >> 10;
            PRINT_DEBUG("\tOpcode: 0x%hhx\n", opcode);
            uint8_t d = (instr & (1 << 9)) >> 9;
            PRINT_DEBUG("\tD: 0x%hhx\n", d);
            uint8_t w = (instr & (1 << 8)) >> 8;
            PRINT_DEBUG("\tW: 0x%hhx\n", w);
            uint8_t mod = (instr & (0b11 << 6)) >> 6;
            PRINT_DEBUG("\tMod: 0x%hhx\n", mod);
            uint8_t reg = (instr & (0b111 << 3)) >> 3;
            PRINT_DEBUG("\tReg: 0x%hhx\n", reg);
            uint8_t rm = instr & 0b111;
            PRINT_DEBUG("\tR/M: 0x%hhx\n", rm);

            if (opcode == 0b100010) {
                if (mod == 0b11) {

                    unsigned char *dst_reg = registers[w << 3 | reg];
                    unsigned char *src_reg = registers[w << 3 | rm];

                    if (d == 0) SWAP(unsigned char *, dst_reg, src_reg);

                    PRINT_OUTPUT("mov %s, %s\n", dst_reg, src_reg);
                    fprintf(fout, "mov %s, %s\n", dst_reg, src_reg);
                    if (ferror(fout)) {
                        PRINT_ERROR("ERROR: Could not write to file %s: %s\n",
                                    output_path, strerror(errno));
                        return_defer(errno);
                    }

                } else {
                    PRINT_ERROR(
                            "ERROR: MOD bits combination not yet supported.\n");
                    continue;
                }
            } else {
                PRINT_ERROR(
                        "ERROR: OPCODE bits combination not yet supported.\n");
                continue;
            }

        }


    }

    defer:
    if (fin) fclose(fin);
    if (fout) fclose(fout);
    return result;
}
