
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h> // printf
#include <string.h> // memset
#include <stdlib.h> // malloc

#include <fcntl.h>
#include <unistd.h>

#define SFMT "\033[92m'%s'\033[0m"
#define DFMT "\033[32m%d\033[0m"
#define LDFMT "\033[92m%ld\033[0m"

static uint64_t total_bytes = 0;
static uint8_t *buf;
static uint8_t *out;
static size_t  len = 0;
static size_t  idx = 0;  // buffer index
static uint8_t bdx = 0;
// static uint32_t bit_len = 0;
// static uint32_t bit_buf = 0;

static const uint8_t CL_ORDER[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

static const uint16_t LENGTH_BASE[31] = {
   3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 
   15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 
   67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};

static const uint8_t LENGTH_EXTRA[31] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4,
    5, 5, 5, 5, 0, 0, 0
};

static const uint32_t DIST_BASE[32] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577, 0, 0
};

static const uint8_t DIST_EXTRA[32] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static uint8_t FIXED_LENGTHS[288] = {
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};

static uint8_t FIXED_DIST[32] = {
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
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

uint32_t read_bits_reverse(uint8_t count) {
    uint32_t o = 0;
    for (uint8_t ta = 0; ta < count; ta++) {
        o = (o << 1) | read_bit();
    }
    return o;
}

// int bit_reverse_16(int n) {
//     n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
//     n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
//     n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
//     n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
//     return n;
// }
//
// int bit_reverse(int v, int bits) {
//     assert(bits <= 16);
//     // to bit reverse n bits, reverse 16 and shift
//     // e.g. 11 bits, bit reverse and shift away 5
//     return bit_reverse_16(v) >> (16-bits);
// }



/*

path is a list of directions on the huffman tree
0010110 is path
lets say that 0 is left and 1 is right direction on the tree
our path becomes: left left right left right right left
                   0    0     1    0     1     1    0
                   L    L     R    L     R     R    L
*/

// Symbol and Length
// typedef uint16_t SymLen[2];

typedef struct TreeNode {
    struct TreeNode *left;
    struct TreeNode *right;
    int32_t value;
} TreeNode;

// #define FAST_BITS 9
// #define FATS_MASK ((1 << 9) - 1)
// #define LLSYM_NUM 288

typedef struct {
    // min and max path length
    // absolute min is 1 which is either 0 or 1. left or right
    uint8_t min;
    // absolute max is 15 which is enforeced by the CL table
    uint8_t max; 

    // table length is dependent on the max value
    // for example if max value is 4. our table length is only 15
    // [010110]: 256 end of block symbol
    // [1011]  : 33 byte literal of char 'A'
    // SymLen table[300];

    // uint16_t fast[1 << FAST_BITS];
    // uint16_t first_codes[16];
    // uint16_t first_symbols[16];
    //
    // int max_codes[17];
    // uint8_t size[LLSYM_NUM];
    // uint16_t value[LLSYM_NUM];

    TreeNode root;
    // TreeNode nodes[10];

    uint16_t length; // max length is (1 << 15) - 1 which is 32768
} TreeTable;

void build_code_tree(uint8_t *code_lengths, uint16_t alen, TreeTable *tree) {
    uint16_t ix; // temp value used in for loops

    // printf("alen: %d\n", alen);
    uint16_t length_frequency[17];
    memset(length_frequency, 0, sizeof(length_frequency));

    tree->min = 0;
    tree->max = 0;
    tree->length = 0;

    // memset(tree->table, 0, sizeof(tree->table));

    for (ix=0; ix < alen; ix++) {
        uint8_t code_length = code_lengths[ix];
        if (!code_length) continue;
        ++length_frequency[code_length];

        if (code_length > tree->max) {
            tree->max = code_length;
        }
        if (!tree->min || tree->min > code_length) {
            tree->min = code_length;
        }
    }

    // tree->length = (1 << tree->max) - 1;

    // printf(
    //     "min length: "DFMT"\nmax length: "DFMT"\ntree table length: "DFMT"\n",
    //     tree->min, tree->max, tree->length
    // );

    // a code with a length of 0 must appear 0 times :)
    length_frequency[0] = 0;


    uint16_t code = 0;
    uint16_t codes[17];
    for (ix=1; ix < tree->max+1; ix++) {
        // printf("code: \33[33m%d\33[m ", code);
        code = (code + length_frequency[ix-1]) << 1;
        codes[ix] = code;

        // printf(
        //     "length: "DFMT" - frequency: "DFMT" - code: "DFMT"\n" ,
        //     ix, length_frequency[ix], codes[ix+1]
        // );
    }

    // static const char *SPACE = "                    ";
    // uint16_t node_idx = 0;


    TreeNode *node;

    tree->root.value = -1;
    tree->root.left = NULL;
    tree->root.right = NULL;
    // for (ix=0; ix < 289; ix++) tree->nodes[ix].value = -1;

    for (ix=0; ix < alen; ix++) {
        uint8_t code_length = code_lengths[ix];
        if (!code_length) continue;

        // uint16_t n = codes[code_length];

        // printf(
        //     "[\33[32m%2d \33[35m%5d \33[32m0b\33[0m",
        //     code_length, n
        // );
        // for (int8_t ij=code_length-1; ij > -1; ij--) {
        //     printf("%u", !!(n & (1 << ij)));
        // }
        // printf("%*.s", tree->max-code_length, SPACE);
        // printf("]: "DFMT"\n", ix);

        // uint16_t code = codes[code_length];
        node = &tree->root;
        code = codes[code_length];

        // assert(node != NULL);
        for (int8_t ij=code_length-1; ij > -1; ij--) {
            // assert(node != NULL);
            node->value = -1;
            // printf(
            //     "node_idx: "DFMT" - value: "DFMT" - "
            //     "left == NULL: "DFMT" - right == NULL: "DFMT"\n",
            //     node_idx, node->value, node->left == NULL, node->right == NULL
            // );
            // printf("")

            if (code & (1 << ij)) { // right
                if (node->right == NULL) {

                    node->right = malloc(sizeof(TreeNode));
                    assert(node->right != NULL);

                    node->right->right = NULL;
                    node->right->left = NULL;
                    node->right->value = -1;
                    // node_idx++;
                }
                node = node->right;
            } else { // left
                if (node->left == NULL) {
                    // node->left= &tree->nodes[node_idx];
                    node->left = malloc(sizeof(TreeNode));
                    assert(node->left != NULL);

                    node->left->right = NULL;
                    node->left->left = NULL;
                    node->left->value = -1;
                    // node_idx++;
                }
                node = node->left;
            }
        }
        assert(node != NULL);
        node->value = (int32_t)ix;

        
        codes[code_length]++;
    }
}

/* create_tree {{{
void create_tree(uint8_t *code_lengths, uint16_t alen, TreeTable *tree) {
    int ix = 0;
    int ik = 0;
    int code = 0;
    int next_code[16];
    int frequencies[17];

    printf("alen: \33[31m%3d\33[m\n", alen);

    memset(frequencies, 0, sizeof(frequencies));
    memset(tree->fast, 0, sizeof(tree->fast));

    for (ix=0; ix < alen; ix++)
        frequencies[code_lengths[ix]]++;

    frequencies[0] = 0;

    for (ix=0; ix < 17; ix++) {
        if (!frequencies[ix]) continue;
        printf(
            "length: \33[33m%2d\33[m frequency: \33[36m%3d\33[m\n",
            ix, frequencies[ix]
        );
    }


    code = 0;
    for (ix=1; ix < 16; ix++) {
        assert(frequencies[ix] < (1 << ix));

        next_code[ix] = code;
        tree->first_codes[ix] = (uint16_t)code;
        tree->first_symbols[ix] = (uint16_t)ik;

        code = (code + frequencies[ix]);
        if (frequencies[ix]) assert((code - 1) < (1 << ix));

        tree->max_codes[ix] = code << (16 - ix); // preshift for inner loop??
        code = code << 1;
        ik += frequencies[ix];
    }

    for (ix=0; ix < 16; ix++) {
        printf(
            "len(\33[33m%2d\33[m) next_code("DFMT") first_code("DFMT") "
            "first_symbol("DFMT") frequency("DFMT")\n",
            ix, next_code[ix], tree->first_codes[ix], tree->first_symbols[ix],
            frequencies[ix]
        );
    }

    tree->max_codes[16] = 0x10000;

    for (ix=0; ix < alen; ix++) {
        int length = code_lengths[ix];
        if (!length) continue;

        int c = (
            next_code[length] - 
            tree->first_codes[length] +
            tree->first_symbols[length]
        );
        uint16_t fastv = (uint16_t)((length << 9) | ix);
        tree->size[c] = (uint8_t)length;
        tree->value[c] = (uint16_t)ix;
        if (length <= FAST_BITS) {
            int j = bit_reverse(next_code[length], length);
            while (j < (1 << FAST_BITS)) {
                tree->fast[j] = fastv;
                j += (1 << length);
            }
        }
        next_code[length]++;
    }
}
}}} */


void parse_data(TreeTable *ll_tree, TreeTable *dist_tree) {
    TreeNode *node = &ll_tree->root;
    int32_t symbol = -1;
    uint16_t rcode = 0;
    uint8_t  rcodex = 0;

    while (true) {
        uint8_t bd = read_bit();
        node = bd ? node->right : node->left;
        assert(node != NULL);
        if (node->value == -1) continue; 

        symbol = node->value;
        node = &ll_tree->root;

        assert(symbol >= 0 && symbol <= 288);

        if (symbol < 256) {
            // printf("%02x ", symbol);
            *out++ = (uint8_t)symbol;
            total_bytes++;
        } else if (symbol == 256) {
            // printf("end of block\n");
            break;
        } else {
            symbol -= 257;
            uint16_t len = LENGTH_BASE[symbol];
            // printf(
            //     "len symbol: "DFMT" - base: "DFMT" - extra: "DFMT"\n",
            //     symbol + 257, len, LENGTH_EXTRA[symbol]
            // );

            if (LENGTH_EXTRA[symbol]) {
                len += read_bits(LENGTH_EXTRA[symbol]);
            }

            node = &dist_tree->root;
            while (true) {
                bd = read_bit();
                rcode |= bd << rcodex;
                rcodex++;
                node = bd ? node->right : node->left;
                if (node == NULL) {
                    printf("dist rcode: "DFMT", len: "DFMT"\n", rcode, rcodex);
                    return;
                }
                // assert(node != NULL);
                if (node->value == -1) continue; 
                rcode = 0;
                rcodex = 0;
                // printf("\n");

                symbol = node->value;
                node = &ll_tree->root;
                assert(symbol >= 0 && symbol <= 30);
                break;
            }


            uint32_t dist = DIST_BASE[symbol];
            // printf(
            //     "dist symbol: "DFMT" - base: "DFMT" - extra: "DFMT"\n",
            //     symbol, dist, DIST_EXTRA[symbol]
            // );

            if (DIST_EXTRA[symbol]) {
                dist += read_bits(DIST_EXTRA[symbol]);
            }

            assert(dist < total_bytes);

            total_bytes += len;

            uint8_t *p = out - dist;
            if (dist == 1) {
                uint8_t v = *p;
                if (len) {
                    do {
                        *out++ = v;
                    } while (--len);
                }
            } else {
                if (len) {
                    do {
                        *out++ = *p++;
                    } while (--len);
                }
            }
            // printf("<"DFMT":"DFMT">\n", len, dist);
        }
    }
}


/* decode_dynamic {{{ */
void decode_dynamic(void) {
    uint8_t hlit = read_bits(5);
    uint16_t num_ll_codes = hlit + 257;

    uint8_t hdist = read_bits(5);
    uint8_t num_dist_codes = hdist + 1;

    uint8_t hclen = read_bits(4);
    uint8_t num_cl_codes = hclen + 4;

    // printf(
    //     "number of LL   codes: "DFMT" (HLIT : "DFMT")\n"
    //     "number of dist codes: "DFMT" (HDIST: "DFMT")\n"
    //     "number of CL   codes: "DFMT" (HCLEN: "DFMT")\n",
    //     num_ll_codes, hlit,
    //     num_dist_codes, hdist,
    //     num_cl_codes, hclen
    // );

    uint8_t cl_code_lengths[19];
    memset(cl_code_lengths, 0, sizeof(cl_code_lengths));

    for (uint8_t ci = 0; ci < num_cl_codes; ci++) {
        cl_code_lengths[CL_ORDER[ci]] = read_bits(3);
    }

    TreeTable cl_tree;
    build_code_tree(cl_code_lengths, 19, &cl_tree);

    uint8_t ll_code_lengths[288];
    uint8_t dist_code_lengths[32];
    memset(ll_code_lengths, 0, sizeof(ll_code_lengths));
    memset(dist_code_lengths, 0, sizeof(dist_code_lengths));

    uint8_t repeat_count = 0;
    uint16_t repeat_symbol = 0;
    int32_t symbol = 0;

    TreeNode *node = &cl_tree.root;


    uint16_t rcode = 0;
    uint8_t  rcodex = 0;

    for (uint16_t ix=0; ix < num_ll_codes + num_dist_codes;) {
        uint8_t bd = read_bit();
        rcode |= bd << rcodex;
        rcodex++;

        // printf("%u", bd);
        node = bd ? node->right : node->left;
        if (node == NULL) {
            printf("code rcode: "DFMT", len: "DFMT"\n", rcode, rcodex);
            return;
        }
        // assert(node != NULL);
        if (node->value == -1) continue; 
        rcode = 0;
        rcodex = 0;
        // printf("\n");

        symbol = node->value;
        node = &cl_tree.root;
        assert(symbol >= 0 && symbol <= 18);

        /* {{{ */
        // uint16_t cl_code = read_bits_reverse(cl_tree.min);
        // uint8_t cl_code_len = cl_tree.min;
        // uint16_t symbol = 420;
        //
        // node = &cl_tree.root;
        // for (int8_t ij=cl_code_len-1; ij > -1; ij--) {
        //     if (cl_code & (1 << ij)) { // right
        //         node = node->right;
        //     } else { // left
        //         node = node->left;
        //     }
        //     assert(node != NULL);
        // }
        // printf("node value == ix: %d\n", node->value == ix);

        // for (uint16_t ik=0; ik < 301; ik++) {
        //     if (ik == 288) {
        //         cl_code = (cl_code << 1) | read_bit();
        //         // cl_code_len++;
        //         ik = 0;
        //         continue;
        //     }
        //

        // if (
        //     cl_tree.table[ik][1] == cl_code_len &&
        //     cl_tree.table[ik][0] == cl_code
        // ) {

        // if (ik == 0) {
        //     printf("code len: %d - code: %d\n", cl_code_len, cl_code);
        // }
        //     symbol = ik;
        //     break;
        // }
        // }

        // assert(symbol != 420);
        // printf("symbol: %d\n", symbol);
        // uint16_t code_length = cl_tree.table[cl_code][1];

        // printf("symbol: "DFMT" - code length: "DFMT, ll_symbol, ll_code_length);
        // printf("\ncl_code: "DFMT" - \33[32m0b\33[0m", cl_code);
        // for (int8_t ij=cl_tree.min-1; ij > -1; ij--) {
        //     printf("%u", !!(cl_code & (1 << ij)));
        // }
        // printf("\n");

        // if (code_length != cl_tree.min) {
        //     for (uint8_t ib=cl_tree.min+1; ib < cl_tree.max+1; ib++) {
        //         cl_code = (cl_code << 1) | read_bit();
        //
        //         // printf("cl_code: "DFMT" - \33[33m0b\33[m", cl_code);
        //         // for (int8_t ij=ib-1; ij > -1; ij--) {
        //         //     printf("%u", !!(cl_code & (1 << ij)));
        //         // }
        //         // printf("\n");
        //         
        //         code_length = cl_tree.table[cl_code][1];
        //         if (code_length == ib) {
        //             symbol = cl_tree.table[cl_code][0];
        //             break;
        //         }
        //     }
        // }
        /* }}} */ 


        if (symbol <= 15) {
            if (ix >= num_ll_codes) {
                dist_code_lengths[ix - num_ll_codes] = symbol;
            } else {
                ll_code_lengths[ix] = symbol;
            }
            ix++;
            repeat_count = 0;
            repeat_symbol = symbol;
        } else if (symbol == 16) {
            repeat_count = read_bits(2) + 3;
            // repeat_symbol = ll_code_lengths[ix - 1];
        } else if (symbol == 17) {
            repeat_count = read_bits(3) + 3;
            repeat_symbol = 0;
        } else if (symbol == 18) {
            repeat_count = read_bits(7) + 11;
            repeat_symbol = 0;
        } else {
            assert(false);
        }
        // printf(
        //     "[%d] symbol: "DFMT" - rcount: "DFMT" - rsymbol: "DFMT"\n",
        //     ix, symbol, repeat_count, repeat_symbol
        // );

        // if (repeat_count) ix--;

        for (uint8_t ih=0; ih < repeat_count; ih++) {
            if (ix >= num_ll_codes) {
                dist_code_lengths[ix - num_ll_codes] = repeat_symbol;
            } else {
                ll_code_lengths[ix] = repeat_symbol;
            }
            ix++;
        }


        // if (ix > 10) break;
    }

    TreeTable ll_tree;
    build_code_tree(ll_code_lengths, num_ll_codes, &ll_tree);

    TreeTable dist_tree;
    build_code_tree(dist_code_lengths, num_dist_codes, &dist_tree);

    parse_data(&ll_tree, &dist_tree);

}
/* }}} */


void decode_fixed(void) {

    TreeTable ll_tree;
    build_code_tree(FIXED_LENGTHS, 288, &ll_tree);

    TreeTable dist_tree;
    build_code_tree(FIXED_DIST, 32, &dist_tree);

    parse_data(&ll_tree, &dist_tree);

}

void inflate(uint8_t *buffer, size_t length, uint8_t *output) {
    /* {{{ */
    printf("inflate length: "LDFMT"\n", length);

    buf = buffer;
    len = length;
    idx = 0;
    out = output;

    uint8_t cfm = buf[idx]; idx++;
    uint8_t cm = cfm & 15; // 15 == 0b00001111
    uint8_t cinfo = cfm >> 4;
    uint32_t win = 1 << (8 + cinfo);

    printf(
        "Compression Method: "DFMT"\n"
        "Compression Info  : "DFMT"\n"
        "Window Size       : "DFMT"\n",
        cm, cinfo, win
    );

    uint8_t flg = buf[idx]; idx++;
    uint8_t fcheck = flg & 31;   // first 5 bytes
    bool fdict = flg & 32;       // byte 6
    uint8_t flevel = flg >> 6;   // byte 7 and 8

    printf(
        "flg: "DFMT", fcheck: "DFMT", fdict: "DFMT
        ", flevel: "DFMT", c: "DFMT"\n",
        flg, fcheck, fdict, flevel,
        (cfm * 256 + flg) % 31
    );

    // inflate(&buffer[i + 2], length - 6);
    // uint32_t adlr32 = buffer[i + length - 4];
    // log_verbose("adlr32: %d", adlr32);


    bool final = false;
    uint8_t type = 0;

    // int fd = open("dump", O_CREAT | O_WRONLY, 00660);
    // lseek(fd, 0, SEEK_SET);
    // write(fd, &buf[2], len-6);
    // close(fd);
    
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

    /* }}} */

    total_bytes = 0;

    while (!final) {
        final = read_bit();
        type = read_bits(2);

        // printf("final: "DFMT", type: "DFMT"\n", final, type);

        if (type == 0) {
            if (bdx) read_bits(8-bdx);

            uint16_t data_len = read_bits(8) | (read_bits(8) << 8);
            uint16_t data_nlen = read_bits(8) | (read_bits(8) << 8);

            assert(data_len == ((~data_nlen) & 0xffff));
            memcpy(out, &buf[idx], data_len);
            out += data_len;
            idx += data_len;
            total_bytes += data_len;
        } else if (type == 1) {
            decode_fixed();
        } else if (type == 2) {
            decode_dynamic();
        } else {
            printf("invalid type: %d\n", type);
            return;
        }
    }

    printf("total_bytes: "LDFMT"\n", total_bytes);
}

