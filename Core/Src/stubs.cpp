#include <errno.h>
#include <sys/types.h>

extern "C" {
    // supress the warning stupid new stm32
    int _getentropy(void *ptr, size_t len) {
        return 0;
    }
}
