#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    char buffer[1024];
    int ret=0;
    memset(buffer,0,1024);

    if (argc != 3) {
        return 1;
    }

    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
    const char *writefile = argv[1];
    const char *writestr = argv[2];
    
    int fd = open(writefile, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Error: Could not create or open the file %s.\n", writefile);
        return 1;
    }
    
    ret=write(fd, writestr, strlen(writestr));
    if (ret == -1) {       
        syslog(LOG_ERR, "Error writing in file %s.\n", writefile);
        close(fd); // Tentativo di chiudere comunque il file
        return 1;
    }  
    close(fd);
    syslog(LOG_DEBUG, "Writing %s to %s\n",writestr,writefile);
    closelog();

    return 0;
}
