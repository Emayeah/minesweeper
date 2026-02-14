const int width = 160, height = 80, mineCount = 30; // yes, global variables bad, but i need them for properly passing 2d arrays
void initBoard(int [width][height]);
void printBoard(int [width][height], int);
int userInput(int*, int*, int [width][height]);
int calcAdjacent(int, int, int [width][height], int);
void expandBoard(int, int, int [width][height]);
void cleanup();
