#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]){
    openlog(NULL,0,LOG_USER);
    char *msg;
    if (argc != 3){
        msg = "Invalid Number of arguments.";
        printf("%s\n", msg);
        syslog(LOG_ERR, "%s %d",msg, argc-1);
        return 1;
    }

    char *writefile = argv[1];
    char *writestr = argv[2];

    FILE *fp = fopen(writefile, "w");
    if (fp == NULL) {
        msg = "Error opening file";
        printf("%s  %s for writing.\n", msg, writefile);
        syslog(LOG_ERR, "%s  %s: %m", msg, writefile);
        return 1;
    }

    fputs(writestr, fp);
    fclose(fp);

    msg = "Writing";
    printf("%s \"%s\" to \"%s\"\n", msg, writestr, writefile);
    syslog(LOG_DEBUG,"%s \"%s\" to \"%s\"", msg, writestr, writefile);

    return 0;
}