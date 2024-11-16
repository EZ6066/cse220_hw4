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
int count_ship (int**);
int validate_input(const char*);
void query(char[], int**);
int place_ship(int, int, int, int, int**, int);
int validate_ship(int, int, int, int, int**, int);

int **board1 = NULL;
int **board2 = NULL;
int boardheight = 0;
int boardwidth = 0;
int initialize1 = 0;
int initialize2 = 0;


int** create_board(int width, int height) {
    int **board = (int**)malloc(height* sizeof(int*));
    for (int i = 0; i < height; i++) {
        board[i] = (int*)malloc(width* sizeof(int));  // Allocate and initialize to 0
    }
    return board;
}

void printBoard(int **board) {
    for (int i = 0; i < boardheight; i++) {
        for (int j = 0; j < boardwidth; j++) {
            printf("%d ", board[i][j]);
        }
        printf("\n");
    }
}

void clear_board(int **board) {
    for (int i = 0; i < boardheight; i++) {
        for (int j = 0; j < boardwidth; j++) {
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

int count_ship(int **board) {
    int ship_id[5] = {0};  // Array to store up to 5 unique numbers
    int ship_num = 0;
    printBoard(board);

    for (int i = 0; i < boardheight; i++) {
        for (int j = 0; j < boardwidth; j++) {
            int num = board[i][j];

            // Ignore -2 and -1 and 0
            if (num == -2 || num == -1 || num == 0) {
                continue;
            }

            // Check if num is already in ship_id
            int is_unique = 1;  // Assume it's unique (1 = true)
            for (int k = 0; k < ship_num; k++) {
                if (ship_id[k] == num) {
                    is_unique = 0;  // Not unique (0 = false)
                    break;
                }
            }

            // If it's a new unique number, add it to ship_id
            if (is_unique && ship_num < 5) {
                ship_id[ship_num] = num;
                ship_num++;
            }
        }
    }

    return ship_num;
}

int validate_input(const char *buffer) {
    int n[20] = {0};
    char command;
    char st;
    // Check if we have exactly 20 numbers
    return sscanf(buffer, "%c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %c",&command, &n[0], &n[1], &n[2], &n[3], &n[4], &n[5], 
                  &n[6], &n[7], &n[8], &n[9], &n[10], &n[11], &n[12], &n[13], &n[14], &n[15], &n[16], &n[17], &n[18], &n[19], &st) == 21;
}

void query(char *result, int **board) {
    printf("Result from query:%s\n", result);
    printBoard(board);
    for (int row = 0; row < boardheight; row++) {
        for (int col = 0; col < boardwidth; col++) {
            
            if (board[row][col] == -1) {
                // Append "M row# col#"
                printf("-1 is working\n");
                sprintf(result + strlen(result), " M %d %d", row, col);
            } else if (board[row][col] == -2) {
                // Append "H row# col#"
                printf("-2 is working\n");
                sprintf(result + strlen(result), " H %d %d", row, col);
            }
        }
    }
    printf("Result from query:%s\n", result);
}


int place_ship(int piece_type, int piece_rotation, int piece_col, int piece_row, int **board, int ship_num){
    printf("Placeshipid: %d\n:", ship_num);
        
    switch(piece_type){
        case 1:

            //Check if the ship can be fit in the board
            if (piece_col + 1 >= boardwidth || piece_row + 1 >= boardheight){
                //Error 302 should be raise
                return 302;
            }
            //Check if an overlap had occurred
            if (board[piece_row][piece_col] || board[piece_row+1][piece_col] || board[piece_row][piece_col+1] || board[piece_row+1][piece_col+1]){
                //Error 303 should be raise
                return 303;
            }
            //Since all roations have the same shape, there will be only one way to place this shape
            board[piece_row][piece_col] = ship_num;
            board[piece_row+1][piece_col] = ship_num;
            board[piece_row][piece_col+1] = ship_num; 
            board[piece_row+1][piece_col+1] = ship_num;

            break;
        case 2:
            //Rotation 1 and 3
            if (piece_rotation % 2){
                //Check if the ship can be fit in the board
                if(piece_row + 3 >= boardheight){
                //Error 302 should be raise
                return 302;
                }

                for (int i = 0; i < 4; i++){
                    //Check if an overlap had occurred in the rows
                    if (board[piece_row+i][piece_col] == 1){
                        //Error 303 should be raise
                        return 303;
                    }
                }

                for (int i = 0; i < 4; i++){
                    //Place the ship into the board
                    board[piece_row+i][piece_col] = 1;
                }
                
            }
            //Rotation 2 and 4
            else {
                //Check if the ship can be fit in the board
                if(piece_col + 3 >= boardwidth){
                //Error 302 should be raise
                return 302;
                }
                for (int i = 0; i < 4; i++){
                    //Check if an overlap had occurred in the cols
                    if (board[piece_row][piece_col+i] == 1){
                        //Error 303 should be raise
                        return 303;
                    }
                }

                for (int i = 0; i < 4; i++){
                    //Place the ship into the board
                    board[piece_row][piece_col+i] = 1;
                }
            }

            break;
        case 3:
            //Rotation 1 and 3
            if(piece_rotation % 2){
                //Check if the ship can fit in the board
                if (piece_row - 1 < 0 || piece_col + 2 >= boardwidth){
                    //Error 302 should be raise
                    return 302;
                }
                
                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] || 
                    board[piece_row-1][piece_col+1] || board[piece_row-1][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row-1][piece_col+1] = ship_num;
                board[piece_row-1][piece_col+1] = ship_num;
            } 
            //Rotation 2 and 4
            else {
                //Check if the ship can fit in the board
                if(piece_row + 2 >= boardheight || piece_col + 1 >= boardwidth){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row+1][piece_col] ||
                    board[piece_row+1][piece_col+1] || board[piece_row+2][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }
                board[piece_row][piece_col] = ship_num;
                board[piece_row+1][piece_col] = ship_num;
                board[piece_row+1][piece_col+1] = ship_num;
                board[piece_row+2][piece_col+1] = ship_num;
            }
            break;
        case 4:
            //Rotation 1
            if (piece_rotation == 1){
                //Check if the ship can fit in the board
                if (piece_col + 1 >= boardwidth || piece_row + 2 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row+1][piece_col] ||
                    board[piece_row+2][piece_col] || board[piece_row+2][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row+1][piece_col] = ship_num;
                board[piece_row+2][piece_col] = ship_num;
                board[piece_row+2][piece_col+1] = ship_num;

            } 
            //Rotation 2
            else if (piece_rotation == 2){
                //Check if the ship can fit in the board
                if (piece_col + 2 >= boardwidth || piece_row + 1 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row][piece_col+2] || board[piece_row+1][piece_col]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row][piece_col+2] = ship_num;
                board[piece_row+1][piece_col] = ship_num;

            } 
            //Rotation 3
            else if (piece_rotation == 3){
                //Check if the ship can fit in the board
                if (piece_col + 1 >= boardwidth || piece_row + 2 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row+1][piece_col+1] || board[piece_row+2][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row+1][piece_col+1] = ship_num;
                board[piece_row+2][piece_col+1] = ship_num;

            } 
            //Rotation 4
            else{
                //Check if the ship can fit in the board
                if (piece_col + 2 >= boardwidth || piece_row - 1 < 0){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row][piece_col+2] || board[piece_row-1][piece_col+2]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row][piece_col+2] = ship_num;
                board[piece_row-1][piece_col+2] = ship_num;
            }

            break;
        case 5:
            //Rotation 1 and 3
            if (piece_rotation % 2){
                //Check if the ship can fit in the board
                if (piece_col + 2 >= boardwidth || piece_row + 1 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row+1][piece_col+1] || board[piece_row+1][piece_col+2]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row+1][piece_col+1] = ship_num;
                board[piece_row+1][piece_col+2] = ship_num;
            }
            //Rotation 2 and 4
            else{
                //Check if the ship can fit in the board
                if (piece_col + 1 >= boardwidth || piece_row + 1 >= boardheight || piece_row - 1 < 0){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row-1][piece_col+1] || board[piece_row+1][piece_col]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row-1][piece_col+1] = ship_num;
                board[piece_row+1][piece_col] = ship_num;

            }
            break;
        case 6:
            //Rotation 1
            if(piece_rotation == 1){
                //Check if the ship can fit in the board
                if (piece_col + 1 >= boardwidth || piece_row - 2 < 0){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row-1][piece_col+1] || board[piece_row-2][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row-1][piece_col+1] = ship_num;
                board[piece_row-2][piece_col+1] = ship_num;
            } 
            //Rotation 2
            else if (piece_rotation == 2){
                //Check if the ship can fit in the board
                if (piece_col + 2 >= boardwidth || piece_row + 1 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row+1][piece_col] ||
                    board[piece_row+1][piece_col+1] || board[piece_row+1][piece_col+2]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row+1][piece_col] = ship_num;
                board[piece_row+1][piece_col+1] = ship_num;
                board[piece_row+1][piece_col+2] = ship_num;
            }
            //Rotation 3
            else if (piece_rotation == 3){
                //Check if the ship can fit in the board
                if (piece_col + 1 >= boardwidth || piece_row + 2 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row+1][piece_col] || board[piece_row+2][piece_col]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row+1][piece_col] = ship_num;
                board[piece_row+2][piece_col] = ship_num;
            }
            //Rotation 4
            else {
                //Check if the ship can fit in the board
                if (piece_col + 2 >= boardwidth || piece_row + 1 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row][piece_col+2] || board[piece_row+1][piece_col+2]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row][piece_col+2] = ship_num;
                board[piece_row+1][piece_col+2] = ship_num;
            }
            break;
        case 7:
            //Rotation 1
            if(piece_rotation == 1){
                //Check if the ship can fit in the board
                if (piece_col + 2 >= boardwidth || piece_row + 1 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row][piece_col+2] || board[piece_row+1][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row][piece_col+2] = ship_num;
                board[piece_row+1][piece_col+1] = ship_num;

            }
            //Rotation 2
            else if (piece_rotation == 2){
                //Check if the ship can fit in the board
                if (piece_col + 1 >= boardwidth || piece_row + 1 >= boardheight || piece_row - 1 < 0){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row-1][piece_col+1] || board[piece_row+1][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row-1][piece_col+1] = ship_num;
                board[piece_row+1][piece_col+1] = ship_num;
            }
            //Rotation 3
            else if (piece_rotation == 3){
                //Check if the ship can fit in the board
                if (piece_col + 2 >= boardwidth || piece_row - 1 < 0){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row][piece_col+1] ||
                    board[piece_row][piece_col+2] || board[piece_row-1][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row][piece_col+1] = ship_num;
                board[piece_row][piece_col+2] = ship_num;
                board[piece_row-1][piece_col+1] = ship_num;

            }
            //Rotation 4
            else{
                //Check if the ship can fit in the board
                if (piece_col + 1 >= boardwidth || piece_row + 2 >= boardheight){
                    //Error 302 should be raise
                    return 302;
                }

                //Check if an overlap had occurred
                if (board[piece_row][piece_col] || board[piece_row+1][piece_col] ||
                    board[piece_row+2][piece_col] || board[piece_row+1][piece_col+1]){
                        //Error 303 should be raise
                        return 303;
                }

                board[piece_row][piece_col] = ship_num;
                board[piece_row+1][piece_col] = ship_num;
                board[piece_row+2][piece_col] = ship_num;
                board[piece_row+1][piece_col+1] = ship_num;
            }
            break;
    }
    return 1;
}

int validate_ship(int piece_type, int piece_rotation, int piece_col, int piece_row, int **board, int ship_num){
    if (piece_type > 7 || piece_type < 1){
        return 300;
    }
    if (piece_rotation > 4 || piece_rotation < 1){
        return 301;
    }
    if (piece_col > boardheight || piece_col < 0 || piece_row > boardwidth || piece_row < 0){
        return 302;
    }
    printf("Validshipid: %d\n", ship_num);
    return place_ship(piece_type, piece_rotation, piece_col, piece_row, board, ship_num);

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

    int flag_b = 0;
    int flag_b2 = 0;
    int flag_i = 0;
    int flag_i2 = 0;
    int in_game = 1;
    int turn = 1;
    int win = 0;
    while (in_game) {
        char message[BUFFER_SIZE] = {0};
        memset(message, 0, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE);
        
        if (turn == 1 && conn_fd1 >= 0){
        // Receive message from Client 1 

        read(conn_fd1, buffer, BUFFER_SIZE);    
        const char *ptr = buffer + 1;
            switch(buffer[0]){
                
                case 'B':

                    if (flag_b){//If B had already been called
                        if (flag_i){//If both B and I had been called
                            sprintf(message,"E 102");
                            send(conn_fd1, message, strlen(message), 0);
                            break;
                        }
                        else {//Executes only when B is called and I is not called
                            sprintf(message,"E 101");
                            send(conn_fd1, message, strlen(message), 0);
                            break;
                        }
                    }
                    char str;
                    int width,height;
                
                    int num = sscanf(ptr, "%d %d %c", &width, &height, &str);
                    if (width < 10 || height < 10 || num != 2){
                       
                        sprintf(message,"E 200");
                        send(conn_fd1, message, strlen(message), 0);  // Send message to Client 1
                        break;
                       
                    }
                        flag_b = 1;
                        board1 = create_board(width,height);
                        board2 = create_board(width,height);
                        boardwidth = width;
                        boardheight = height;
                       
                        turn = 2;
                        //sprintf(message, "W:%d H:%d N:%d",&width, &height, &num);
                        sprintf(message, "A");
                        send(conn_fd1, message, strlen(message), 0);  // Send message to Client 1
                       
                    break;

                case 'I':

                    //If B is not called and I is intended to be called, prompt E 100
                    if(!flag_b){
                        sprintf(message,"E 100");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }
                    if (flag_i){
                        sprintf(message,"E 102");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    if (!validate_input(buffer)){
                        sprintf(message,"E 201");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }
                    
                    int ship_id = 1;
                    int is_valid = 0;
                    char stri,st;
                    int n[20] = {0};
                    int array[5] = {0};
                    int size = 0;
                    
                    sscanf(buffer, "%c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %c",&stri, &n[0], &n[1], &n[2], &n[3], &n[4], &n[5], 
                    &n[6], &n[7], &n[8], &n[9], &n[10], &n[11], &n[12], &n[13], &n[14], &n[15], &n[16], &n[17], &n[18], &n[19], &st);
                    printf("Initial board\n");
                    printBoard(board1);
                    printf("%s\n", buffer);
                    for (int i = 0; i < 20; i+=4){
                        printf("shipid: %d\n", ship_id);
                        is_valid = validate_ship(n[i], n[i+1], n[i+2], n[i+3], board1, ship_id);
                        
                        printBoard(board1);
                        
                        
                        printf("i:%d\n",i);
                        printf("Is_valid:%d\n", is_valid);
                        printf("%d %d %d %d\n",n[i], n[i+1], n[i+2], n[i+3]);
                        if (is_valid != 1) {
                            array[size] = is_valid;
                            size++;
                        }
                        ship_id++;
                    }

                    if(size){
                        int min = array[0];
                        for (int i = 1; i < size; i++){
                            if (array[i] < min){
                                min = array[i];
                            }
                        }
                            sprintf(message, "E %d", min);  // Acknowledge success
                            send(conn_fd1, message, strlen(message), 0);
                            clear_board(board1);
                            ship_id = 0;
                            break;
                    }

                   
                    if (ship_id == 6 && is_valid == 1) {
                        flag_i = 1;
                       
                       turn = 2;
                        sprintf(message, "A");  // Acknowledge success
                        send(conn_fd1, message, strlen(message), 0);
                       
                    }
                    
                    break;
                case 'S':
                   

                    if (!flag_b){
                        sprintf(message,"E 100");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    if (!flag_i){
                        sprintf(message,"E 101");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    int row, col;
                    char commands, strs;

                    if (sscanf(buffer, "%c %d %d %c", &commands, &row, &col, &strs) != 3){
                        sprintf(message,"E 202");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    if (row >= boardwidth || row < 0 || col >= boardheight || col < 0){
                        sprintf(message,"E 400");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    if (board2[row][col] == -1 || board2[row][col] == -2){
                        sprintf(message,"E 401");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    if (board2[row][col] == 0){
                        board2[row][col] = -1;
                        turn = 2;
                        sprintf(message,"R %d M", count_ship(board2));
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }
                    else{
                        
                        board2[row][col] = -2;
                        turn = 2;
                        
                        sprintf(message,"R %d H", count_ship(board2));
                        send(conn_fd1, message, strlen(message), 0);
                        if (!count_ship(board2)){
                            in_game = 0;
                            win = 1;
                        }
                        break;
                    }
                case 'Q':

                    if (!flag_b){
                        sprintf(message,"E 100");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    if (!flag_i){
                        sprintf(message,"E 101");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    sprintf(message, "G %d", count_ship(board2));
                    query(message, board2);
                    send(conn_fd1, message, strlen(message), 0);
                    break;

                case 'F':
                
                    sprintf(message, "H 0");
                    send(conn_fd1, message, strlen(message), 0);

                    
                    sprintf(message, "H 1");
                    send(conn_fd2, message, strlen(message), 0);
                   

                    in_game = 0;
                    break;
                default:


                    if (!flag_b){
                        sprintf(message,"E 100");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    if (!flag_i){
                        sprintf(message,"E 101");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }

                    sprintf(message,"E 102");
                        send(conn_fd1, message, strlen(message), 0);
                        break;

            }
        }
        else{
        
        memset(buffer, 0, BUFFER_SIZE);
        read(conn_fd2, buffer, BUFFER_SIZE);
        const char *ptr = buffer + 1;
        
            switch(buffer[0]){
                
                case 'B':
                   

                    if (flag_b2){//If B had already been called
                        if (flag_i2){//If both B and I had been called
                            sprintf(message,"E 102");
                            send(conn_fd2, message, strlen(message), 0);
                            break;
                        }
                        else {//Executes only when B is called and I is not called
                            sprintf(message,"E 101");
                            send(conn_fd2, message, strlen(message), 0);
                            break;
                        }
                    }

                    if(strcmp(buffer, "B") != 0){
                        sprintf(message,"E 200");
                        send(conn_fd2, message, strlen(message), 0);
                        break;
                    }

                    flag_b2 = 1;
                    turn = 1;
                    sprintf(message, "A");
                    send(conn_fd2, message, strlen(message), 0);
                   
                    break;

                case 'I':
                   

                    //If B is not called and I is intended to be called, prompt E 100
                    if(!flag_b2){
                        sprintf(message,"E 100");
                        send(conn_fd1, message, strlen(message), 0);
                        break;
                    }
                    if (flag_i2){
                        sprintf(message,"E 102");
                        send(conn_fd2, message, strlen(message), 0);
                        break;
                    }

                    if (!validate_input(buffer)){
                        sprintf(message,"E 201");
                        send(conn_fd2, message, strlen(message), 0);
                        break;
                    }
                    
                    int ship_id = 1;
                    int is_valid = 0;
                    char stri,st;
                    int n[20] = {0};
                    int array[5] = {0};
                    int size = 0;
                    
                    sscanf(buffer, "%c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %c",&stri, &n[0], &n[1], &n[2], &n[3], &n[4], &n[5], 
                    &n[6], &n[7], &n[8], &n[9], &n[10], &n[11], &n[12], &n[13], &n[14], &n[15], &n[16], &n[17], &n[18], &n[19], &st);
                    printf("Initial board\n");
                    printBoard(board2);
                    printf("%s\n", buffer);
                    for (int i = 0; i < 20; i+=4){
                        printf("shipid: %d\n", ship_id);
                        is_valid = validate_ship(n[i], n[i+1], n[i+2], n[i+3], board2, ship_id);
                        
                        printBoard(board2);
                        
                        
                        printf("i:%d\n",i);
                        printf("Is_valid:%d\n", is_valid);
                        printf("%d %d %d %d\n",n[i], n[i+1], n[i+2], n[i+3]);
                        if (is_valid != 1) {
                            array[size] = is_valid;
                            size++;
                        }
                        ship_id++;
                    }

                    if(size){
                        int min = array[0];
                        for (int i = 1; i < size; i++){
                            if (array[i] < min){
                                min = array[i];
                            }
                        }
                            sprintf(message, "E %d", min);  // Acknowledge success
                            send(conn_fd2, message, strlen(message), 0);
                            clear_board(board2);
                            ship_id == 0;
                            break;
                    }

                   
                    if (ship_id == 6 && is_valid == 1) {
                        flag_i2 = 1;
                       
                       turn = 1;
                        sprintf(message, "A");  // Acknowledge success
                        send(conn_fd2, message, strlen(message), 0);
                       
                    }
                    
                    break;
                case 'S':
                   

                    if (!flag_b2){
                       
                        sprintf(message,"E 100");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }

                    if (!flag_i2){
                       
                        sprintf(message,"E 101");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }

                    int row, col;
                    char commands, strs;

                    if (sscanf(buffer, "%c %d %d %c", &commands, &row, &col, &strs) != 3){
                       
                        sprintf(message,"E 202");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }

                    if (row >= boardwidth || row < 0 || col >= boardheight || col < 0){
                       
                        sprintf(message,"E 400");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }

                    if (board1[row][col] == -1 || board1[row][col] == -2){
                       
                        sprintf(message,"E 401");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }

                    if (board1[row][col] == 0){
                        board1[row][col] = -1;
                       
                       turn = 1;
                        sprintf(message,"R %d M", count_ship(board1));
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }
                    else{
                        board1[row][col] = -2;
                       
                       turn = 1;
                        sprintf(message,"R %d H", count_ship(board1));
                        send(conn_fd2, message, strlen(message), 0);
                        if (!count_ship(board1)){
                            in_game = 0;
                            win = 2;
                        }
                       
                        break;
                    }
                case 'Q':
                   

                    if (!flag_b2){
                       
                        sprintf(message,"E 100");
                        
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }

                    if (!flag_i2){
                       
                        sprintf(message,"E 101");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }
                   

                    sprintf(message, "G %d", count_ship(board1));
                    query(message, board1);
                    send(conn_fd2, message, strlen(message), 0);
                   
                    break;

                case 'F':
                   
                    sprintf(message, "H 0");
                    send(conn_fd2, message, strlen(message), 0);
                   
                    sprintf(message, "H 1");
                    send(conn_fd1, message, strlen(message), 0);
                   

                    in_game = 0;
                    break;

                default:


                    if (!flag_b2){
                       
                        sprintf(message,"E 100");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }

                    if (!flag_i2){
                       
                        sprintf(message,"E 101");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
                    }
                   
                    sprintf(message,"E 102");
                        send(conn_fd2, message, strlen(message), 0);
                       
                        break;
            }
        }
    }
    char message[BUFFER_SIZE] = {0};
    
    if (win == 1){
        memset(buffer, 0, BUFFER_SIZE);
        read(conn_fd2, buffer, BUFFER_SIZE); 
        sprintf(message,"H 0");
        send(conn_fd2, message, strlen(message), 0);
        memset(buffer, 0, BUFFER_SIZE);
        read(conn_fd1, buffer, BUFFER_SIZE); 
        sprintf(message,"H 1");
        send(conn_fd1, message, strlen(message), 0);
    }
    else if (win == 2){
        memset(buffer, 0, BUFFER_SIZE);
        read(conn_fd1, buffer, BUFFER_SIZE); 
        sprintf(message,"H 0");
        send(conn_fd1, message, strlen(message), 0);
        memset(buffer, 0, BUFFER_SIZE);
        read(conn_fd2, buffer, BUFFER_SIZE); 
        sprintf(message,"H 1");
        send(conn_fd2, message, strlen(message), 0);
        
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

//Main function to decide whether to run the server or the client
int main() {
    server_function();
    return 0;
}