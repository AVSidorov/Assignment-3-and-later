#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define PORT 9000
#define MAX_CLIENTS 5
#define BUF_SIZE 1024
#define FILENAME "/var/tmp/aesdsocketdata"

static volatile int running = 1;
static volatile int file_fd = -1, server_fd = -1, client_fd = -1;


void signal_handler(int sig)
{
    if (file_fd >= 0)
        close(file_fd);

    if (client_fd >= 0)
        close(client_fd);

    if (server_fd >= 0)
        close(server_fd);

     if (unlink(FILENAME))
        syslog(LOG_ERR, "%s: %m", "Error delete file");


    if (sig == SIGINT || sig == SIGTERM){
        syslog(LOG_DEBUG,"%s", "Caught signal, exiting");
        running = 0;
        exit(EXIT_SUCCESS);
    }
    if (sig == -1){
        exit(EXIT_FAILURE);
    }
}

void log_error(const char* message) {
    syslog(LOG_ERR, "%s: %m", message);
}

int main(int argc, char * argv[]){
    int  opt = 1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

     // Open syslog with LOG_USER facility
    openlog(NULL, 0, LOG_USER);


    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        log_error("Failed to create socket");
        return -1;
    }

    // Set socket options to reuse address and enable keepalive
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_KEEPALIVE, &opt, sizeof(opt)) == -1)
    {
        log_error("Failed to set socket options");
        close(server_fd);
        return -1;
    }


    // Set the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        log_error("Bind failed");
        close(server_fd);
        return -1;
    }

    if (argc>1){
        if (strcmp(argv[1], "-d") == 0){
            int pid = fork();
            if (pid == -1){
                log_error("fork");
                close(server_fd);
                return -1;
            }
            else if (pid != 0)
                exit(EXIT_SUCCESS);

            // Daemon section
            if (pid == 0){

                if (setsid() == -1){
                    log_error("Create new session");
                    close(server_fd);
                    return -1;
                }

                if ( chdir("/") == -1){
                    log_error("Chdir to /");
                    close(server_fd);
                    return -1;
                }

                // redirect stdout stdin stderr
                for (int i=0; i<3; i++)
                    close(i);
                open("/dev/null", O_RDWR);
                dup(0);
                dup(0);
            }
        }
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) == -1)
    {
        log_error("Failed to listen for incoming connections");
        close(server_fd);
        return -1;
    }

    // Set signal handlers for SIGINT and SIGTERM
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Loop forever accepting incoming connections
    ssize_t bytes_read=0;
    while (running){
        // Accept a new client connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len)) < 0) {
            log_error("Accept failed");
            continue;
        }

        // Log connection details to syslog
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        syslog(LOG_DEBUG, "Accepted connection from %s", client_ip);

        // Read data from the client connection
        while ((bytes_read = recv(client_fd, buffer, BUF_SIZE, 0)) > 0) {
            if (bytes_read == -1){
                log_error("recv");
                signal_handler(-1);
            }
            // Append the data to the file
            // Open if closed
            if (file_fd < 0) {
                file_fd = open(FILENAME, O_CREAT | O_RDWR | O_APPEND, 0644);
                if (file_fd < 0) {
                    log_error("Failed to open data file");
                    signal_handler(-1);
                }
            }
            // write to file
            // TODO check error and partial write
            write(file_fd, &buffer, bytes_read);
            // exit from reading loop if
            if (buffer[bytes_read-1] == '\n')
                break;
        }

        // Sending answer
        if (lseek(file_fd, (off_t) 0, SEEK_SET) != (off_t)  0){
            log_error("Fail to seek to file start");
            signal_handler(-1);
        }

        int bytes_send;
        while ((bytes_read = read(file_fd, &buffer, BUF_SIZE)) > 0){
            // TODO check error and partial send
            while ((bytes_send = send(client_fd, &buffer, bytes_read, 0)) < bytes_read){
                log_error("Fail send");
                signal_handler(-1);
            }
            ;
        }

        // Close connnection and log details to syslog
        close(file_fd);
        file_fd = -1;
        close(client_fd);
        client_fd = -1;
        syslog(LOG_DEBUG, "Closed connection from %s", client_ip);
    }
    exit(EXIT_SUCCESS);
}




