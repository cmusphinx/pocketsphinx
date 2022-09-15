/**
 * @file test_bitarr.c Test bit array io
 */

#include "lm/bitarr.h"
#include <pocketsphinx/err.h>
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
    uint32 test25 = 0x1234567;
    char mem[57+8];
    bitarr_address_t address;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    //sign bit should be 0x80000000 (BUT WHY?!?!?)
    TEST_EQUAL((neg1.i ^ pos1.i), 0x80000000);
    memset(mem, 0, sizeof(mem));
    address.base = mem;

    /* Test packed bits */
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 57)
      bitarr_write_int57(address, 57, test57);
    E_INFO("%d\n", mem[0]);
    E_INFO("%d\n", mem[1]);
    E_INFO("%d\n", mem[2]);
    E_INFO("%d\n", mem[3]);
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 57) {
      uint64 read57 = bitarr_read_int57(address, 57, ((1ULL << 57) - 1));
      E_INFO("Testing %llx at %d = %llx\n", test57, address.offset, read57);
      TEST_EQUAL(test57, read57);
    }
    /* Test arbitrary addresses */
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 1) {
      uint64 read57;

      memset(mem, 0, sizeof(mem));
      bitarr_write_int57(address, 57, test57);
      read57 = bitarr_read_int57(address, 57, ((1ULL << 57) - 1));
      E_INFO("Testing %llx at %d = %llx\n", test57, address.offset, read57);
      TEST_EQUAL(test57, read57);
    }
    memset(mem, 0, sizeof(mem));

    /* Test packed bits */
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 25)
      bitarr_write_int25(address, 25, test25);
    E_INFO("%d\n", mem[0]);
    E_INFO("%d\n", mem[1]);
    E_INFO("%d\n", mem[2]);
    E_INFO("%d\n", mem[3]);
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 25) {
      uint32 read25 = bitarr_read_int25(address, 25, ((1UL << 25) - 1));
      E_INFO("Testing %lx at %d = %lx\n", test25, address.offset, read25);
      TEST_EQUAL(test25, read25);
    }
    /* Test arbitrary addresses */
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 1) {
      uint32 read25;

      memset(mem, 0, sizeof(mem));
      bitarr_write_int25(address, 25, test25);
      read25 = bitarr_read_int25(address, 25, ((1UL << 25) - 1));
      E_INFO("Testing %lx at %d = %lx\n", test25, address.offset, read25);
      TEST_EQUAL(test25, read25);
    }
    memset(mem, 0, sizeof(mem));

    /* Test mixing 32 and 64-bit access. */
    for (address.offset = 0; address.offset < 57 * 8; address.offset += 82) {
      bitarr_address_t address2 = address;
      uint64 read25;

      bitarr_write_int25(address, 25, test25);
      address2.offset += 25;
      bitarr_write_int57(address2, 57, test57);
      read25 = bitarr_read_int25(address, 25, ((1UL << 25) - 1));
      E_INFO("Testing %lx at %d = %lx\n", test25, address.offset, read25);
      TEST_EQUAL(test25, read25);
      read25 = bitarr_read_int57(address2, 57, ((1ULL << 57) - 1));
      E_INFO("Testing %llx at %d = %llx\n", test57, address2.offset, read25);
      TEST_EQUAL(test57, read25);
    }
    return 0;
}
