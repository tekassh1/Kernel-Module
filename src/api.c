#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 

#define KERNEL_MODULE_INTERFACE_FILE "/sys/kernel/debug/page_table_interface/interface"
#define ANSWER_BUFFER_SIZE 4000000

int main() {
    int pid;

    printf("Enter PID: ");
    if (scanf("%d", &pid) != 1) {
        perror("Error");
        return 1;
    }

    int fd = open(KERNEL_MODULE_INTERFACE_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Error");
        return 1;
    }

    char buffer[50];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", pid);

    if (write(fd, buffer, len) == -1) {
        perror("Error writing to file");
        close(fd);
        return 1;
    }

    char read_buffer[ANSWER_BUFFER_SIZE];
    ssize_t bytes_read = read(fd, read_buffer, sizeof(read_buffer) - 1);
    if (bytes_read == -1) {
        perror("Error reading from kernel module!");
        close(fd);
        return 1;
    }

    read_buffer[bytes_read] = '\0';

    printf("\n%s", read_buffer);

    close(fd);

    return 0;
}