#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 8000

struct sockaddr_in server_addr = {0};

void handle_client(int server_fd);
void parse_received(char *buffer, char *method, char *path);
void send_response(int client_fd, char *status, char *body);

int main(){
    // Initialize server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        perror("socket");
        return EXIT_FAILURE;
    }
    printf("Created a socket!\n");
    // Define port, family, address
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Binding the socket\n");
    // Bind the socket
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
        handle_client(server_fd);
    }
    close(server_fd);
}

void handle_client(int server_fd)
{
    int client_fd = accept(server_fd, NULL, NULL);

    if (client_fd == -1){
        perror("accept");
        return;
    }

    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0){
        perror("recv");
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';

    char method[32];
    char path[256];

    parse_received(buffer, method, path);

    printf("%s %s\n", method, path);

    if (strcmp(path, "/") == 0 || strcmp(path, "/home") == 0){
        FILE *file = fopen("public/index.html", "r");
        if (file){
            char body[1024];
            size_t n = fread(body, 1, sizeof(body) - 1, file);
            body[n] = '\0';
            send_response(client_fd, "200 OK", body);
            fclose(file);
        } else {
            send_response(client_fd, "500 Internal Server Error", "Could not open index.html\n");
        }
    }
    else if (strcmp(path, "/about") == 0){
        FILE *file = fopen("public/about.html", "r");
        if (file){
            char body[1024];
            size_t n = fread(body, 1, sizeof(body) - 1, file);
            body[n] = '\0';
            send_response(client_fd, "200 OK", body);
            fclose(file);
        } else {
            send_response(client_fd, "500 Internal Server Error", "Could not open about.html\n");
        }
    }
    else{
        send_response(client_fd, "404 Not Found", "404\n");
    }

    close(client_fd);
}

void parse_received(char *buffer, char *method, char *path)
{
    if (sscanf(buffer, "%31s %255s", method, path) != 2){
        strcpy(method, "");
        strcpy(path, "");
    }
}
void send_response(int client_fd, char *status, char *body)
{
    char response[1024];

    snprintf(
        response,
        sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "%s",
        status,
        body
    );

    if (send(client_fd, response, strlen(response), 0) == -1){
        perror("send");
    }
}

