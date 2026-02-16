int width = 20, height = 20, mineCount = 20; // yes, global variables bad, but i need them for properly passing 2d arrays
void initBoard(int []);
void printBoard(int [], int);
int userInput(int*, int*, int [], int, int);
int calcAdjacent(int, int, int [], int);
void expandBoard(int, int, int []);
void cleanup();
int getMouseVal(int*);
int clickLogic(int*, int*, int[], int);
void wordArt(int[]);
void clearBuffer();
void printSettingsMenu(int);
void asyncClearBuffer();
