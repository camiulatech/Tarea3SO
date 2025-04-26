#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 4096

int server_fd;
int port = DEFAULT_PORT;
char *root_dir = ".";

int num_threads = 4;
int clientes_activos = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void guardar_body_binario(int client_fd, int file_fd, char *cabecera, int cabecera_len) {
    int content_length = 0;
    char *len_ptr = strcasestr(cabecera, "Content-Length:");
    if (len_ptr) {
        sscanf(len_ptr, "Content-Length: %d", &content_length);
    }

    char *body = strstr(cabecera, "\r\n\r\n");
    if (!body || content_length <= 0) return;

    body += 4;
    int header_bytes = body - cabecera;
    int body_en_buffer = cabecera_len - header_bytes;

    // Escribimos solo lo que ya está en el buffer
    write(file_fd, body, body_en_buffer);

    // Leer el resto
    int remaining = content_length - body_en_buffer;
    char temp_buffer[BUFFER_SIZE];
    while (remaining > 0) {
        int bytes = read(client_fd, temp_buffer, (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE);
        if (bytes <= 0) break;
        write(file_fd, temp_buffer, bytes);
        remaining -= bytes;
    }
}

void *handle_client(void *arg) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // Verificar si hay hilos disponibles
        pthread_mutex_lock(&lock);
        if (clientes_activos >= num_threads) {
            pthread_mutex_unlock(&lock);
            char msg[] =
                "HTTP/1.1 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Servidor ocupado. Se ha sobrepasado el número de clientes que pueden ser atendidos.\n";
            write(client_fd, msg, strlen(msg));
            printf("%s\n", msg); // Imprimir también en consola
            close(client_fd);
            continue;
        }
        clientes_activos++;
        pthread_mutex_unlock(&lock);

        // Verificar si el puerto corresponde a HTTP
        if (port != 80 && port != 8080){
            char protocolo[64];
            switch (port) {
                case 21: 
                case 2121: strcpy(protocolo, "FTP"); break;
                case 22: 
                case 2222: strcpy(protocolo, "SSH"); break;
                case 25: 
                case 2525: strcpy(protocolo, "SMTP"); break;
                case 53: 
                case 5353: strcpy(protocolo, "DNS"); break;
                case 23: 
                case 2323: strcpy(protocolo, "TELNET"); break;
                case 161: 
                case 16161: strcpy(protocolo, "SNMP"); break;
                default: strcpy(protocolo, "Desconocido"); break;
            }

            char respuesta[256];
            snprintf(respuesta, sizeof(respuesta),
                "HTTP/1.1 501 Not Implemented\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Este servidor no implementa el protocolo %s. Solo se admite HTTP/1.1 por el puerto 8080.\n",
                protocolo);

            write(client_fd, respuesta, strlen(respuesta));
            printf("%s\n", respuesta);
            close(client_fd);
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        read(client_fd, buffer, BUFFER_SIZE);
        printf("Solicitud: %s\n", buffer);

        // Extraer método y ruta
        char method[8], path[1024];
        sscanf(buffer, "%s %s", method, path);

        char *filename = path + 1;
        if (strlen(filename) == 0) filename = "index.html";

        char fullpath[2048];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", root_dir, filename);

        if (strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0) {
            int file_fd = open(fullpath, O_RDONLY);
            if (file_fd == -1) {
                char error_msg[] =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "Archivo no encontrado";
                write(client_fd, error_msg, strlen(error_msg));
                printf("%s\n", error_msg);
            } else {
                char header[] =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n";
                write(client_fd, header, strlen(header));
                printf("%s\n", header);

                if (strcmp(method, "GET") == 0) {
                    int bytes;
                    while ((bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
                        write(client_fd, buffer, bytes);
                    }
                }
                close(file_fd);
            }

        } else if (strcmp(method, "POST") == 0) {
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) {
                body += 4;
            }

            char response[] =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Datos recibidos correctamente:";
            write(client_fd, response, strlen(response));
            printf("%s %s\n", response, body);

        } else if (strcmp(method, "PUT") == 0) {
            int file_fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (file_fd == -1) {
                char error_msg[] =
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "No se pudo crear el archivo";
                write(client_fd, error_msg, strlen(error_msg));
                printf("%s\n", error_msg);
            } else {
                guardar_body_binario(client_fd, file_fd, buffer, BUFFER_SIZE);
                close(file_fd);
        
                char response[] =
                    "HTTP/1.1 201 Created\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "Archivo creado exitosamente";
                write(client_fd, response, strlen(response));
                printf("%s\n", response);
            }        
        } else if (strcmp(method, "DELETE") == 0) {
            if (unlink(fullpath) == 0) {
                char response[] =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "Archivo eliminado exitosamente";
                write(client_fd, response, strlen(response));
                printf("%s\n", response);
            } else {
                char error_msg[] =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "No se pudo eliminar el archivo";
                write(client_fd, error_msg, strlen(error_msg));
                printf("%s\n", error_msg);
            }

        } else {
            char not_implemented[] =
                "HTTP/1.1 501 Not Implemented\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Método no soportado";
            write(client_fd, not_implemented, strlen(not_implemented));
            printf("%s\n", not_implemented);
        }

        close(client_fd);

        pthread_mutex_lock(&lock);
        clientes_activos--;
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            root_dir = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
    }

    struct sockaddr_in server_addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en puerto %d con %d hilos...\n", port, num_threads);
    printf("Sirviendo archivos desde: %s\n", root_dir);

    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, handle_client, NULL);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    close(server_fd);
    return 0;
}
