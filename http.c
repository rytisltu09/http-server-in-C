#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8000

void log_request(const char *method, const char *path);
void determine_file_type(const char *path, char *content_type, size_t size);
void create_thread(void *(*start_routine)(void *), void *arg);
void *handle_client(void *arg);
void parse_received(char *buffer, char *method, char *path);
void send_response(int client_fd, const char *status, const char *content_type, const char *body, size_t body_len);
void serve_file(int client_fd, const char *file_path, const char *uri_path);

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }
    printf("Created a socket!\n");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Binding the socket...\n");
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("binding");
        close(server_fd);
        return EXIT_FAILURE;
    }
    printf("Bound successfully!\n");

    printf("Listening on port %d...\n", PORT);
    if (listen(server_fd, 10) == -1) {
        perror("listening");
        close(server_fd);
        return EXIT_FAILURE;
    }

    while (1) {
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

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        if (bytes_received < 0) perror("recv");
        close(client_fd);
        return NULL;
    }

    buffer[bytes_received] = '\0';

    char method[32] = {0};
    char path[256] = {0};

    parse_received(buffer, method, path);
    printf("Request: %s %s\n", method, path);
    log_request(method, path);

    // Route request
    if (strcmp(path, "/") == 0 || strcmp(path, "/home") == 0) {
        serve_file(client_fd, "public/index.html", "public/index.html");
    } else if (strcmp(path, "/about") == 0) {
        serve_file(client_fd, "public/about.html", "public/about.html");
    } else {
        const char *not_found_msg = "404 Not Found\n";
        send_response(client_fd, "404 Not Found", "text/plain", not_found_msg, strlen(not_found_msg));
    }

    close(client_fd);
    return NULL;
}

void parse_received(char *buffer, char *method, char *path) {
    if (sscanf(buffer, "%31s %255s", method, path) != 2) {
        strcpy(method, "");
        strcpy(path, "");
    }
}

void serve_file(int client_fd, const char *file_path, const char *uri_path) {
    char content_type[64];
    determine_file_type(uri_path, content_type, sizeof(content_type));

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        const char *err_msg = "Could not open requested file.\n";
        send_response(client_fd, "500 Internal Server Error", "text/plain", err_msg, strlen(err_msg));
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *body = malloc(file_size);
    if (body == NULL) {
        const char *err_msg = "Memory allocation failed.\n";
        send_response(client_fd, "500 Internal Server Error", "text/plain", err_msg, strlen(err_msg));
        fclose(file);
        return;
    }

    size_t n = fread(body, 1, file_size, file);
    send_response(client_fd, "200 OK", content_type, body, n);

    fclose(file);
    free(body);
}

void send_response(int client_fd, const char *status, const char *content_type, const char *body, size_t body_len) {
    char header[512];
    int header_len = snprintf(
        header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, content_type, body_len
    );

    // Send HTTP header
    if (send(client_fd, header, header_len, 0) == -1) {
        perror("send header");
        return;
    }

    // Send HTTP body
    if (send(client_fd, body, body_len, 0) == -1) {
        perror("send body");
    }
}

void create_thread(void *(*start_routine)(void *), void *arg) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, start_routine, arg) != 0) {
        perror("pthread_create");
        free(arg);
        return;
    }
    pthread_detach(thread);
}

void determine_file_type(const char *path, char *content_type, size_t size) {
    if (strstr(path, ".html") != NULL) {
        snprintf(content_type, size, "text/html");
    } else if (strstr(path, ".css") != NULL) {
        snprintf(content_type, size, "text/css");
    } else if (strstr(path, ".js") != NULL) {
        snprintf(content_type, size, "application/javascript");
    } else if (strstr(path, ".png") != NULL) {
        snprintf(content_type, size, "image/png");
    } else if (strstr(path, ".jpg") != NULL || strstr(path, ".jpeg") != NULL) {
        snprintf(content_type, size, "image/jpeg");
    } else {
        snprintf(content_type, size, "text/plain");
    }
}

void log_request(const char *method, const char *path) {
    FILE *log_file = fopen("server.log", "a");
    if (log_file == NULL) {
        perror("fopen");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log_file, "[%s] %s %s\n", time_str, method, path);
    fclose(log_file);
}