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
#define PACKET_HALT 'H'

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
    int game_over;
} GameState;

GameState game_state;
Client clients[2];
TetrisShape shapes[7];

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
    game_state.game_over = 0;

    game_state.board = malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++) {
        game_state.board[i] = calloc(width, sizeof(int));
    }
}

void initialize_ships(Client *client, const char *ship_data) {
    int shape, rotation, x, y;
    const char *ptr = ship_data;
    int ships_placed = 0;

    // Debugging: Print the ship data
    printf("Initializing ships for Player %d with data: %s\n", client->player_num, ship_data);

    // Skip the 'I' part and process each ship's data (in groups of 4 numbers per ship)
    ptr = strchr(ptr, ' ');  // Skip the 'I'
    if (ptr) {
        ptr++; // Move past the space to the next data part
    }

    // Loop through the ship data and process each ship (4 values per ship)
    while (ships_placed < MAX_SHIPS && ptr != NULL) {
        // Parse each ship's data (e.g., "1 1 0 0")
        if (sscanf(ptr, "%d %d %d %d", &shape, &rotation, &x, &y) == 4) {
            // Debugging: Log the parsed values
            printf("Parsed ship data: Type %d, Rotation %d, Position (%d, %d)\n", shape, rotation, x, y);

            // Validate and place the ship
            if (validate_ship_placement(x, y, shape, rotation, client->player_num)) {
                place_ship(x, y, shape, rotation, client->player_num);
                ships_placed++;
                printf("Ship placed: Type %d, Rotation %d, Position (%d, %d)\n", shape, rotation, x, y);
            } else {
                printf("Invalid ship placement for Type %d, Rotation %d at Position (%d, %d)\n", shape, rotation, x, y);
                send_error(client->socket_fd, ERR_INVALID_PARAMS_INIT);
                return;
            }
        } else {
            // If parsing failed, send an error and stop ship initialization
            printf("Invalid ship data format: %s\n", ptr);
            send_error(client->socket_fd, ERR_INVALID_PARAMS_INIT);
            return;
        }

        // Move to the next ship (skip past this ship's data)
        ptr = strchr(ptr, ' ');
        if (ptr) {
            ptr++; // Move past the space to the next ship data
        }
    }

    // If all ships were placed correctly, acknowledge the player
    if (ships_placed == MAX_SHIPS) {
        send_acknowledgement(client->socket_fd);
    } else {
        send_error(client->socket_fd, ERR_INVALID_PARAMS_INIT);
    }
}

// Validate the ship placement for the given piece
int validate_ship_placement(int x, int y, int shape, int rotation, int player) {
    for (int i = 0; i < 4; i++) {
        int dx = shapes[shape].rotations[rotation][i][0];
        int dy = shapes[shape].rotations[rotation][i][1];
        int nx = x + dx;
        int ny = y + dy;

        // Check if the ship is within bounds and doesn't overlap
        if (nx < 0 || ny < 0 || nx >= game_state.width || ny >= game_state.height || game_state.board[ny][nx] != 0) {
            return 0; // Invalid placement
        }
    }
    return 1; // Valid placement
}

// Place the ship on the board for the given player
void place_ship(int x, int y, int shape, int rotation, int player) {
    for (int i = 0; i < 4; i++) {
        int dx = shapes[shape].rotations[rotation][i][0];
        int dy = shapes[shape].rotations[rotation][i][1];
        // Mark the board with the player's number to indicate ship placement
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

    // Step 1: Create the client socket
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

    // Step 2: Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    printf("Connected to the server as Player %d\n", player_num);

    // Player 1 initializes the board
    if (player_num == 1) {
        printf("Enter command to set up board (format: B <Width_of_board Height_of_board>): ");
        fgets(buffer, BUFFER_SIZE, stdin);  // Read board dimensions

        // Remove newline character from the input
        buffer[strcspn(buffer, "\n")] = 0;

        // Send the board setup command to the server
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("Failed to send board setup command");
            close(sock);
            return;
        }
        printf("Sent board setup command: %s\n", buffer);

        // Wait for acknowledgment from the server
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0 && buffer[0] == 'A') {
            printf("Server acknowledged board setup.\n");
        } else {
            printf("Failed to receive acknowledgment for board setup. Exiting.\n");
            close(sock);
            return;
        }
    }

    // For Player 2, prompt them to send 'B' to start the game
    if (player_num == 2) {
        // Server should print a message asking Player 2 to send 'B' to start the game
        printf("Please send 'B' to start the game, Player 2...\n");

        // Wait for Player 2 to send 'B' to start the game
        fgets(buffer, BUFFER_SIZE, stdin);  // Wait for input from Player 2
        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline

        // Send the "B" to the server
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("Failed to send 'B' packet");
            close(sock);
            return;
        }
        printf("Sent 'B' to the server.\n");

        // Wait for server's response (acknowledgment)
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0 && buffer[0] == 'A') {
            printf("Server acknowledged the game start.\n");
        } else {
            printf("Error: Failed to receive acknowledgment from server.\n");
            close(sock);
            return;
        }
    }

    // Step 3: Start game handling loop for both players
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);

        // Display the server's response
        if (valread > 0) {
            printf("Server: %s\n", buffer);
            if (buffer[0] == 'H') {
                printf("Game over. Result: %s\n", buffer);
                break;
            }
        }

        // Prompt for commands (I, S, Q, F)
        printf("Enter command (I to initialize ships, S to shoot, Q for query, F to forfeit): ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Remove newline character from the input
        buffer[strcspn(buffer, "\n")] = 0;

        // Send the command to the server
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("Failed to send command");
            break;
        }

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

        printf("Received from Player %d: %s\n", client->player_num, buffer);

        char packet_type = buffer[0];
        switch (packet_type) {
            case 'B':  // Handle the Begin (board setup) command
                if (client->player_num == 1) {
                    int width, height;
                    // Parse the board dimensions from the command
                    if (sscanf(buffer + 2, "%d %d", &width, &height) == 2 &&
                        width >= MIN_BOARD_SIZE && height >= MIN_BOARD_SIZE) {
                        
                        setup_board(width, height);  // Set up the board on the server
                        send_acknowledgement(client->socket_fd);  // Send acknowledgment to Player 1
                        printf("Board initialized to %dx%d by Player 1.\n", width, height);
                    } else {
                        send_error(client->socket_fd, ERR_INVALID_PARAMS_BEGIN);
                        printf("Invalid board dimensions from Player 1. Minimum size is %dx%d.\n", MIN_BOARD_SIZE, MIN_BOARD_SIZE);
                    }
                } else {
                    send_acknowledgement(client->socket_fd);
                    printf("Player 2 joined the game.\n");
                }
                break;

            // Other cases like initialize, shoot, query, and forfeit go here
            default:
                send_error(client->socket_fd, ERR_INVALID_PACKET_TYPE);
                break;
        }
    }

    close(client->socket_fd);
}

void start_server() {
    int server_fd1, server_fd2;
    struct sockaddr_in address1, address2;
    int opt = 1;
    socklen_t addrlen1 = sizeof(address1);
    socklen_t addrlen2 = sizeof(address2);

    // Setup socket for Player 1
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

    // Accept Player 1 connection
    clients[0].socket_fd = accept(server_fd1, (struct sockaddr *)&clients[0].address, &addrlen1);
    if (clients[0].socket_fd >= 0) {
        clients[0].player_num = 1;
        printf("Player 1 connected. Waiting for board dimensions.\n");
    } else {
        perror("Failed to accept Player 1 connection");
        close(server_fd1);
        return;
    }

    // Wait for Player 1's board setup command
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int valread = read(clients[0].socket_fd, buffer, BUFFER_SIZE);
    if (valread > 0 && buffer[0] == 'B') {
        int width, height;
        if (sscanf(buffer + 2, "%d %d", &width, &height) == 2 && width >= MIN_BOARD_SIZE && height >= MIN_BOARD_SIZE) {
            setup_board(width, height);  // Initialize the board
            send_acknowledgement(clients[0].socket_fd);  // Acknowledge Player 1
            printf("Board initialized to %dx%d by Player 1.\n", width, height);
        } else {
            send_error(clients[0].socket_fd, ERR_INVALID_PARAMS_BEGIN);
            printf("Invalid board dimensions from Player 1. Minimum size is %dx%d.\n", MIN_BOARD_SIZE, MIN_BOARD_SIZE);
            return;
        }
    }

    // Step 2: Now wait for Player 2’s "B" packet to indicate they are ready to start
    printf("Waiting for Player 2 to send 'B' to start the game...\n");

    // Set up socket for Player 2 (PORT 2202)
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

    // Accept Player 2's connection
    clients[1].socket_fd = accept(server_fd2, (struct sockaddr *)&clients[1].address, &addrlen2);
    if (clients[1].socket_fd >= 0) {
        clients[1].player_num = 2;
        printf("Player 2 connected.\n");
    } else {
        perror("Failed to accept Player 2 connection");
        close(server_fd2);
        return;
    }

    // Step 3: Manually prompt Player 2 to send "B"
    printf("Please send 'B' to start the game, Player 2...\n");

    // Wait for Player 2 to send the 'B' packet
    memset(buffer, 0, BUFFER_SIZE);
    valread = read(clients[1].socket_fd, buffer, BUFFER_SIZE);
    if (valread > 0 && buffer[0] == 'B') {
        send_acknowledgement(clients[1].socket_fd);  // Acknowledge Player 2
        printf("Player 2 acknowledged the game start.\n");
    } else {
        send_error(clients[1].socket_fd, ERR_INVALID_PACKET_TYPE);
        return;
    }

    // **New Step**: After Player 2 acknowledges, prompt both players to initialize their ships
    send_response(clients[0].socket_fd, "Enter ship initialization (I <Piece_type Piece_rotation Piece_column Piece_row>): ");
    send_response(clients[1].socket_fd, "Enter ship initialization (I <Piece_type Piece_rotation Piece_column Piece_row>): ");

    // Step 4: Handle ship initialization for Player 1
    memset(buffer, 0, BUFFER_SIZE);
    valread = read(clients[0].socket_fd, buffer, BUFFER_SIZE);
    if (valread > 0) {
        printf("Received ship setup from Player 1: %s\n", buffer);
        initialize_ships(&clients[0], buffer); // Validate and place ships for Player 1
        send_acknowledgement(clients[0].socket_fd);
    }

    // Step 5: Handle ship initialization for Player 2
    memset(buffer, 0, BUFFER_SIZE);
    valread = read(clients[1].socket_fd, buffer, BUFFER_SIZE);
    if (valread > 0) {
        printf("Received ship setup from Player 2: %s\n", buffer);
        initialize_ships(&clients[1], buffer); // Validate and place ships for Player 2
        send_acknowledgement(clients[1].socket_fd);
    }

    // Step 6: After both players have initialized ships, the game can proceed
    printf("Both players have initialized ships. Starting the game...\n");
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
