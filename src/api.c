#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 

#define KERNEL_MODULE_INPUT_FILE "/sys/kernel/debug/page_table_interface/input"
#define KERNEL_MODULE_OUTPUT_FILE "/sys/kernel/debug/page_table_interface/output"

int main() {
    int pid;

    printf("Enter PID: ");
    if (scanf("%d", &pid) != 1) {
        perror("Error");
        return 1;
    }

    int fd = open(KERNEL_MODULE_INPUT_FILE, O_WRONLY | O_CREAT, 0644);
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

    close(fd);

    return 0;
}