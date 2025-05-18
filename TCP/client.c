#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 2000
#define BUFFER_SIZE 1024


struct sockaddr_in *sockaddrMake(const char *server_ip){

    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));

    address->sin_family = AF_INET;
    address->sin_port = htons(PORT);

    // Set the server's IP address
    if (inet_pton(AF_INET, server_ip, &address->sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    return address;
}


int* socketMake(){
    int *server_fd = malloc(sizeof(int));
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_fd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}


int main()
{
    int *sock = socketMake();
    char *ipaddr="127.0.0.1";
    struct sockaddr_in *server_addr = sockaddrMake(ipaddr);
    char buffer[BUFFER_SIZE];


    // Connect to the server
    if (connect(*sock, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
    {
        perror("Connection failed");
        close(*sock);
        exit(EXIT_FAILURE);
    }

    // Main loop for communication
    while (1)
    {
        bzero(buffer, sizeof(buffer));
        recv(*sock, buffer, BUFFER_SIZE, 0);
        printf("%s", buffer);

        if (strstr(buffer, "Your move"))
        {
            printf("Enter your move: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strlen(buffer) - 1] = 0;
            send(*sock, buffer, strlen(buffer), 0);
        }
        else if (strstr(buffer, "Do you want to play again?"))
        {

            while (1)
            {

                fgets(buffer, BUFFER_SIZE, stdin);

                buffer[strlen(buffer) - 1] = 0;

                if (strcmp(buffer, "yes") == 0 || strcmp(buffer, "no") == 0)
                {
                    break;
                }
                else
                {
                    printf("Invalid statement!!\n");
                    printf("Enter 'yes' or 'no': ");
                }
                memset(buffer,0,sizeof(buffer));
            }
            send(*sock, buffer, strlen(buffer), 0);
        }

        if (strstr(buffer, "Ending"))
        {
            break;
        }
    }

    close(*sock);
    free(server_addr);
    return 0;
}
