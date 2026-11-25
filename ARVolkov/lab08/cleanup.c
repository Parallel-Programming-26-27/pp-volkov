#include <sys/mman.h>
#include <unistd.h>

void cleanup_mmap_fd(char* data, size_t file_size, int fd) {
    if (data != NULL && data != MAP_FAILED) {
        munmap(data, file_size);
    }
    if (fd != -1) {
        close(fd);
    }
}
