#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define PORT 2000
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

void send_board_to_clients(int client1, int client2)
{
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message),
             "\n %c | %c | %c\n---|---|---\n %c | %c | %c\n---|---|---\n %c | %c | %c\n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    send(client1, message, strlen(message), 0);
    send(client2, message, strlen(message), 0);
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

void move_handling(int player, int client1, int client2)
{
    int row, col;
    char move[BUFFER_SIZE];
    int current_client = player == 1 ? client1 : client2;

    while (1)
    {
        // Clear the move buffer to avoid any old data
        memset(move, 0, sizeof(move));

        // Flush any residual data from the input buffer
        char flush_buffer[BUFFER_SIZE];
        while (recv(current_client, flush_buffer, BUFFER_SIZE, MSG_DONTWAIT) > 0)
            ;

        send(current_client, "Your move (row col): ", 21, 0);

        // Receive the actual move
        recv(current_client, move, BUFFER_SIZE, 0);

        if (strlen(move) == 3)
        {
            sscanf(move, "%d %d", &row, &col);

            if (row >= 1 && row <= 3 && col >= 1 && col <= 3 && board[row - 1][col - 1] == ' ')
            {
                board[row - 1][col - 1] = player == 1 ? 'X' : 'O';
                break;
            }
            else
            {
                send(current_client, "Invalid move, try again.\n", 25, 0);
            }
        }else{
            send(current_client, "Invalid move, try again.\n", 25, 0);
        }
    }

    send_board_to_clients(client1, client2);

    if (check_winner() == player)
    {
        char result[BUFFER_SIZE];
        snprintf(result, sizeof(result), "Player %d wins!\n", player);
        send(client1, result, strlen(result), 0);
        send(client2, result, strlen(result), 0);
    }
    else if (is_draw())
    {
        send(client1, "It's a draw!\n", 13, 0);
        send(client2, "It's a draw!\n", 13, 0);
    }
}

int ask_replay_both(int client1, int client2)
{
    char response1[BUFFER_SIZE] = {0};
    char response2[BUFFER_SIZE] = {0};
    fd_set readfds;
    int max_fd = client1 > client2 ? client1 : client2;
    struct timeval timeout;

    // Send replay prompt to both clients
    send(client1, "Do you want to play again? (yes/no): ", 38, 0);
    send(client2, "Do you want to play again? (yes/no): ", 38, 0);

    // Clear buffers before receiving new input
    // memset(response1, 0, sizeof(response1));
    // memset(response2, 0, sizeof(response2));

    // Wait for responses from both clients
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(client1, &readfds);
        FD_SET(client2, &readfds);

        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            perror("select error");
            return 0; // On error, assume no replay
        }

        if (FD_ISSET(client1, &readfds))
        {
            recv(client1, response1, BUFFER_SIZE, 0);
        }

        if (FD_ISSET(client2, &readfds))
        {
            recv(client2, response2, BUFFER_SIZE, 0);
        }

        if (strlen(response1) > 0 && strlen(response2) > 0)
        {
            break;
        }
    }

    // Flush any remaining data from input buffers to avoid residual inputs
    char flush_buffer[BUFFER_SIZE];
    while (recv(client1, flush_buffer, BUFFER_SIZE, MSG_DONTWAIT) > 0)
        ;
    while (recv(client2, flush_buffer, BUFFER_SIZE, MSG_DONTWAIT) > 0)
        ;

    // Process replay responses
    int replay1 = (strncmp(response1, "yes", 3) == 0);
    int replay2 = (strncmp(response2, "yes", 3) == 0);

    if (replay1 && replay2)
    {
        return 1; // Both players want to play again
    }
    else if (!replay1 && replay2)
    {
        send(client2, "Player 1 doesn't want to play again. Exiting...\n", 47, 0);
    }
    else if (replay1 && !replay2)
    {
        send(client1, "Player 2 doesn't want to play again. Exiting...\n", 47, 0);
    }

    return 0; // At least one player doesn't want to continue
}

void game_session(int client1 ,int client2)
{
    // int *client_sockets = (int *)clients;
    // int client1 = client_sockets[0];
    // int client2 = client_sockets[1];

    int play = 1;

    while (play)
    {
        reset_board();
        send_board_to_clients(client1, client2);

        while (1)
        {
            move_handling(1, client1, client2);
            if (check_winner() || is_draw())
                break;
            move_handling(2, client1, client2);
            if (check_winner() || is_draw())
                break;
        }

        // Ask both players if they want to play again
        if (ask_replay_both(client1, client2))
        {
            send(client1, "Starting a new game...\n", 23, 0);
            send(client2, "Starting a new game...\n", 23, 0);
            play = 1;
        }
        else
        {
            send(client1, "Ending the game.... Thank you for playing!\n", 39, 0);
            send(client2, "Ending the game.... Thank you for playing!\n", 39, 0);
            play = 0;
        }
    }

    // free(client_sockets);
    // close(client1);
    // close(client2);
    // pthread_exit(NULL);
}

struct sockaddr_in *sockaddrMake(){

    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

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
    int *server_fd, client1_fd, client2_fd;

    server_fd = socketMake();

    struct sockaddr_in *address = sockaddrMake();
    socklen_t addr_len = sizeof(*address);

    if (bind(*server_fd, (struct sockaddr *)address, addr_len) < 0)
    {
        perror("Bind failed");
        close(*server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(*server_fd, 2) < 0)
    {
        perror("Listen failed");
        close(*server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    while (1)
    {
        client1_fd= accept(*server_fd, (struct sockaddr *)address, &addr_len);
        client2_fd = accept(*server_fd, (struct sockaddr *)address, &addr_len);

        if (client1_fd < 0 || client2_fd < 0)
        {
            perror("Connection failed");
            continue;
        }

        printf("Players connected. Starting game...\n");

        // int *clients = malloc(2 * sizeof(int));
        // clients[0] = client1_fd;
        // clients[1] = client2_fd;

        // pthread_t thread;
        // pthread_create(&thread, NULL, game_session, (void *)clients);
        // pthread_detach(thread);
        game_session(client1_fd,client2_fd);
    }

    close(*server_fd);
    close(client1_fd);
    close(client2_fd);

    free(address);
    return 0;
}
