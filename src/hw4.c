#include <stdio.h>      // For input/output functions like printf and perror
#include <stdlib.h>     // For memory allocation, process control, conversions, and other utilities
#include <string.h>     // For handling string operations like strlen and strcpy
#include <unistd.h>     // For close() and other POSIX operating system API functions
#include <arpa/inet.h>  // For internet address operations (e.g., sockaddr_in, inet_pton)
#include <pthread.h>    // For multi-threading support
#include <asm-generic/socket.h>

#define PORT1 2201      // Port for Client 1
#define PORT2 2202      // Port for Client 2
#define SERVER_IP "127.0.0.1" // Server IP address (localhost)

typedef struct {
    int socket_fd;               // The socket file descriptor for this client connection
    struct sockaddr_in address;  // Stores the address information for this client
} Client;

// Function Prototypes
void *handle_client(void *arg); // Function to handle communication with each client
void start_server();            // Function to start the server and wait for connections
void start_client(int port);    // Function to start a client and connect to the server

// -------------------------------------------
// Function to Handle Each Client
// This function will run in a separate thread for each client.
// It reads messages from the client and responds back to it.
// -------------------------------------------
void *handle_client(void *arg) {
    Client *client = (Client *)arg; // Convert argument to a Client pointer
    char buffer[1024] = {0};        // Buffer to store incoming messages from client

    // Infinite loop to keep reading messages from the client
    while (1) {
        // Read message from client into buffer
        int valread = read(client->socket_fd, buffer, sizeof(buffer));

        // If valread <= 0, the client has disconnected, so break out of the loop
        if (valread <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        // Print the received message for debugging or logging
        printf("Received from client: %s\n", buffer);

        // Send a response back to the client
        // Here you can customize the response based on the message received.
        char *response = "Message received by server\n";
        send(client->socket_fd, response, strlen(response), 0);
    }

    // Close the client socket when done
    close(client->socket_fd);
    return NULL;
}

// -------------------------------------------
// Server Function
// This function initializes the server, binds to the specified ports,
// and listens for incoming connections from two clients.
// -------------------------------------------
void start_server() {
    int server_fd1, server_fd2;
    struct sockaddr_in address1, address2;
    int opt = 1;

    // Step 1: Create socket for Client 1
    if ((server_fd1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed for Player 1");
        exit(EXIT_FAILURE);
    }

    // Step 2: Set socket options for Client 1
    if (setsockopt(server_fd1, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
        perror("Setting socket options failed for Player 1");
        close(server_fd1);
        exit(EXIT_FAILURE);
    }

    // Step 3: Configure address structure for Client 1 and bind
    address1.sin_family = AF_INET;
    address1.sin_addr.s_addr = INADDR_ANY;
    address1.sin_port = htons(PORT1); // Port for Client 1

    if (bind(server_fd1, (struct sockaddr *)&address1, sizeof(address1)) < 0) {
        perror("Bind on PORT1 failed");
        close(server_fd1);
        exit(EXIT_FAILURE);
    }

    // Step 4: Listen on PORT1 for Client 1
    listen(server_fd1, 3);
    printf("Waiting for Player 1 to connect on port %d...\n", PORT1);

    // Step 5: Accept connection from Client 1
    Client client1;
    int addrlen1 = sizeof(address1);
    client1.socket_fd = accept(server_fd1, (struct sockaddr *)&client1.address, (socklen_t *)&addrlen1);
    if (client1.socket_fd < 0) {
        perror("Accept failed for Player");
        close(server_fd1);
        exit(EXIT_FAILURE);
    }
    printf("Player 1 connected.\n");

    // Step 6: Create a separate socket for Client 2
    if ((server_fd2 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed for Player 2");
        exit(EXIT_FAILURE);
    }

    // Step 7: Set socket options for Client 2
    if (setsockopt(server_fd2, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
        perror("Setting socket options failed for Player 2");
        close(server_fd2);
        exit(EXIT_FAILURE);
    }

    // Step 8: Configure address structure for Client 2 and bind
    address2.sin_family = AF_INET;
    address2.sin_addr.s_addr = INADDR_ANY;
    address2.sin_port = htons(PORT2); // Port for Client 2

    if (bind(server_fd2, (struct sockaddr *)&address2, sizeof(address2)) < 0) {
        perror("Bind on PORT2 failed");
        close(server_fd2);
        exit(EXIT_FAILURE);
    }

    // Step 9: Listen on PORT2 for Client 2
    listen(server_fd2, 3);
    printf("Waiting for Player 2 to connect on port %d...\n", PORT2);

    // Step 10: Accept connection from Client 2
    Client client2;
    int addrlen2 = sizeof(address2);
    client2.socket_fd = accept(server_fd2, (struct sockaddr *)&client2.address, (socklen_t *)&addrlen2);
    if (client2.socket_fd < 0) {
        perror("Accept failed for Player 2");
        close(server_fd2);
        exit(EXIT_FAILURE);
    }
    printf("Player 2 connected.\n");

    // Step 11: Create threads to handle each client concurrently
    pthread_t client1_thread, client2_thread;
    pthread_create(&client1_thread, NULL, handle_client, (void *)&client1);
    pthread_create(&client2_thread, NULL, handle_client, (void *)&client2);

    // Wait for both threads to finish
    pthread_join(client1_thread, NULL);
    pthread_join(client2_thread, NULL);

    // Step 12: Close both server sockets once done
    close(server_fd1);
    close(server_fd2);
}

// -------------------------------------------
// Client Function
// This function sets up a client, connects it to the server,
// and sends a message to the server. It then receives a response.
// -------------------------------------------
void start_client(int port) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0}; // Buffer for server responses

    // Step 1: Create client socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return;
    }

    // Set up server address structure
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_port = htons(port); // Connect to specified port (PORT1 or PORT2)

    // Convert server IP address to binary form and store it in serv_addr
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address\n");
        return;
    }

    // Step 2: Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection failed\n");
        return;
    }

    // Step 3: Send a message to the server
    char *message = "Hello from client";
    send(sock, message, strlen(message), 0);
    printf("Message sent to server.\n");

    // Step 4: Read response from server
    int valread = read(sock, buffer, 1024);
    printf("Server response: %s\n", buffer);

    // Close the client socket when done
    close(sock);
}

// -------------------------------------------
// Main Function
// Determines whether to run the program as a server or client
// based on command-line arguments.
// -------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <server|player1|player2>\n", argv[0]);
        return 1;
    }

    // If argument is "server", start the server
    if (strcmp(argv[1], "server") == 0) {
        start_server();

    // If argument is "player1", start Player 1 (connects to PORT1)
    } else if (strcmp(argv[1], "player1") == 0) {
        start_client(PORT1);

    // If argument is "player2", start Player 2 (connects to PORT2)
    } else if (strcmp(argv[1], "player2") == 0) {
        start_client(PORT2);

    // If argument is invalid, show usage information
    } else {
        printf("Invalid argument. Use 'server', 'player1', or 'player2'.\n");
        return 1;
    }

    return 0;
}
