const int width = 20, height = 20, mineCount = 40; // yes, global variables bad, but i need them for properly passing 2d arrays
void initBoard(int [width][height]);
void printBoard(int [width][height], int);
int userInput(int*, int*, int [width][height]);
int calcAdjacent(int, int, int [width][height], int);
void expandBoard(int, int, int [width][height]);
void cleanup();
