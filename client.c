#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define COMMAND1 0
#define COMMAND2 1
#define COMMAND3 8
#define COMMAND4 16
int main(int argc, char * argv[]) {
    if (argc < 2) {
        printf("Please input the command\n");
        exit(-1);
    }
    int fd, fd1;
    int i;
    char data[40960];
    int retval;
    fd = open("/dev/inf", O_RDWR);
    if (fd == -1) {
        perror("error open\n");
        exit(-1);
    }
    printf("open /dev/inf successfully\n");
    
    if (!strcmp(argv[1], "processtree")) {
        printf("%s\n", argv[1]);
        retval = ioctl(fd, COMMAND1, 0);
        if (retval == -1) {
            perror("ioctl error\n");
            exit(-1);
        } 
        else {
            read(fd, data, 40960);
            int s;
            for (s = 0; s < 40960; s++) {
                printf("%c", data[s]);
            }
            printf("\n");
            printf("send command1 successfully\n");
        }
    }
    if (!strcmp(argv[1], "threadgroup")) {
        retval = ioctl(fd, COMMAND2, 0);
        if (retval == -1) {
            perror("ioctl error\n");
            exit(-1);
        } 
        else {
            read(fd, data, 40960);
            int s;
            for (s = 0; s < 40960; s++) {
                printf("%c", data[s]);
            }
            printf("\n");
            printf("send command2 successfully\n");
        }
    }
    if (!strcmp(argv[1], "memstat")) {
        pid_t pid = atoi(argv[2]);
        printf("%d\n", pid);
        retval = ioctl(fd, COMMAND3, pid);
        if (retval == -1) {
            perror("ioctl error\n");
            exit(-1);
        }
        else {
            read(fd, data, 40960);
            int s;
            for (s = 0; s < 40960; s++) {
                printf("%c", data[s]);
            }
            printf("\n");
            printf("send command3 successfully\n");
        }
    }
    if (!strcmp(argv[1], "processdetail")) {
        retval = ioctl(fd, COMMAND4, 0);
        if (retval == -1) {
            perror("ioctl error\n");
            exit(-1);
        } 
        else {
            int count = read(fd, data, 40960);
            int s;
            for (s = 0; s < 40960; s++) {
                printf("%c", data[s]);
            }
            printf("\n");
            printf("send command4 successfully\n");
        }
    }
    printf("read successfully\n");
    close(fd);
}
