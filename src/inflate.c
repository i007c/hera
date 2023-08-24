
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h> // printf
#include <string.h> // memset

#include <fcntl.h>
#include <unistd.h>

static uint8_t *buf;
static size_t  len = 0;
static size_t  idx = 0;  // buffer index
static uint8_t bdx = 0;
// static uint32_t bit_len = 0;
// static uint32_t bit_buf = 0;

static const uint8_t CL_ORDER[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

// void fill_bits(void) {
//     while (bit_len <= 24) {
//         assert(bit_buf < (1U << bit_len));
//         assert(idx < len);
//
//         bit_buf |= buf[idx] << bit_len;
//         idx++;
//         bit_len += 8;
//     }
// }
//
// uint32_t get_bits(uint32_t count) {
//     if (bit_len < count) fill_bits();
//     uint32_t output = bit_buf & ((1 << count) - 1);
//     bit_buf = bit_buf >> count;
//     bit_len -= count;
//     return output;
// }

uint8_t read_bit(void) {
    uint8_t c = (buf[idx] >> bdx) & 1;
    bdx++;
    if (bdx >= 8) {
        idx++;
        bdx = 0;
    }
    return c;
}

uint32_t read_bits(uint8_t count) {
    uint32_t o = 0;

    for (uint8_t ta = 0; ta < count; ta++) {
        o |= read_bit() << ta;
    }

    return o;
}

void inflate(uint8_t *buffer, size_t length) {
    printf("inflate length: %ld\n", length);

    buf = buffer;
    len = length;
    idx = 0;

    uint8_t cfm = buf[idx]; idx++;
    uint8_t cm = cfm & 15; // 15 == 0b00001111
    uint8_t cinfo = cfm >> 4;
    uint32_t win = 1 << (8 + cinfo);

    printf(
        "Compression Method: %d\n"
        "Compression Info  : %d\n"
        "Window Size       : %d\n",
        cm, cinfo, win
    );

    uint8_t flg = buffer[idx]; idx++;
    uint8_t fcheck = flg & 31;   // first 5 bytes
    bool fdict = flg & 32;       // byte 6
    uint8_t flevel = flg >> 6;   // byte 7 and 8

    printf(
        "flg: %d, fcheck: %d, fdict: %d, flevel: %d, c: %d\n",
        flg, fcheck, fdict, flevel,
        (cfm * 256 + flg) % 31
    );

    // inflate(&buffer[i + 2], length - 6);
    // uint32_t adlr32 = buffer[i + length - 4];
    // log_verbose("adlr32: %d", adlr32);


    bool final = false;
    uint8_t type = 0;

    // int fd = open("dump", O_CREAT | O_WRONLY);
    // printf("fd: %d", fd);
    // write(fd, buffer, len);
    // close(fd);
    // printf("\033[33m10110111\033[0m\n");
    
    // final = read_bit();
    // type = read_bits(2);

    // printf("final: %d, type: %d\n", final, type);

    // return;
    // for (uint8_t tk = 0; tk < 10; tk++) {
    //     printf("%u", read_bit());
    //     // uint8_t n = buf[idx];
    //     //
    //     // for (uint8_t j=0; j < 8; j++) {
    //     //     // printf("%u", !!((n & 128)));
    //     //     // n = n << 1;
    //     //     printf("%u", !!((n & 0x01)));
    //     //     n = n >> 1;
    //     // }
    //     // printf(" ");
    //     // printf("%x", buf[idx]);
    // }
    // printf("\n");


    while (!final) {
        final = read_bit();
        type = read_bits(2);
        printf("final: %d, type: %d\n", final, type);

        if (type != 2) return;

        uint8_t hlit = read_bits(5);
        // uint16_t num_ll_codes = hlit + 257;

        uint8_t hdist = read_bits(5);
        // uint8_t num_dist_codes = hdist + 1;

        uint8_t hclen = read_bits(4);
        uint8_t num_cl_codes = hclen + 4;

        printf("HLIT: %d, HDIST: %d, HCLEN: %d\n", hlit, hdist, hclen);

        uint8_t cl_code_lengths[19];
        memset(cl_code_lengths, 0, sizeof(cl_code_lengths));

        for (uint8_t ci = 0; ci < num_cl_codes; ci++) {
            cl_code_lengths[CL_ORDER[ci]] = read_bits(3);
        }

        break;
    }


    // printf("buffer[0] = %d, length = %ld\n", buffer[0], length);
}

