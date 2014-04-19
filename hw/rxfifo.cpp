#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <iostream>

#define MAX_BUF 1024

int main()
{
    int fd;
    char myfifo[] = "/tmp/myfifo";
    uint16_t I=0;
    uint16_t Q=0;


    /* open, read, and display the message from the FIFO */
    fd = open(myfifo, O_RDONLY);
    while( I != EOF && Q != EOF ){
      read(fd, &I, sizeof(uint16_t));
      read(fd, &Q, sizeof(uint16_t));
      std::cout << "Read: I: " << I << " Q: " << Q << std::endl;
    }
    close(fd);

    return 0;
}

