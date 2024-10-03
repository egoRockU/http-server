#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define PORT 3030

int checkWSAStartup (WSADATA wsaData);
void* sendData (int client_socket, char* route);
char* checkContentType(char* filename);
char* getFileExtension(char* filename);
void* sendDocument (int client_socket, char* route, long file_size, char* buffer, size_t bytesRead);
void* sendBinary (int client_socket, char* route, long file_size, char* buffer, size_t bytesRead);

int main() {
    WSADATA wsaData;
    checkWSAStartup(wsaData);

    // create socket file descriptor
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    // specifying an address for the server socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    int bindResult = bind(socketFD, (struct sockaddr*) &server_address, sizeof(server_address));

    int listenResult = listen(socketFD, 10);

    if (listenResult == 0) {
        printf("Webserver is listening on http://localhost:%d\n", PORT);
    } else {
        printf("Error: Unable to listen on the socket. Error code: %d\n", WSAGetLastError());
        return 0;
    }

    while(true) {
        int client_size = sizeof(struct sockaddr_in);
        int client_socket = accept(socketFD, (struct sockaddr*)&client_size, &client_size);

        char buffer[1024];
        int recieved = recv(client_socket, buffer, sizeof(buffer), 0);
        buffer[recieved] = '\0';

        char route[256];
        sscanf(buffer, "GET %s HTTP/1.1", route);

        sendData(client_socket, route);
    }
    
    shutdown(socketFD, SD_BOTH);

    WSACleanup();
    return 0;
}

int checkWSAStartup (WSADATA wsaData) {
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }
}

void* sendData (int client_socket, char* route) {
    FILE* fptr;
    char* buffer;
    long file_size;
    char* fileExt = getFileExtension(route);
    char fileToFetch[256];
    char* mode = "r";

    if (strcmp(route, "/") == 0){
        fptr = fopen("files/index.html", "r");
        if (fptr == NULL){
            printf("Error: Could not open file\n");
            return NULL;
        }
    } else {
        if (strcmp(fileExt, "ico") == 0 
        || strcmp(fileExt, "png") == 0
        || strcmp(fileExt, "jpeg") == 0
        || strcmp(fileExt, "jpg") == 0)
        {
            mode = "rb";
        }
        sprintf(fileToFetch, "files%s", route);
        fptr = fopen(fileToFetch, mode);
        if (fptr == NULL){
            fptr = fopen("files/notFound.html", "r");
            printf("Error: Could not open file\n");
        }
    }

    fseek(fptr, 0, SEEK_END);
    file_size = ftell(fptr);
    rewind(fptr);

    buffer = (char *)malloc((file_size + 1) * sizeof(char));
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        fclose(fptr);
        return NULL;
    }
    size_t bytesRead = fread(buffer, sizeof(char), file_size, fptr);

    if (strcmp(mode, "r") == 0){
        sendDocument(client_socket, route, file_size, buffer, bytesRead);
    } else if (strcmp(mode, "rb") == 0) {
        sendBinary(client_socket, route, file_size, buffer, bytesRead);
    }

    fclose(fptr);
}

char* checkContentType(char* filename) {
    char* extension = getFileExtension(filename);

    if (strcmp(extension, "html") == 0) {
        return "text/html";
    } else if(strcmp(extension, "css") == 0){
        return "text/css";
    } else if(strcmp(extension, "ico") == 0) {
        return "image/x-icon";
    } else if (strcmp(extension, "png") == 0 
            || strcmp(extension, "jpeg") == 0 
            || strcmp(extension, "jpg") == 0) 
    {
        return "image/png";
    }
    return "text/html";
}

char* getFileExtension(char* filename) {
    filename = filename + 1;
    char* extension = strrchr(filename, '.');

    if (!extension || extension == filename || *(extension + 1) == '\0') {
        extension =  "";
    }

    extension = extension + 1;
    return extension;
}

void* sendDocument (int client_socket, char* route, long file_size, char* buffer, size_t bytesRead) {
    char message[1024];
    char* contentType = checkContentType(route);
    int messageLen = snprintf(message, sizeof(message), "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n", contentType, bytesRead);

    buffer[bytesRead] = '\0';
    strcat(message, buffer);
    send(client_socket, message, strlen(message) + bytesRead, 0);
}

void* sendBinary (int client_socket, char* route, long file_size, char* buffer, size_t bytesRead) {
    char header[1024];
    char* contentType = checkContentType(route);
    int headerLen = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "\0", contentType, file_size);

    send(client_socket, header, strlen(header), 0); 
    send(client_socket, buffer, bytesRead, 0);
}