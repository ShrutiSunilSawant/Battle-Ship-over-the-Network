#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>        // For close()
#include <arpa/inet.h>     // For socket functions
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctype.h>         // For toupper()

#define PORT 5000  //50000 for mac
#define ROWS 8
#define COLS 5
#define MAX_SHIPS 3
#define BUFFER_SIZE 1024

// Define cell structure
typedef struct {
    int hasShip;   // 1 if there's part of a ship, 0 otherwise
    int isHit;     // 1 if hit, 0 otherwise
} Cell;

// Define ship structure
typedef struct {
    int size;
    int bowRow, bowCol;    // starting position
    int sternRow, sternCol;// ending position
} Ship;

// Function prototypes
void initGrid(Cell grid[ROWS][COLS]);
void printGrid(Cell grid[ROWS][COLS], int hideShips, int client_fd);
void sendGrid(Cell grid[ROWS][COLS], int hideShips, int client_fd);
int allShipsSunk(Cell grid[ROWS][COLS], Ship ships[MAX_SHIPS]);
void sendMessage(int client_fd, const char *message);
void receiveMessage(int client_fd, char *buffer);
void placeShipsServer(Cell grid[ROWS][COLS], Ship ships[MAX_SHIPS], int client_fd);
int processAttack(Cell grid[ROWS][COLS], int row, int col);
void printSeparator(int client_fd);

int main() {
    int server_fd, client_fd[2];
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    Cell grids[2][ROWS][COLS];          // Grids for both players
    Ship ships[2][MAX_SHIPS];           // Ships for both players
    int currentPlayer = 0;              // Index for current player (0 or 1)
    int gameOver = 0;

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address to accept any connection on PORT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address))<0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for up to 2 clients
    if (listen(server_fd, 2) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Accept two clients
    for (int i = 0; i < 2; i++) {
        if ((client_fd[i] = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen))<0) {
            perror("Accept");
            exit(EXIT_FAILURE);
        }
        printf("Player %d connected.\n", i + 1);
        sendMessage(client_fd[i], "Welcome to Battleship!\n");
    }

    // Initialize grids
    initGrid(grids[0]);
    initGrid(grids[1]);

    // Players place ships
    for (int i = 0; i < 2; i++) {
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "=== Player %d: Place your ships ===\n", i + 1);
        sendMessage(client_fd[i], buffer);
        placeShipsServer(grids[i], ships[i], client_fd[i]);
        printSeparator(client_fd[i]);
    }

    // Gameplay loop
    while (!gameOver) {
        int opponent = (currentPlayer + 1) % 2;
        char buffer[BUFFER_SIZE];
        int row, colIndex, result;

        // Inform current player it's their turn
        sprintf(buffer, "=== Your Turn (Player %d) ===\n", currentPlayer + 1);
        sendMessage(client_fd[currentPlayer], buffer);

        // Send opponent's grid (hide ships)
        sendGrid(grids[opponent], 1, client_fd[currentPlayer]);

        // Prompt for attack coordinates
        sendMessage(client_fd[currentPlayer], "Enter row (1-8) and column (A-E) for attack (e.g., 3 B): ");

        // Receive attack coordinates
        receiveMessage(client_fd[currentPlayer], buffer);

        // Parse attack coordinates
        if (sscanf(buffer, "%d %c", &row, buffer) != 2) {
            sendMessage(client_fd[currentPlayer], "Invalid input! Please enter a valid row and column.\n");
            continue;
        }

        row -= 1;
        colIndex = toupper(buffer[0]) - 'A';

        // Validate attack coordinates
        if (row >= 0 && row < ROWS && colIndex >= 0 && colIndex < COLS) {
            result = processAttack(grids[opponent], row, colIndex);
            if (result == 1) {
                sendMessage(client_fd[currentPlayer], "======HIT======!\n");
            } else if (result == 0) {
                sendMessage(client_fd[currentPlayer], "======MISS======\n");
            } else {
                sendMessage(client_fd[currentPlayer], "You already attacked this location. Try again.\n");
                continue;
            }

            // Check if all ships are sunk
            if (allShipsSunk(grids[opponent], ships[opponent])) {
                sprintf(buffer, "All ships of Player %d are sunk!\n=== Player %d Wins! ===\n", opponent + 1, currentPlayer + 1);
                sendMessage(client_fd[currentPlayer], buffer);
                sendMessage(client_fd[opponent], buffer);
                gameOver = 1;
            } else {
                // Notify both players of the result
                sendGrid(grids[opponent], 1, client_fd[currentPlayer]);
                sendGrid(grids[currentPlayer], 0, client_fd[opponent]); // Send own grid to opponent
                // Switch players
                currentPlayer = opponent;
            }
        } else {
            sendMessage(client_fd[currentPlayer], "Invalid attack location! Please try again.\n");
        }
        printSeparator(client_fd[currentPlayer]);
    }

    // Close client sockets
    close(client_fd[0]);
    close(client_fd[1]);
    // Close server socket
    close(server_fd);

    return 0;
}

// Initialize the grid with no ships and no hits
void initGrid(Cell grid[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            grid[i][j] = (Cell){0, 0};
}

// Send a message to a client
void sendMessage(int client_fd, const char *message) {
    send(client_fd, message, strlen(message), 0);
}

// Receive a message from a client
void receiveMessage(int client_fd, char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
    read(client_fd, buffer, BUFFER_SIZE);
}

// Allow player to place ships (server side)
void placeShipsServer(Cell grid[ROWS][COLS], Ship ships[MAX_SHIPS], int client_fd) {
    int shipSizes[MAX_SHIPS] = {3, 2, 2}; // Example ship sizes
    char buffer[BUFFER_SIZE];

    // Display the empty grid before ship placement
    sendMessage(client_fd, "Here is your grid before placing ships:\n");
    sendGrid(grid, 0, client_fd);

    for (int s = 0; s < MAX_SHIPS; s++) {
        Ship ship;
        ship.size = shipSizes[s];
        int validPlacement = 0;

        while (!validPlacement) {
            int row;
            char col, orientation;

            // Send placement prompt
            sprintf(buffer, "Place ship of size %d\n", ship.size);
            sendMessage(client_fd, buffer);
            sendMessage(client_fd, "Enter starting row (1-8): ");
            receiveMessage(client_fd, buffer);
            if (sscanf(buffer, "%d", &row) != 1) {
                sendMessage(client_fd, "Invalid input! Please enter a number between 1 and 8.\n");
                continue;
            }

            sendMessage(client_fd, "Enter starting column (A-E): ");
            receiveMessage(client_fd, buffer);
            col = buffer[0];

            sendMessage(client_fd, "Enter orientation (H for horizontal, V for vertical): ");
            receiveMessage(client_fd, buffer);
            orientation = buffer[0];

            // Convert inputs to indices
            row -= 1;
            col = toupper(col);
            orientation = toupper(orientation);
            int colIndex = col - 'A';

            // Validate starting position
            if (row < 0 || row >= ROWS || colIndex < 0 || colIndex >= COLS) {
                sendMessage(client_fd, "Invalid starting position! Please try again.\n");
                continue;
            }

            // Set ship positions based on orientation
            if (orientation == 'H') {
                ship.bowRow = row;
                ship.bowCol = colIndex;
                ship.sternRow = row;
                ship.sternCol = colIndex + ship.size - 1;
            } else if (orientation == 'V') {
                ship.bowRow = row;
                ship.bowCol = colIndex;
                ship.sternRow = row + ship.size - 1;
                ship.sternCol = colIndex;
            } else {
                sendMessage(client_fd, "Invalid orientation! Please enter 'H' or 'V'.\n");
                continue;
            }

            // Check if ship is within bounds
            if (ship.sternRow >= ROWS || ship.sternCol >= COLS) {
                sendMessage(client_fd, "Ship goes out of bounds! Please try again.\n");
                continue;
            }

            // Check for overlap with existing ships
            int overlap = 0;
            for (int i = ship.bowRow; i <= ship.sternRow; i++)
                for (int j = ship.bowCol; j <= ship.sternCol; j++)
                    if (grid[i][j].hasShip) {
                        overlap = 1;
                        break;
                    }
            if (overlap) {
                sendMessage(client_fd, "Ship overlaps with another ship! Please try again.\n");
                continue;
            }

            // Place the ship
            for (int i = ship.bowRow; i <= ship.sternRow; i++)
                for (int j = ship.bowCol; j <= ship.sternCol; j++)
                    grid[i][j].hasShip = 1;

            ships[s] = ship; // Store ship details
            validPlacement = 1;

            // Show the grid with ships after each placement
            sendMessage(client_fd, "Updated grid:\n");
            sendGrid(grid, 0, client_fd);
        }
    }
}

// Send the grid to the client
void sendGrid(Cell grid[ROWS][COLS], int hideShips, int client_fd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    strcat(buffer, "  ");
    for (char c = 'A'; c < 'A' + COLS; c++) {
        char temp[4];
        sprintf(temp, " %c ", c);
        strcat(buffer, temp);
    }
    strcat(buffer, "\n");
    for (int i = 0; i < ROWS; i++) {
        char temp[4];
        sprintf(temp, "%d ", i + 1);
        strcat(buffer, temp);
        for (int j = 0; j < COLS; j++) {
            char symbol = '~'; // Water symbol
            if (grid[i][j].isHit && grid[i][j].hasShip)
                symbol = 'X'; // Hit ship
            else if (grid[i][j].isHit)
                symbol = 'o'; // Missed shot
            else if (grid[i][j].hasShip && !hideShips)
                symbol = 'S'; // Ship
            else
                symbol = '~'; // Water
            sprintf(temp, " %c ", symbol);
            strcat(buffer, temp);
        }
        strcat(buffer, "\n");
    }
    sendMessage(client_fd, buffer);
}

// Process an attack on the grid
int processAttack(Cell grid[ROWS][COLS], int row, int col) {
    if (grid[row][col].isHit) {
        return -1; // Already attacked
    } else {
        grid[row][col].isHit = 1;
        if (grid[row][col].hasShip) {
            return 1; // Hit
        } else {
            return 0; // Miss
        }
    }
}

// Check if all ships are sunk
int allShipsSunk(Cell grid[ROWS][COLS], Ship ships[MAX_SHIPS]) {
    for (int s = 0; s < MAX_SHIPS; s++) {
        Ship ship = ships[s];
        for (int i = ship.bowRow; i <= ship.sternRow; i++)
            for (int j = ship.bowCol; j <= ship.sternCol; j++)
                if (!grid[i][j].isHit)
                    return 0; // Ship is not fully hit
    }
    return 1; // All ships are sunk
}

// Print a separator line
void printSeparator(int client_fd) {
    sendMessage(client_fd, "====================================\n");
}