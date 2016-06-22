#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool parseNibble(char c, uint8_t *nibble)
{
    if ((c >= '0') && (c <= '9')) {
        *nibble = (c - '0');
    } else if ((c >= 'A') && (c <= 'F')) {
        *nibble = (c - 'A' + 10);
    } else {
        return false;
    }
    return true;
}

static bool parseHexByte(const char *str, uint8_t *data)
{
    uint8_t upper, lower;
    if (!parseNibble(str[0], &upper) || !parseNibble(str[1], &lower)) {
        return false;
    }
    *data = (upper << 4) | lower;
    return true;
}

bool parseHexString(const char *str, uint8_t buf[], int size)
{
    int len;
    int i;
    int j;

    // check hex string length (should be even)
    len = strlen(str);
    if ((len % 2) != 0) {
        return false;
    }

    // check buffer size (should fit the converted data)
    if (size < (len / 2)) {
        return false;
    }

    // convert string
    for (i = 0, j = 0; i < len; i += 2, j++) {
        if (!parseHexByte(str + i, buf + j)) {
            // invalid char
            return false;
        }
    }

    return true;
}

