#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    char *ipaddr="127.0.0.1";

    // Replace this with the server's IP address (e.g., "192.168.1.100")
    if (inet_pton(AF_INET, ipaddr, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Notify server of connection
    sendto(sockfd, "", 1, 0, (const struct sockaddr *)&server_addr, addr_len);

    while (1)
    {
        bzero(buffer, sizeof(buffer));
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
        printf("%s", buffer);

        if (strstr(buffer, "Your move"))
        {

            printf("Enter your move (row col): ");

            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strlen(buffer) - 1] = 0;

            sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, addr_len);
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
                memset(buffer, 0, sizeof(buffer));
            }
            sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, addr_len);
        }

        if (strstr(buffer, "Exiting"))
        {
            break;
        }
    }

    close(sockfd);
    return 0;
}
