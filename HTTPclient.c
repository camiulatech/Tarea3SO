#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

void uso() {
    printf("Usage: ./HTTPclient -h <host> <method> <path> [-d \"data\"] [-o output_file]\n");
    printf("Example: ./HTTPclient -h localhost:8080 GET /file.txt -o downloaded.txt\n");
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
    if (argc < 5) uso();

    char *host = NULL;
    char *method = NULL;
    char *path = NULL;
    char *data = NULL;
    char *output_file = NULL;
    FILE *fp = NULL;
    long filesize = 0;

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

    if (!host || !method || !path) uso();

    char url[1024];
    snprintf(url, sizeof(url), "http://%s%s", host, path);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
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
        if (data) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        } else if (file_path) {
            fp = fopen(file_path, "rb");
            if (!fp) {
                fprintf(stderr, "No se pudo abrir el archivo '%s'\n", file_path);
                curl_easy_cleanup(curl);
                return 1;
            }
    
            fseek(fp, 0L, SEEK_END);
            filesize = ftell(fp);
            rewind(fp);
    
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READDATA, fp);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)filesize);

            struct curl_slist *headers = NULL;
            char content_type[256];
            snprintf(content_type, sizeof(content_type), "Content-Type: %s", get_content_type(file_path));
            headers = curl_slist_append(headers, content_type);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        } else {
            fprintf(stderr, "PUT requiere -d o -f.\n");
            curl_easy_cleanup(curl);
            return 1;
        }    
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (strcmp(method, "HEAD") == 0) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (strcmp(method, "GET") != 0) {
        fprintf(stderr, "Unsupported HTTP method: %s\n", method);
        fprintf(stderr, "Valid methods: GET, POST, PUT, DELETE, HEAD\n");
        return 1;
    }

    CURLcode res = curl_easy_perform(curl);
    if (fp) {
        fclose(fp);
    }    
    if (res != CURLE_OK) {
        fprintf(stderr, "Curl error: %s\n", curl_easy_strerror(res));
        return 1;
    }

    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        printf("HTTP code: %ld\n", http_code);

        if (output_file && http_code == 200 && strcmp(method, "GET") == 0) {
            FILE *f = fopen(output_file, "w");
            if (f) {
                fwrite(chunk.response, 1, chunk.size, f);
                fclose(f);
                printf("File saved to '%s'\n", output_file);
            } else {
                printf("Could not save the file to '%s'\n", output_file);
            }
        } else if (chunk.response && chunk.size > 0) {
            if (chunk.size < 10000) {
                printf("Server response:\n%s\n", chunk.response);
            } else {
                printf("File received (%.2f MB), not printed due to size.\n",
                       chunk.size / (1024.0 * 1024.0));
        }
    } else {
        fprintf(stderr, "Curl error: %s\n", curl_easy_strerror(res));
    }

    free(chunk.response);
    curl_easy_cleanup(curl);
    return 0;
    }
}