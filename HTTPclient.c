#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

void uso() {
    printf("Uso: ./HTTPclient -h <host> <metodo> <ruta> [-d \"datos\"]\n");
    printf("Ejemplo: ./HTTPclient -h localhost:8080 GET /index.html\n");
    printf("         ./HTTPclient -h localhost:8080 POST /archivo -d \"nombre=Cami\"\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        uso();
    }

    char *host = NULL;
    char *method = NULL;
    char *path = NULL;
    char *data = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (!method) {
            method = argv[i];
        } else if (!path) {
            path = argv[i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            data = argv[++i];
        }
    }

    if (!host || !method || !path) {
        uso();
    }

    char url[1024];
    snprintf(url, sizeof(url), "http://%s%s", host, path);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error al inicializar curl\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

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
    } else if (strcmp(method, "GET") == 0) {
        // por defecto ya es GET
    } else {
        fprintf(stderr, "Método HTTP no soportado: %s\n", method);
        return 1;
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Error en curl: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return 0;
}
