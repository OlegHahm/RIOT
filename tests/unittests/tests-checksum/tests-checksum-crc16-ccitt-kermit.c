/*
 * Copyright 2016 Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdint.h>

#include "embUnit/embUnit.h"

#include "checksum/crc16_ccitt_kermit.h"

#include "tests-checksum.h"

static int calc_and_compare_crc_with_update(const unsigned char *buf,
        size_t len, size_t split, uint16_t expected)
{
    uint16_t result = crc16_ccitt_kermit_calc(buf, split);

    result = crc16_ccitt_kermit_update(result, buf + split, len - split);

    return result == expected;
}

static int calc_and_compare_crc(const unsigned char *buf, size_t len,
        uint16_t expected)
{
    uint16_t result = crc16_ccitt_kermit_calc(buf, len);

    return result == expected;
}

static void test_checksum_crc16_ccitt_sequence(void)
{
    /* Reference values according to
     * http://www.lammertbies.nl/comm/info/crc-calculation.html */
    {
        unsigned char buf[] = "";
        uint16_t expect = 0x0000;

        TEST_ASSERT(calc_and_compare_crc(buf, sizeof(buf) - 1, expect));
        TEST_ASSERT(calc_and_compare_crc_with_update(buf, sizeof(buf) - 1,
                    (sizeof(buf) - 1) / 2, expect)); }

    {
        unsigned char buf[] = "A";
        uint16_t expect = 0x8D53;

        TEST_ASSERT(calc_and_compare_crc(buf, sizeof(buf) - 1, expect));
        TEST_ASSERT(calc_and_compare_crc_with_update(buf, sizeof(buf) - 1,
                    (sizeof(buf) - 1) / 2, expect)); }

    {
        unsigned char buf[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                              "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                              "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                              "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                              "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                              "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                              "AAAA";
        uint16_t expect = 0xE2E7;

        TEST_ASSERT(calc_and_compare_crc(buf, sizeof(buf) - 1, expect));
        TEST_ASSERT(calc_and_compare_crc_with_update(buf, sizeof(buf) - 1,
                    (sizeof(buf) - 1) / 2, expect)); }

    {
        unsigned char buf[] = "123456789";
        uint16_t expect = 0x8921;

        TEST_ASSERT(calc_and_compare_crc(buf, sizeof(buf) - 1, expect));
        TEST_ASSERT(calc_and_compare_crc_with_update(buf, sizeof(buf)
                    - 1, (sizeof(buf) - 1) / 2, expect));
    }

    {
        unsigned char buf[] = { 0x12, 0x34, 0x56, 0x78 };
        uint16_t expect = 0xF067;

        TEST_ASSERT(calc_and_compare_crc(buf, sizeof(buf), expect));
        TEST_ASSERT(calc_and_compare_crc_with_update(buf, sizeof(buf),
                    sizeof(buf) / 2, expect));
    }
}

Test *tests_checksum_crc16_ccitt_kermit_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_checksum_crc16_ccitt_sequence),
    };

    EMB_UNIT_TESTCALLER(checksum_crc16_ccitt_kermit_tests, NULL, NULL, fixtures);

    return (Test *)&checksum_crc16_ccitt_kermit_tests;
}
