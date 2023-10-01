#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char **argv) {

    struct termios old_tio, new_tio;
    char c;

    
    FILE *fp;
    fp = fopen(argv[1], "wb");
    printf("Enter file_size: ");
    int size = 0;
    scanf("%d", &size);
    
    // current terminal setting
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;

    // disable enter and display
    new_tio.c_lflag &= (~ICANON & ~ECHO);

    // baudrate, data 8 byte, direct connect device, for read
    //options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;

    // ignore parity error
    //sim->options.c_iflag = IGNPAR;


    tcflush(STDIN_FILENO, TCIFLUSH);

    // new terminal setting
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    while (size--) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
         
            fwrite(&c, sizeof(char), 1, fp);
        }
    }

    fclose(fp);
    
    // restore setting 
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

   
    printf("Transfer finished.\n");
}