#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT1 2201
#define PORT2 2202
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define MAX_SHIPS 5
#define MIN_BOARD_SIZE 10

// Packet Identifiers
#define PACKET_BEGIN 'B'
#define PACKET_INITIALIZE 'I'
#define PACKET_SHOOT 'S'
#define PACKET_QUERY 'Q'
#define PACKET_FORFEIT 'F'

// Error Codes
#define ERR_INVALID_PACKET_TYPE 100
#define ERR_INVALID_PARAMS_BEGIN 200
#define ERR_INVALID_PARAMS_INIT 201
#define ERR_INVALID_SHOOT 202

typedef struct {
    int rotations[4][4][2];
} TetrisShape;

typedef struct {
    int socket_fd;
    int player_num;
    struct sockaddr_in address;
} Client;

typedef struct {
    int width, height;
    int **board;
    int ships_remaining[2];
    int turn;
} GameState;

// Function Prototypes
void initialize_tetris_shapes();
void setup_board(int width, int height);
int validate_ship_placement(int x, int y, int shape, int rotation, int player);
void place_ship(int x, int y, int shape, int rotation, int player);
void send_acknowledgement(int client_fd);
void send_error(int client_fd, int error_code);
void send_response(int client_fd, const char *response);
void start_client(int player_num);
void *handle_client(void *arg);
void initialize_ships(Client *client, const char *ship_data);
void start_server();

GameState game_state;
Client clients[2];
TetrisShape shapes[7];

// Initialize Tetris shapes with rotations
void initialize_tetris_shapes() {
    shapes[0].rotations[0][0][0] = 0; shapes[0].rotations[0][0][1] = 0;
    shapes[0].rotations[0][1][0] = 1; shapes[0].rotations[0][1][1] = 0;
    shapes[0].rotations[0][2][0] = 2; shapes[0].rotations[0][2][1] = 0;
    shapes[0].rotations[0][3][0] = 2; shapes[0].rotations[0][3][1] = 1;
    // Define the rest of the Tetris shapes and their rotations here.
}

// Setup game board with given dimensions
void setup_board(int width, int height) {
    game_state.width = width;
    game_state.height = height;
    game_state.turn = 1;
    game_state.ships_remaining[0] = MAX_SHIPS;
    game_state.ships_remaining[1] = MAX_SHIPS;

    game_state.board = malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++) {
        game_state.board[i] = calloc(width, sizeof(int));
    }
}

// Validate Tetris-shaped ship placement
int validate_ship_placement(int x, int y, int shape, int rotation, int player) {
    for (int i = 0; i < 4; i++) {
        int dx = shapes[shape].rotations[rotation][i][0];
        int dy = shapes[shape].rotations[rotation][i][1];
        int nx = x + dx;
        int ny = y + dy;
        if (nx < 0 || ny < 0 || nx >= game_state.width || ny >= game_state.height || game_state.board[ny][nx] != 0) {
            return 0;
        }
    }
    return 1;
}

// Place a ship on the game board
void place_ship(int x, int y, int shape, int rotation, int player) {
    for (int i = 0; i < 4; i++) {
        int dx = shapes[shape].rotations[rotation][i][0];
        int dy = shapes[shape].rotations[rotation][i][1];
        game_state.board[y + dy][x + dx] = player;
    }
}

// Send responses to clients
void send_acknowledgement(int client_fd) {
    send(client_fd, "A\n", 2, 0);
}

void send_error(int client_fd, int error_code) {
    char response[16];
    snprintf(response, sizeof(response), "E %d\n", error_code);
    send(client_fd, response, strlen(response), 0);
}

void send_response(int client_fd, const char *response) {
    send(client_fd, response, strlen(response), 0);
}

// Client start function
void start_client(int player_num) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int port = (player_num == 1) ? PORT1 : PORT2;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    printf("Connected to the server as Player %d\n", player_num);

    if (player_num == 1) {
        int width = 10, height = 10;
        snprintf(buffer, sizeof(buffer), "B %d %d\n", width, height);
        send(sock, buffer, strlen(buffer), 0);
        printf("Sent Begin packet with board size %dx%d.\n", width, height);
    } else {
        snprintf(buffer, sizeof(buffer), "B\n");
        send(sock, buffer, strlen(buffer), 0);
        printf("Sent Begin packet to join the game.\n");
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);

        if (valread > 0) {
            printf("Server: %s\n", buffer);
            if (buffer[0] == 'H') {
                printf("Game over. Result: %s\n", buffer);
                break;
            }
        }

        printf("Enter command (I to initialize, S to shoot, Q for query, F to forfeit): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "exit") == 0) break;

        if (buffer[0] == 'I') {
            snprintf(buffer, sizeof(buffer), "I 1 0 1 1 2 0 3 1 4 1\n");
            printf("Sent Initialize packet: %s\n", buffer);
        } else if (buffer[0] == 'S') {
            snprintf(buffer, sizeof(buffer), "S 2 3\n");
            printf("Sent Shoot packet: %s\n", buffer);
        } else if (buffer[0] == 'Q') {
            snprintf(buffer, sizeof(buffer), "Q\n");
            printf("Sent Query packet.\n");
        } else if (buffer[0] == 'F') {
            snprintf(buffer, sizeof(buffer), "F\n");
            printf("Sent Forfeit packet.\n");
        }

        send(sock, buffer, strlen(buffer), 0);

        if (buffer[0] == 'F') {
            printf("Forfeited the game.\n");
            break;
        }
    }
    close(sock);
}

// Handle incoming packets and gameplay logic
void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client->socket_fd, buffer, BUFFER_SIZE);

        if (valread <= 0) {
            printf("Player %d disconnected.\n", client->player_num);
            break;
        }

        char packet_type = buffer[0];
        switch (packet_type) {
            case PACKET_BEGIN:
                if (client->player_num == 1) {
                    int width, height;
                    if (sscanf(buffer + 2, "%d %d", &width, &height) == 2 && width >= MIN_BOARD_SIZE && height >= MIN_BOARD_SIZE) {
                        setup_board(width, height);
                        send_acknowledgement(client->socket_fd);
                        printf("Board initialized to %dx%d by Player 1.\n", width, height);
                    } else {
                        send_error(client->socket_fd, ERR_INVALID_PARAMS_BEGIN);
                    }
                } else {
                    send_acknowledgement(client->socket_fd);
                    printf("Player 2 joined the game.\n");
                }
                break;
            case PACKET_INITIALIZE:
                initialize_ships(client, buffer + 2);
                send_acknowledgement(client->socket_fd);
                break;
            case PACKET_SHOOT:
                // Implement shooting logic here
                break;
            case PACKET_QUERY:
                // Implement query handling here
                break;
            case PACKET_FORFEIT:
                send_response(client->socket_fd, "H 0\n");
                break;
            default:
                send_error(client->socket_fd, ERR_INVALID_PACKET_TYPE);
                break;
        }
    }

    close(client->socket_fd);
}

// Initialize Tetris-shaped ships
void initialize_ships(Client *client, const char *ship_data) {
    int shape, rotation, x, y;
    const char *ptr = ship_data;

    for (int i = 0; i < MAX_SHIPS; i++) {
        if (sscanf(ptr, "%d %d %d %d", &shape, &rotation, &x, &y) == 4) {
            if (validate_ship_placement(x, y, shape, rotation, client->player_num)) {
                place_ship(x, y, shape, rotation, client->player_num);
            } else {
                send_error(client->socket_fd, ERR_INVALID_PARAMS_INIT);
                return;
            }
        }
        ptr = strchr(ptr, '\n') + 1;
    }
}

// Server start function
void start_server() {
    int server_fd1, server_fd2;
    struct sockaddr_in address1, address2;
    int opt = 1;

    if ((server_fd1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed for Player 1");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address1.sin_family = AF_INET;
    address1.sin_addr.s_addr = INADDR_ANY;
    address1.sin_port = htons(PORT1);

    if (bind(server_fd1, (struct sockaddr *)&address1, sizeof(address1)) < 0) {
        perror("Bind on PORT1 failed");
        close(server_fd1);
        exit(EXIT_FAILURE);
    }
    listen(server_fd1, 3);

    printf("Waiting for Player 1 to connect and set board dimensions...\n");

    clients[0].socket_fd = accept(server_fd1, (struct sockaddr *)&clients[0].address, (socklen_t *)&address1);
    if (clients[0].socket_fd >= 0) {
        clients[0].player_num = 1;
        printf("Player 1 connected. Waiting for board dimensions.\n");
    } else {
        perror("Failed to accept Player 1 connection");
        return;
    }

    pthread_t client_thread1;
    pthread_create(&client_thread1, NULL, handle_client, (void *)&clients[0]);
    pthread_join(client_thread1, NULL);  // Ensure board is initialized

    if ((server_fd2 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed for Player 2");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address2.sin_family = AF_INET;
    address2.sin_addr.s_addr = INADDR_ANY;
    address2.sin_port = htons(PORT2);

    if (bind(server_fd2, (struct sockaddr *)&address2, sizeof(address2)) < 0) {
        perror("Bind on PORT2 failed");
        close(server_fd2);
        exit(EXIT_FAILURE);
    }
    listen(server_fd2, 3);

    clients[1].socket_fd = accept(server_fd2, (struct sockaddr *)&clients[1].address, (socklen_t *)&address2);
    if (clients[1].socket_fd >= 0) {
        clients[1].player_num = 2;
        printf("Player 2 connected. Game ready to start.\n");
    } else {
        perror("Failed to accept Player 2 connection");
    }

    pthread_t client_thread2;
    pthread_create(&client_thread2, NULL, handle_client, (void *)&clients[1]);
    pthread_join(client_thread2, NULL);

    close(server_fd1);
    close(server_fd2);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <server|player1|player2>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        initialize_tetris_shapes();
        start_server();
    } else if (strcmp(argv[1], "player1") == 0) {
        start_client(1);
    } else if (strcmp(argv[1], "player2") == 0) {
        start_client(2);
    } else {
        printf("Invalid argument. Use 'server', 'player1', or 'player2'.\n");
    }

    return 0;
}
