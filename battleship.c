#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define ROWS 8
#define COLS 5
#define MAX_SHIPS 3

// Define cell structure
typedef struct {
    int hasShip;   // 1 if there’s part of a ship, 0 otherwise
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
void placeShips(Cell grid[ROWS][COLS], const char* playerName, Ship ships[MAX_SHIPS]);
void printGrid(Cell grid[ROWS][COLS], int hideShips);
int attack(Cell grid[ROWS][COLS], int row, int col);
void clearInputBuffer();
int allShipsSunk(Cell grid[ROWS][COLS], Ship ships[MAX_SHIPS]);
void printSeparator();

int main() {
    Cell player1Grid[ROWS][COLS], player2Grid[ROWS][COLS];
    Ship player1Ships[MAX_SHIPS], player2Ships[MAX_SHIPS];
    int currentPlayer = 1;
    int gameOver = 0;

    // Initialize grids
    initGrid(player1Grid);
    initGrid(player2Grid);

    // Players place ships
    printf("=== Player 1: Place your ships ===\n");
    placeShips(player1Grid, "Player 1", player1Ships);
    printSeparator();
    printf("=== Player 2: Place your ships ===\n");
    placeShips(player2Grid, "Player 2", player2Ships);
    printSeparator();

    // Gameplay loop
    while (!gameOver) {
        int row;
        char col;
        int result;
        Cell (*opponentGrid)[COLS];
        Ship *opponentShips;

        if (currentPlayer == 1) {
            opponentGrid = player2Grid;
            opponentShips = player2Ships;
        } else {
            opponentGrid = player1Grid;
            opponentShips = player1Ships;
        }

        // Display opponent's grid
        printf("=== %s's Turn ===\n", currentPlayer == 1 ? "Player 1" : "Player 2");
        printGrid(opponentGrid, 1);

        // Get attack coordinates
        printf("Enter row (1-8) and column (A-E) for attack (e.g., 3 B): ");
        if (scanf("%d %c", &row, &col) != 2) {
            printf("Invalid input! Please enter a valid row and column.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        // Convert input to indices
        row -= 1;
        col = toupper(col);
        int colIndex = col - 'A';

        // Validate attack coordinates
        if (row >= 0 && row < ROWS && colIndex >= 0 && colIndex < COLS) {
            result = attack(opponentGrid, row, colIndex);
            if (result == 1) {
                printf("======HIT======!\n");
            } else if (result == 0) {
                printf("======MISS======\n");
            } else {
                printf("You already attacked this location. Try again.\n");
                continue;
            }

            // Check if all ships are sunk
            if (allShipsSunk(opponentGrid, opponentShips)) {
                printf("All ships of %s are sunk!\n", currentPlayer == 1 ? "Player 2" : "Player 1");
                printf("=== %s Wins! ===\n", currentPlayer == 1 ? "Player 1" : "Player 2");
                gameOver = 1;
            } else {
                // Switch players
                currentPlayer = currentPlayer == 1 ? 2 : 1;
            }
        } else {
            printf("Invalid attack location! Please try again.\n");
        }

        printSeparator();
    }

    return 0;
}

// Initialize the grid with no ships and no hits
void initGrid(Cell grid[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            grid[i][j] = (Cell){0, 0};
}

// Allow player to place ships on the grid
void placeShips(Cell grid[ROWS][COLS], const char* playerName, Ship ships[MAX_SHIPS]) {
    int shipSizes[MAX_SHIPS] = {3, 2, 2}; // Example ship sizes
    for (int s = 0; s < MAX_SHIPS; s++) {
        Ship ship;
        ship.size = shipSizes[s];
        int validPlacement = 0;

        while (!validPlacement) {
            int row;
            char col, orientation;
            printf("%s, place ship of size %d\n", playerName, ship.size);
            printf("Enter starting row (1-8): ");
            if (scanf("%d", &row) != 1) {
                printf("Invalid input! Please enter a number between 1 and 8.\n");
                clearInputBuffer();
                continue;
            }
            printf("Enter starting column (A-E): ");
            scanf(" %c", &col);
            printf("Enter orientation (H for horizontal, V for vertical): ");
            scanf(" %c", &orientation);
            clearInputBuffer();

            // Convert inputs to indices
            row -= 1;
            col = toupper(col);
            orientation = toupper(orientation);
            int colIndex = col - 'A';

            // Validate starting position
            if (row < 0 || row >= ROWS || colIndex < 0 || colIndex >= COLS) {
                printf("Invalid starting position! Please try again.\n");
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
                printf("Invalid orientation! Please enter 'H' or 'V'.\n");
                continue;
            }

            // Check if ship is within bounds
            if (ship.sternRow >= ROWS || ship.sternCol >= COLS) {
                printf("Ship goes out of bounds! Please try again.\n");
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
                printf("Ship overlaps with another ship! Please try again.\n");
                continue;
            }

            // Place the ship
            for (int i = ship.bowRow; i <= ship.sternRow; i++)
                for (int j = ship.bowCol; j <= ship.sternCol; j++)
                    grid[i][j].hasShip = 1;

            ships[s] = ship; // Store ship details
            validPlacement = 1;
            printGrid(grid, 0); // Show the grid with ships
        }
    }
}

// Print the grid to the console
void printGrid(Cell grid[ROWS][COLS], int hideShips) {
    printf("  ");
    for (char c = 'A'; c < 'A' + COLS; c++)
        printf(" %c ", c);
    printf("\n");
    for (int i = 0; i < ROWS; i++) {
        printf("%d ", i + 1);
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
            printf(" %c ", symbol);
        }
        printf("\n");
    }
}

// Process an attack on the grid
int attack(Cell grid[ROWS][COLS], int row, int col) {
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

// Clear the input buffer
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
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
void printSeparator() {
    printf("====================================\n");
}