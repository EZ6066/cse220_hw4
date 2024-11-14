#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT1 2201 //Client 1
#define PORT2 2202 //Client 2
#define BUFFER_SIZE 1024

void server_function();
void client_function(int);
int **create_board(int,int);
void clear_board(int**);
void free_board(int**);
int place_ship(int, int, int, int, int**);
int validate_ship(int, int, int, int, int**);

int **board1 = NULL;
int **board2 = NULL;
int boardheight = 0;
int boardwidth = 0;


int** create_board(int width, int height) {
    int **board = (int**)malloc(width * sizeof(int*));
    for (int i = 0; i < width; i++) {
        board[i] = (int*)calloc(height, sizeof(int));  // Allocate and initialize to 0
    }
    return board;
}

void clear_board(int **board) {
    for (int i = 0; i < boardwidth; i++) {
        for (int j = 0; j < boardheight; j++) {
            board[i][j] = 0;  // Set each element to 0
        }
    }
}

void free_board(int **board) {
    for (int i = 0; i < boardheight; i++) {
        free(board[i]);
    }
    free(board);
}

int place_ship(int piece_type, int piece_rotation, int piece_col, int piece_row, int **board){
    switch(piece_type){
        case '1':
            //Check if the ship can be fit in the board
            if (piece_col + 1 > boardwidth || piece_row + 1 > boardheight){
                //Error 302 should be raise
            }
            //Check if an overlap had occured
            else if (board[piece_row][piece_col] || board[piece_row+1][piece_col] || board[piece_row][piece_col+1] || board[piece_row+1][piece_col+1]){
                //Error 303 should be raise
            }
            board[piece_row][piece_col] == 1;
            board[piece_row+1][piece_col] == 1;
            board[piece_row][piece_col+1] == 1; 
            board[piece_row+1][piece_col+1] == 1;

            break;
        case '2':
            

            break;
        case '3':
            
            break;
        case '4':

            break;
        case '5':

            break;
        case '6':

            break;
        case '7':

            break;
    }
    return 1;
}

int validate_ship(int piece_type, int piece_rotation, int piece_col, int piece_row, int **board){
    if (piece_type > 7 || piece_type < 1 || piece_rotation > 4 || piece_rotation < 4 ||
        piece_col > boardheight || piece_col < 0 || piece_row > boardwidth || piece_row < 0){
            return 0;
        }

        
    return 1;

}
// Function to handle the server
void server_function() {
    int listen_fd1, listen_fd2, conn_fd1, conn_fd2;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE] = {0};
    int opt = 1;
    int addrlen = sizeof(address);

    // Create the first listening socket for Client 1
    if ((listen_fd1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed for Client 1.");
        exit(EXIT_FAILURE);
    }

    // Set socket options for Client 1
    if (setsockopt(listen_fd1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed for Client 1.");
        exit(EXIT_FAILURE);
    }

    // Bind socket to PORT1 (for Client 1)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT1);  // Use PORT1 for Client 1
    if (bind(listen_fd1, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed for Client 1.");
        exit(EXIT_FAILURE);
    }

    // Start listening for Client 1
    if (listen(listen_fd1, 1) < 0) {
        perror("Listen failed for Client 1.");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d for Client 1\n", PORT1);

    // Create the second listening socket for Client 2
    if ((listen_fd2 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed for Client 2.");
        exit(EXIT_FAILURE);
    }

    // Set socket options for Client 2
    if (setsockopt(listen_fd2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed for Client 2.");
        exit(EXIT_FAILURE);
    }

    // Bind socket to PORT2 (for Client 2)
    address.sin_port = htons(PORT2);  // Use PORT2 for Client 2
    if (bind(listen_fd2, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed for Client 2.");
        exit(EXIT_FAILURE);
    }

    // Start listening for Client 2
    if (listen(listen_fd2, 1) < 0) {
        perror("Listen failed for Client 2.");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d for Client 2\n", PORT2);

    // Accept connection for Client 1 (on port1)
    if ((conn_fd1 = accept(listen_fd1, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Accept failed for Client 1.");
        exit(EXIT_FAILURE);
    }
    printf("Client 1 connected.\n");

    //Accept connection for Client 2 (on port2)
    if ((conn_fd2 = accept(listen_fd2, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Accept failed for Client 2.");
        exit(EXIT_FAILURE);
    }
    printf("Client 2 connected.\n");
    //Handle communication between two clients
    while (1) {
        // Receive message from Client 1 
        memset(buffer, 0, BUFFER_SIZE);
        read(conn_fd1, buffer, BUFFER_SIZE);
            
            switch(buffer[0]){
                
                case 'B':
                    char message[BUFFER_SIZE];
                    int width,height;
                    char command;
                    sscanf(buffer, "%c %d %d", &command, &width, &height);
                    if (width < 10 || height < 10){
                        sprintf(message,"Invalid");fflush(stdout);
                        send(conn_fd1, message, strlen(message), 0);  // Send message to Client 1
                    }
                    else{
                        board1 = create_board(width,height);
                        board2 = create_board(width,height);
                        boardwidth = width;
                        boardheight = height;
                        sprintf(message, "Board of %dx%d created.", width, height);fflush(stdout);
                        send(conn_fd1, message, strlen(message), 0);  // Send message to Client 1
                    }
                case 'I':
                    int piece_type, piece_rotation, piece_col, piece_row;
                    int ship_count = 0;
                    
                    while(sscanf(buffer, "%d %d %d %d", &piece_type, &piece_rotation, &piece_col, &piece_row) == 4 &&){
                        ship_count++;
                    }
                    
                    
                    break;

                default:

            }
        

        // Receive message from Client 2 and send to Client 1
        memset(buffer, 0, BUFFER_SIZE);
        read(conn_fd2, buffer, BUFFER_SIZE);
         switch(buffer[0]){
                
                case 'B':

                    break;

                default:
         }

    }

    // Close connections
    close(conn_fd1);
    close(conn_fd2);
    close(listen_fd1);
    close(listen_fd2);
    //Free the boards
    free_board(board1);
    free_board(board2);
    printf("Server shutting down.\n");
}

// Function to handle the client
void client_function(int clientID) {
    int client_fd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed.");
        exit(EXIT_FAILURE);
    }
    // Define server address
    int PORT = PORT1;
    if (clientID == 2){
        PORT = PORT2;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address.");
        exit(EXIT_FAILURE);
    }
    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed.");
        exit(EXIT_FAILURE);
    }

    // Start sending and receiving messages
    if (clientID == 1){
        int width,height;
        char command;
            // Parse the input using sscanf
        do{
            printf("Set up the board (B <Width_of_board Height_of_board>):");fflush(stdout);
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strlen(buffer)-1] = '\0';

            if (sscanf(buffer, "%c %d %d", &command, &width, &height) == 3){
                snprintf(buffer, sizeof(buffer), "%c %d %d", command, width, height);fflush(stdout);
                send(client_fd, buffer, strlen(buffer), 0);  // Send the formatted message to the server
            }

            memset(buffer, 0, BUFFER_SIZE);
            read(client_fd, buffer, BUFFER_SIZE);
            if (strcmp(buffer, "Invalid") == 0){
                printf("Invalid width/height. ");fflush(stdout);
            }
        } while (strcmp(buffer, "Invalid") == 0);

        printf("%s", buffer);fflush(stdout);
            
    }
    else{

    }
   
        // Close connection
        close(client_fd);
        printf("Client shutting down.\n");
}

    

// Main function to decide whether to run the server or the client
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <server/client1/client2>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        server_function();
    } else if (strcmp(argv[1], "client1") == 0) {
        client_function(1);
    } else if (strcmp(argv[1], "client2") == 0){
        client_function(2);
    } else {
        printf("Invalid argument. Use 'server' or 'client1' or 'client2'.\n");
        return 1;
    }
    fflush(stdout);
    return 0;
}
