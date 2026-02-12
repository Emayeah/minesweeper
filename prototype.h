const int width = 20, height = 20, mineCount = 20; // yes, global variables bad, but i need them for properly passing 2d arrays
void initBoard(int [width][height]);
void printBoard(int [width][height]);
int userInput(int*, int*, int [width][height]);
void expandBoard(int, int, int [width][height]);
