#include <nds/ndstypes.h>

static char hexbuffer[9];

char* tohex(u32 n) {
    unsigned size = 9;
    char *buffer = hexbuffer;
    unsigned index = size - 2;

	for (int i = 0; i < size; i++) {
		buffer[i] = '0';
	}

    while (n > 0) {
        unsigned mod = n % 16;

        if (mod >= 10)
            buffer[index--] = (mod - 10) + 'A';
        else
            buffer[index--] = mod + '0';

        n /= 16;
    }
    buffer[size - 1] = '\0';
    return buffer;
}
