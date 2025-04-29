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

// Definiciones de constantes
#define DEFAULT_PORT 8080
#define BUFFER_SIZE 4096

// Variables globales
int server_fd;
int port = DEFAULT_PORT;
char *root_dir = ".";

int num_threads = 4;
int active_clients = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Función para obtener el tipo de contenido basado en la extensión del archivo
// Asume que es text/plain si no se encuentra la extensión
const char* get_content_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "text/plain";

    ext++;
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) return "text/html";
    if (strcasecmp(ext, "txt") == 0) return "text/plain";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "css") == 0) return "text/css";
    if (strcasecmp(ext, "js") == 0) return "application/javascript";
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(ext, "zip") == 0) return "application/zip";
    if (strcasecmp(ext, "xci") == 0) return "application/octet-stream";
    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(ext, "xml") == 0) return "application/xml";
    if (strcasecmp(ext, "mp4") == 0) return "video/mp4";
    if (strcasecmp(ext, "mp3") == 0) return "audio/mpeg";
    if (strcasecmp(ext, "wav") == 0) return "audio/wav";
    if (strcasecmp(ext, "avi") == 0) return "video/x-msvideo";
    if (strcasecmp(ext, "flv") == 0) return "video/x-flv";
    if (strcasecmp(ext, "mkv") == 0) return "video/x-matroska";
    if (strcasecmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcasecmp(ext, "ico") == 0) return "image/x-icon";
    if (strcasecmp(ext, "woff") == 0) return "font/woff";
    if (strcasecmp(ext, "woff2") == 0) return "font/woff2";
    if (strcasecmp(ext, "ttf") == 0) return "font/ttf";
    if (strcasecmp(ext, "otf") == 0) return "font/otf";
    if (strcasecmp(ext, "eot") == 0) return "font/eot";
    if (strcasecmp(ext, "csv") == 0) return "text/csv";
    return "application/octet-stream";
}

// Función para guardar el cuerpo binario de la solicitud POST
// Lee el cuerpo del mensaje y lo guarda en un archivo
void save_binary_body(int client_fd, int file_fd, char *header, int header_len) {
    int content_length = 0;

    // Buscar el Content-Length en el encabezado
    char *len_ptr = strcasestr(header, "Content-Length:");
    if (len_ptr) {
        sscanf(len_ptr, "Content-Length: %d", &content_length);
    }

    // Si no se encuentra Content-Length o es menor o igual a 0, no hacemos nada
    char *body = strstr(header, "\r\n\r\n");
    if (!body || content_length <= 0) return;

    // Avanzar el puntero del cuerpo para omitir los encabezados
    body += 4;
    int header_bytes = body - header;
    int body_en_buffer = header_len - header_bytes;

    // Escribir solo lo que ya está en el buffer
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

// Función para manejar las solicitudes de los clientes, un hilo por cliente
void *handle_client(void *arg) {

    // Crear un socket para el cliente
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Aceptar conexiones de clientes
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // Verificar si hay hilos disponibles, en caso contrario, responder con 503
        pthread_mutex_lock(&lock);                                         // Bloquear el mutex
        if (active_clients >= num_threads) {
            pthread_mutex_unlock(&lock);
            char msg[] =
                "HTTP/1.1 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Server busy. Maximum number of clients exceeded.\n";
            write(client_fd, msg, strlen(msg));
            printf("%s\n", msg); // Imprimir también en consola
            close(client_fd);
            continue;
        }

        // Si hay hilos disponibles, aumentar el contador de clientes activos
        active_clients++;
        pthread_mutex_unlock(&lock);                                       // Desbloquear el mutex

        // Verificar si el puerto corresponde a HTTP
        if (port != 80 && port != 8080){
            char protocol[64];
            switch (port) {
                case 21: 
                case 2121: strcpy(protocol, "FTP"); break;
                case 22: 
                case 2222: strcpy(protocol, "SSH"); break;
                case 25: 
                case 2525: strcpy(protocol, "SMTP"); break;
                case 53: 
                case 5353: strcpy(protocol, "DNS"); break;
                case 23: 
                case 2323: strcpy(protocol, "TELNET"); break;
                case 161: 
                case 16161: strcpy(protocol, "SNMP"); break;
                default: strcpy(protocol, "Desconocido"); break;
            }

            // Responder con un mensaje de error en caso de que el puerto no sea HTTP
            char response[256];
            snprintf(response, sizeof(response),
                "HTTP/1.1 501 Not Implemented\r\n"
                "Connection: close\r\n\r\n"
                "This server does not implement protocol %s. Only HTTP/1.1 on port 8080 is supported.\n",
                protocol);

            write(client_fd, response, strlen(response));
            printf("%s\n", response);
            close(client_fd);
            continue;
        }

        // Leer la solicitud del cliente, limpiar el buffer antes de leer
        memset(buffer, 0, BUFFER_SIZE);
        read(client_fd, buffer, BUFFER_SIZE);
        printf("Request: %s\n", buffer);

        // Extraer método y ruta
        char method[8], path[1024];
        sscanf(buffer, "%s %s", method, path);

        // Verifica si se indicó un archivo, sino se usa index.html
        char *filename = path + 1;
        if (strlen(filename) == 0) filename = "index.html";

        char fullpath[2048];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", root_dir, filename); // Concatenar el directorio raíz con el nombre del archivo

        const char *content_type = get_content_type(filename);             // Obtener el tipo de contenido

        // Manejar diferentes métodos HTTP
        if (strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0) {
            int file_fd = open(fullpath, O_RDONLY);                        // Abrir el archivo para lectura

            // Si el archivo no se encuentra, responder con 404
            if (file_fd == -1) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: %s\r\n"
                    "Connection: close\r\n\r\n"
                    "File not found", content_type);
                write(client_fd, error_msg, strlen(error_msg));
                printf("%s\n", error_msg);
            } else {
                // Si el archivo se encuentra, envia el encabezado de respuesta
                char header[256];
                snprintf(header, sizeof(header),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: %s\r\n"
                    "Connection: close\r\n\r\n",
                    content_type);
                write(client_fd, header, strlen(header));
                printf("%s\n", header);

                // Si el método es GET, además de lo anterior, leer el archivo y enviarlo al cliente
                if (strcmp(method, "GET") == 0) {
                    int bytes;
                    while ((bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
                        write(client_fd, buffer, bytes);
                    }
                }
                close(file_fd);
            }

        } else if (strcmp(method, "POST") == 0) {
            // Enviar datos al servidor
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) {
                body += 4;
            }

            // Recibir la información desde el cliente y mostrarla
            char response[256 + strlen(body)];
            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Data received succesfully: %s", body);
            write(client_fd, response, strlen(response));
            printf("%s\n", response);

        } else if (strcmp(method, "PUT") == 0) {
            // Crear el archivo en el servidor
            int file_fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (file_fd == -1) {
                // Si no se puede crear el archivo, responder con 500
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: %s\r\n"
                    "Connection: close\r\n\r\n"
                    "Could not create file", content_type);
                write(client_fd, error_msg, strlen(error_msg));
                printf("%s\n", error_msg);
            } else {
                // Guardar el cuerpo del mensaje en el archivo, donde el cuerpo es el contenido del archivo
                save_binary_body(client_fd, file_fd, buffer, strlen(buffer));
                close(file_fd);
                char response[256];
                snprintf(response, sizeof(response),
                    "HTTP/1.1 201 Created\r\n"
                    "Content-Type: %s\r\n"
                    "Connection: close\r\n\r\n"
                    "File created successfully", content_type);
                write(client_fd, response, strlen(response));
                printf("%s\n", response);
            }        

        } else if (strcmp(method, "DELETE") == 0) {
            // Eliminar el archivo del servidor, si se logra eliminar, responder con 200, sino con 404
            if (unlink(fullpath) == 0) {
                char response[256];
                snprintf(response, sizeof(response),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: %s\r\n"
                    "Connection: close\r\n\r\n"
                    "File deleted successfully", content_type);
                write(client_fd, response, strlen(response));
                printf("%s\n", response);
            } else {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: %s\r\n"
                    "Connection: close\r\n\r\n"
                    "Could not delete the file", content_type);
                write(client_fd, error_msg, strlen(error_msg));
                printf("%s\n", error_msg);
            }

        } else {
            // Si el método no es soportado, responder con 501
            char not_implemented[256];
            snprintf(not_implemented, sizeof(not_implemented),
                "HTTP/1.1 501 Not Implemented\r\n"
                "Content-Type: %s\r\n"
                "Connection: close\r\n\r\n"
                "Method not supported", content_type);
            write(client_fd, not_implemented, strlen(not_implemented));
            printf("%s\n", not_implemented);
        }

        close(client_fd);                                                  // Cerrar el socket del cliente

        // Decrementar el contador de clientes activos
        pthread_mutex_lock(&lock);
        active_clients--;
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // Procesar argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            root_dir = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
    }

    // Crear el socket del servidor
    struct sockaddr_in server_addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bindear el socket a la dirección y puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Mostrar información del servidor
    printf("Server listening on port %d with %d threads...\n", port, num_threads);
    printf("Serving files from: %s\n", root_dir);

    // Crear hilos para manejar las solicitudes de los clientes
    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, handle_client, NULL);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Cerrar el socket del servidor
    close(server_fd);
    return 0;
}
