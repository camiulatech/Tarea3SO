#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

void usage() {
    printf("Usage: ./HTTPclient -h <host> <method> <path> [-d \"data\"] [-o output_file]\n");
    printf("Example: ./HTTPclient -h localhost:8080 GET /file.txt -o downloaded.txt\n");
    exit(1);
}

// Estructura para almacenar la respuesta
struct Memory {
    char *response;
    size_t size;
};

// Callback para manejar la respuesta del servidor
size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    // Calcular el tamaño total de los datos recibidos
    size_t realsize = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    // Reasignar memoria para almacenar los nuevos datos
    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) return 0; // sin memoria

    // Copiar los nuevos datos al final de la respuesta actual
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;                                   // Null-terminar la cadena

    return realsize;
}

// Función para obtener el tipo de contenido basado en la extensión del archivo
// Asume que es text/plain si no se encuentra la extensión
const char* get_content_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "text/plain";

    ext++; // Saltar el punto
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

char *file_path = NULL;

int main(int argc, char *argv[]) {
    if (argc < 5) usage();                                          // Verificar si se pasaron suficientes argumentos

    // Inicializar las variables
    char *host = NULL;
    char *method = NULL;
    char *path = NULL;
    char *data = NULL;
    char *output_file = NULL;
    FILE *fp = NULL;
    long filesize = 0;

    // Procesar los argumentos de la línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (!method) {
            method = argv[i];
        } else if (!path) {
            path = argv[i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            data = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            file_path = argv[++i];
        }        
    }

    if (!host || !method || !path) usage();                         // Verificar si se pasaron los argumentos obligatorios

    // Inicializar la URL
    char url[1024];
    snprintf(url, sizeof(url), "http://%s%s", host, path);

    // Inicializar libcurl
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }

    // Crear una estructura para almacenar la respuesta
    struct Memory chunk = { .response = NULL, .size = 0 };

    // Configurar las opciones de curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    // Actuar según el método HTTP
    if (strcmp(method, "POST") == 0) {
        // Si el método es POST, configurar los datos a enviar y enviarlos
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (data)
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    } else if (strcmp(method, "PUT") == 0) {
        // Si el método es PUT, configurar los datos a enviar
        if (data) {
            // Si se pasan datos, enviarlos
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        } else if (file_path) {
            // Si se pasa un archivo, subirlo
            // Abrir el archivo en modo binario
            fp = fopen(file_path, "rb");

            // Verificar si se pudo abrir el archivo
            if (!fp) {
                fprintf(stderr, "No se pudo abrir el archivo '%s'\n", file_path);
                curl_easy_cleanup(curl);
                return 1;
            }
    
            // Obtener el tamaño del archivo
            fseek(fp, 0L, SEEK_END);
            filesize = ftell(fp);
            rewind(fp);
    
            
            // Configurar las opciones de curl para realizar la subida del archivo
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READDATA, fp);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)filesize);

            // Configurar el encabezado Content-Type basado en la extensión del archivo
            struct curl_slist *headers = NULL;
            char content_type[256];
            snprintf(content_type, sizeof(content_type), "Content-Type: %s", get_content_type(file_path));
            headers = curl_slist_append(headers, content_type);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        } else {
            // Si no se pasan datos ni archivo, mostrar un mensaje de error
            fprintf(stderr, "PUT requiere -d o -f.\n");
            curl_easy_cleanup(curl);
            return 1;
        }    
    } else if (strcmp(method, "DELETE") == 0) {
        // Si el método es DELETE, configurar la opción correspondiente
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (strcmp(method, "HEAD") == 0) {
        // Si el método es HEAD, configurar la opción correspondiente
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (strcmp(method, "GET") != 0) {
        // Si el método no es soportado, mostrar un mensaje de error
        fprintf(stderr, "Unsupported HTTP method: %s\n", method);
        fprintf(stderr, "Valid methods: GET, POST, PUT, DELETE, HEAD\n");
        return 1;
    }

    // Ejecutar la solicitud
    CURLcode res = curl_easy_perform(curl);

    // Cerrar el archivo si se abrió
    if (fp) {
        fclose(fp);
    }

    // Si hubo un error al ejecutar la solicitud, mostrar un mensaje de error
    if (res != CURLE_OK) {
        fprintf(stderr, "Curl error: %s\n", curl_easy_strerror(res));
        return 1;
    }

    if (res == CURLE_OK) {
        // Obtener el código de respuesta HTTP
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        printf("HTTP code: %ld\n", http_code);

        // Si es un GET y se especifica un archivo de salida, guardar la respuesta en el archivo
        if (output_file && http_code == 200 && strcmp(method, "GET") == 0) {
            // Abrir el archivo de salida
            FILE *f = fopen(output_file, "w");

            // Verificar si se pudo abrir el archivo
            if (f) {
                // Guardar la respuesta en el archivo
                fwrite(chunk.response, 1, chunk.size, f);
                fclose(f);
                printf("File saved to '%s'\n", output_file);
            } else {
                // Si no se pudo abrir el archivo, mostrar un mensaje de error
                printf("Could not save the file to '%s'\n", output_file);
            }
        } else if (chunk.response && chunk.size > 0) {
            // Si la respuesta es muy grande, no imprimirla pero mostrar el tamaño
            if (chunk.size < 10000) {
                printf("Server response:\n%s\n", chunk.response);
            } else {
                printf("File received (%.2f MB), not printed due to size.\n",
                       chunk.size / (1024.0 * 1024.0));
        }
    } else {
        // Si hubo un error al ejecutar la solicitud, mostrar un mensaje de error
        fprintf(stderr, "Curl error: %s\n", curl_easy_strerror(res));
    }

    // Liberar la memoria de la respuesta y limpiar libcurl
    free(chunk.response);
    curl_easy_cleanup(curl);
    return 0;
    }
}