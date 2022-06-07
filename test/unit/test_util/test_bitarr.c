/**
 * @file test_bitarr.c Test bit array io
 */

#include "bitarr.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union {
    float f;
    uint32 i;
} float_enc;

int
main(int argc, char *argv[])
{
    float_enc neg1 = { -1.0 }, pos1 = { 1.0 };
    uint64 test57 = 0x123456789abcdefULL;
    char mem[57+8];
    bitarr_address_t address;
    //sign bit should be 0x80000000
    TEST_EQUAL((neg1.i ^ pos1.i), 0x80000000);
    memset(mem, 0, sizeof(mem));
    address.base = mem;
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 57) {
      bitarr_write_int57(address, 57, test57);
    }
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 57) {
      TEST_EQUAL(test57, bitarr_read_int57(address, 57, ((1ULL << 57) - 1)));
    }
    // TODO: more checks.
    return 0;
}
