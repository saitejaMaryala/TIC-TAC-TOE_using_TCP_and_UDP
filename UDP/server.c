#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BOARD_SIZE 3
#define BUFFER_SIZE 1024

char board[BOARD_SIZE][BOARD_SIZE];
int current_player = 1;

void reset_board()
{
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            board[i][j] = ' ';
}

void send_board(int sockfd, struct sockaddr_in *client1_addr, struct sockaddr_in *client2_addr)
{
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message),
             "\n %c | %c | %c\n---|---|---\n %c | %c | %c\n---|---|---\n %c | %c | %c\n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)client1_addr, sizeof(*client1_addr));
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)client2_addr, sizeof(*client2_addr));
}

int check_winner()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return board[i][0] == 'X' ? 1 : 2;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return board[0][i] == 'X' ? 1 : 2;
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return board[0][0] == 'X' ? 1 : 2;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return board[0][2] == 'X' ? 1 : 2;
    return 0;
}

int is_draw()
{
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] == ' ')
                return 0;
    return 1;
}

void handle_move(int sockfd, int player, struct sockaddr_in *client1_addr, struct sockaddr_in *client2_addr, socklen_t addr_len)
{
    int row, col;
    char move[BUFFER_SIZE];
    struct sockaddr_in *current_client = player == 1 ? client1_addr : client2_addr;

    while (1)
    {

        memset(move ,0,sizeof(move));

        // char flush_buffer[BUFFER_SIZE];
        // while (recvfrom(sockfd, flush_buffer, BUFFER_SIZE, 0, (struct sockaddr *)current_client, &addr_len) > 0)
        //     ;



        sendto(sockfd, "Your move (row col): ", 21, 0, (struct sockaddr *)current_client, addr_len);
        recvfrom(sockfd, move, BUFFER_SIZE, 0, (struct sockaddr *)current_client, &addr_len);

        sscanf(move, "%d %d", &row, &col);

        if (strlen(move) == 3)
        {

            if (row >= 1 && row <= 3 && col >= 1 && col <= 3 && board[row - 1][col - 1] == ' ')
            {
                board[row - 1][col - 1] = player == 1 ? 'X' : 'O';
                break;
            }
            else
            {
                sendto(sockfd, "Invalid move, try again.\n", 25, 0, (struct sockaddr *)current_client, addr_len);
            }
        }
        else
        {
            sendto(sockfd, "Invalid move, try again.\n", 25, 0, (struct sockaddr *)current_client, addr_len);
        }
    }

    send_board(sockfd, client1_addr, client2_addr);

    if (check_winner() == player)
    {
        char result[BUFFER_SIZE];
        snprintf(result, sizeof(result), "Player %d wins!\n", player);
        sendto(sockfd, result, strlen(result), 0, (struct sockaddr *)client1_addr, addr_len);
        sendto(sockfd, result, strlen(result), 0, (struct sockaddr *)client2_addr, addr_len);
    }
    else if (is_draw())
    {
        sendto(sockfd, "It's a draw!\n", 13, 0, (struct sockaddr *)client1_addr, addr_len);
        sendto(sockfd, "It's a draw!\n", 13, 0, (struct sockaddr *)client2_addr, addr_len);
    }
}

int ask_replay_non_blocking(int sockfd, struct sockaddr_in *client1_addr, struct sockaddr_in *client2_addr, socklen_t addr_len)
{
    char response1[BUFFER_SIZE], response2[BUFFER_SIZE];
    int replay1 = 0, replay2 = 0, received1 = 0, received2 = 0;
    fd_set read_fds;
    struct timeval timeout;

    // Send replay question to both clients
    sendto(sockfd, "Do you want to play again? (yes/no): ", 38, 0, (struct sockaddr *)client1_addr, addr_len);
    sendto(sockfd, "Do you want to play again? (yes/no): ", 38, 0, (struct sockaddr *)client2_addr, addr_len);

    // Set timeout to wait for the replies
    timeout.tv_sec = 30; // Wait for up to 30 seconds
    timeout.tv_usec = 0;

    while (!received1 || !received2)
    {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            perror("select error");
            break;
        }
        else if (activity == 0)
        {
            printf("Timeout waiting for clients' responses.\n");
            break;
        }

        if (FD_ISSET(sockfd, &read_fds))
        {
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);
            char buffer[BUFFER_SIZE];
            recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &len);

            if (!received1 && memcmp(&client_addr, client1_addr, sizeof(client_addr)) == 0)
            {
                strncpy(response1, buffer, BUFFER_SIZE);
                received1 = 1;
                if (strncmp(response1, "yes", 3) == 0)
                {
                    replay1 = 1;
                }
            }
            else if (!received2 && memcmp(&client_addr, client2_addr, sizeof(client_addr)) == 0)
            {
                strncpy(response2, buffer, BUFFER_SIZE);
                received2 = 1;
                if (strncmp(response2, "yes", 3) == 0)
                {
                    replay2 = 1;
                }
            }
        }
    }

    // Determine the result of the replay requests
    if (replay1 && replay2)
    {
        sendto(sockfd, "Starting a new game...\n", 23, 0, (struct sockaddr *)client1_addr, addr_len);
        sendto(sockfd, "Starting a new game...\n", 23, 0, (struct sockaddr *)client2_addr, addr_len);
        return 1; // Both players want to replay
    }
    else if (!replay1 && !replay2)
    {
        sendto(sockfd, "Both players declined to play again. Exiting...\n", 47, 0, (struct sockaddr *)client1_addr, addr_len);
        sendto(sockfd, "Both players declined to play again. Exiting...\n", 47, 0, (struct sockaddr *)client2_addr, addr_len);
        return 0; // Both players declined
    }
    else if (!replay1)
    {
        sendto(sockfd, "You declined to play again. Exiting...\n", 39, 0, (struct sockaddr *)client1_addr, addr_len);
        sendto(sockfd, "Player 1 declined to play again. Exiting...\n", 43, 0, (struct sockaddr *)client2_addr, addr_len);
        return 0; // Player 1 declined
    }
    else
    {
        sendto(sockfd, "Player 2 declined to play again. Exiting...\n", 43, 0, (struct sockaddr *)client1_addr, addr_len);
        sendto(sockfd, "You declined to play again. Exiting...\n", 39, 0, (struct sockaddr *)client2_addr, addr_len);
        return 0; // Player 2 declined
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client1_addr, client2_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for Player 1...\n");
    recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *)&client1_addr, &addr_len);
    sendto(sockfd, "You are Player 1 (X)\n", 22, 0, (struct sockaddr *)&client1_addr, addr_len);

    printf("Waiting for Player 2...\n");
    recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *)&client2_addr, &addr_len);
    sendto(sockfd, "You are Player 2 (O)\n", 22, 0, (struct sockaddr *)&client2_addr, addr_len);

    reset_board();
    send_board(sockfd, &client1_addr, &client2_addr);

    while (1)
    {
        handle_move(sockfd, current_player, &client1_addr, &client2_addr, addr_len);
        current_player = 3 - current_player; // Switch between player 1 and 2

        if (check_winner() || is_draw())
        {
            if (!ask_replay_non_blocking(sockfd, &client1_addr, &client2_addr, addr_len))
                break; // End the game if no one wants to replay
            reset_board();
            send_board(sockfd, &client1_addr, &client2_addr);
        }
    }

    close(sockfd);
    return 0;
}
