#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

void uso() {
    printf("Uso: ./HTTPclient -h <host> <metodo> <ruta> [-d \"datos\"] [-o archivo_salida]\n");
    printf("Ejemplo: ./HTTPclient -h localhost:8080 GET /archivo.txt -o descargado.txt\n");
    exit(1);
}

struct Memory {
    char *response;
    size_t size;
};

size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) return 0; // sin memoria

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

int main(int argc, char *argv[]) {
    if (argc < 5) uso();

    char *host = NULL;
    char *method = NULL;
    char *path = NULL;
    char *data = NULL;
    char *output_file = NULL;

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
        }
    }

    if (!host || !method || !path) uso();

    char url[1024];
    snprintf(url, sizeof(url), "http://%s%s", host, path);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error al inicializar curl\n");
        return 1;
    }

    struct Memory chunk = { .response = NULL, .size = 0 };
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (data)
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    } else if (strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (data)
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (strcmp(method, "HEAD") == 0) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (strcmp(method, "GET") != 0) {
        fprintf(stderr, "Método HTTP no soportado: %s\n", method);
        return 1;
    }

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        printf("Código HTTP: %ld\n", http_code);

        if (output_file && http_code == 200 && strcmp(method, "GET") == 0) {
            FILE *f = fopen(output_file, "w");
            if (f) {
                fwrite(chunk.response, 1, chunk.size, f);
                fclose(f);
                printf("Archivo guardado en '%s'\n", output_file);
            } else {
                printf("No se pudo guardar el archivo en '%s'\n", output_file);
            }
        } else if (chunk.response && chunk.size > 0) {
            printf("Respuesta del servidor:\n%s\n", chunk.response);
        }
    } else {
        fprintf(stderr, "Error en curl: %s\n", curl_easy_strerror(res));
    }

    free(chunk.response);
    curl_easy_cleanup(curl);
    return 0;
}
