#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(){

    int fd;
    char buf[6];
    fd = open("textForSimpleLSeekTest.txt", O_RDWR);
    read(fd, buf, 6);
    printf("%s\n", buf);

    lseek(fd, 5, SEEK_SET);
    read(fd, buf, 6);
    printf("%s\n", buf);  

    lseek(fd, 2, SEEK_SET);
    read(fd, buf, 6);
    printf("%s\n", buf);  
}

// in text file: 

    // abcdefgh123456789

// SEEK_CUR - skip n items AFTER THE LAST READ
    // [abcdef]gh123[456789]
    /*
        abcdef
        456789
    */
// SEEK_SET - offset the first n (in this case 5)
    // [abcde<f]gh123>456789
    //      ^
    // offsets n elements, and read starts from the next element
    /*
        abcdef
        fgh123
    */

// SEEK_END - ???
    /*
        abcdef
        abcdef
    */


// If lseek is removed/ commented, each read call just reads the next buf_size items