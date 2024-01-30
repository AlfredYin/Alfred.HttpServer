#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc,char *argv[]) {
    const char *hostname = "127.0.0.1";
    const char *path;
    
    if(argc!=2){
        path = "/";
    }
    else{
        path = argv[1];
    }
    
    
    const int port = 8080;

    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Error: Could not resolve hostname\n");
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    struct sockaddr_in server_addr;
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        error("Error connecting to server");
    }

    // // 构建 Get请求
    // char request[BUFFER_SIZE];
    // sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, hostname);

    // if (write(sockfd, request, strlen(request)) < 0) {
    //     error("Error writing to socket");
    // }

    // 构建POST请求
    const char *post_data = "param1=value1&param2=value2";
    char request[BUFFER_SIZE];
    sprintf(request, "POST %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Content-Type: application/x-www-form-urlencoded\r\n"
                     "Content-Length: %ld\r\n\r\n%s",
            path, hostname, strlen(post_data), post_data);

    if (write(sockfd, request, strlen(request)) < 0) {
        error("Error writing to socket");
    }

    char response[BUFFER_SIZE];
    int bytesRead;

    while ((bytesRead = read(sockfd, response, BUFFER_SIZE - 1)) > 0) {
        response[bytesRead] = '\0';
        printf("%s", response);
    }

    if (bytesRead < 0) {
        error("Error reading from socket");
    }

    close(sockfd);

    return 0;
}

