#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>        // For close()
#include <arpa/inet.h>     // For socket functions
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctype.h>         // For toupper()

#define PORT 5000   //50000 for mac
#define BUFFER_SIZE 1024

void clearInputBuffer();

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    int valread;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Define server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to server!\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(sock, buffer, BUFFER_SIZE);

        if (valread == 0) {
            printf("Server closed the connection.\n");
            break;
        }

        printf("%s", buffer);

        // Check if server is prompting for input
        if (strstr(buffer, "Enter") != NULL) {
            // Read input from user
            fgets(buffer, BUFFER_SIZE, stdin);
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}