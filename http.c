#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 8000

struct sockaddr_in server_addr = {0};

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        perror("socket");
        return EXIT_FAILURE;
    }
    printf("Created a socket!\n");

    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Binding the socket\n");
    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("binding");
        return EXIT_FAILURE;
    }
    printf("Bound successfully!\n");
    printf("Listening...\n");
    if (listen(server_fd, 10) == -1){
        perror("listening");
        return EXIT_FAILURE;
    }

    while (1){
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1){
            perror("accept");
            continue;
        }
        printf("Client connected!\n");
        char buffer[1024];
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0){
            perror("recv");
        }
        else{
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer);
        }
        char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello from the server!\n";
        if (send(client_fd, response, strlen(response), 0) == -1){
            perror("send");
        }
        close(client_fd);
    }
    close(server_fd);
}