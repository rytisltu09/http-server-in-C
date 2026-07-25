#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8000

struct sockaddr_in server_addr = {0};

void create_thread(void *(*start_routine)(void *), void *arg);
void *handle_client(void *arg);
void parse_received(char *buffer, char *method, char *path);
void send_response(int client_fd, char *status, char *body);

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

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        perror("setsockopt");
        return EXIT_FAILURE;
    }

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
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        int *client_fd_ptr = malloc(sizeof(int));
        if (client_fd_ptr == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }
        *client_fd_ptr = client_fd;

        create_thread(handle_client, client_fd_ptr);
    }
    close(server_fd);
    return 0;
}

void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0){
        perror("recv");
        close(client_fd);
        return NULL;
    }

    buffer[bytes_received] = '\0';

    char method[32];
    char path[256];

    parse_received(buffer, method, path);

    printf("%s %s\n", method, path);

    if (strcmp(path, "/") == 0 || strcmp(path, "/home") == 0){
        FILE *file = fopen("public/index.html", "r");
        if (file){
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);
            char *body = malloc(file_size + 1);
            if (body == NULL) {
                send_response(client_fd, "500 Internal Server Error", "Memory allocation failed\n");
                fclose(file);
                close(client_fd);
                return NULL;
            }
            size_t n = fread(body, 1, file_size, file);
            body[n] = '\0';
            send_response(client_fd, "200 OK", body);
            fclose(file);
            free(body);
        } else {
            send_response(client_fd, "500 Internal Server Error", "Could not open index.html\n");
        }
    }
    else if (strcmp(path, "/about") == 0){
        FILE *file = fopen("public/about.html", "r");
        if (file){
            fseek (file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);
            char *body = malloc(file_size + 1);
            if (body == NULL) {
                send_response(client_fd, "500 Internal Server Error", "Memory allocation failed\n");
                fclose(file);
                close(client_fd);
                return NULL;
            }

            size_t n = fread(body, 1, file_size, file);
            body[n] = '\0';
            send_response(client_fd, "200 OK", body);
            fclose(file);
            free(body);
        } else {
            send_response(client_fd, "500 Internal Server Error", "Could not open about.html\n");
        }
    }
    else{
        send_response(client_fd, "404 Not Found", "404\n");
    }

    close(client_fd);
    return NULL;
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

void create_thread(void *(*start_routine)(void *), void *arg)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, start_routine, arg) != 0){
        perror("pthread_create");
        free(arg);
        return;
    }
    pthread_detach(thread);
}