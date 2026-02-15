#include <iostream>
#include <cstdlib>
#include "prototype.h"		// width, height and minecount are stored here!
#include "rawmode.h"
//#include <termios.h>
//#include <unistd.h>		// for sleeping
#include <csignal>			// throwing sigint if ctrl-c, sigtstp for ctrl-z
#include <sys/ioctl.h>		// for terminal size
#include <thread>			// for sleep
#include <future>			// for async
#include <chrono>
using namespace std;
/*
 * 0 = uncovered (nothing)
 * 1 - 8 = uncovered (bomb nearby)
 * 9 = mine
 * 10 = covered
 * 11 = selection tile
 * 19 = mine with flag
 * 20 = covered with flag
 * 21 - 28 = selection number
 * 30 = uncovered selection
 * 39 = selected mine with flag
 * 40 = selected tile with flag
 * 50 = clicked selected (orange), good ux
 */
const int devBit = 0;
const int debugBit = 0;

int main() {
	cout << "\e[?1049h";		// alternate screen buffer
	cout << "\e[?1003h\e[?1006h";	// set any-event to high and sgr to high for the mouse button release
	cout << "\e[?25l";			// set cursor to low
	cout << "\e[H";				// set cursor to home position
	cout << "\e[B";				// one lower to not overlap titlebar
	enable_raw_mode();
	/*
	 * some sections that weren't covered by the teacher were partially done by google gemini
	 * my stance on ai for programming is very clear: it's a tool for learning.
	 * most of the raw mode toggle function is provided by gemini,
	 * but i always analyze ai code before pasting it.
	 * vibe coding and slight llm aid are completely different things
	 * one's for losers who are afraid of learning
	 * the other is for people willing to learn new things.
	 * also the actual logic and handling raw code is fully gonna be made by me
	 */
	int firstInput = 1;
	int win, x = 0, y = 0;
	int adjacent;
	int board[width][height];
	initBoard(board);
	int flag;
	//wordArt();
	std::future<void> idkman = std::async(std::launch::async, wordArt);
	//std::thread printTitle(wordArt);	// gemini
	//printTitle.detach();				// gemini
	do {
		win = userInput(&x, &y, board); // win == 1 that means you lose because it's the game that wins against the player lol
		if (firstInput == 1 && win == 1) { // first input is always safe
			srand(time(NULL));
			int tempx, tempy;
			do {
				tempx = rand() % width;
				tempy = rand() % height;
			} while (board[tempx][tempy] == 9 && (tempx != x || tempy != y));
			board[tempx][tempy] = 9;
			board[x][y] = 10;
			win = 0;
			firstInput = 0;
		}
		else if (win == 0) {
			firstInput = 0;
		}
		if (win == 0) {
			adjacent = calcAdjacent(x, y, board, 0); // 0 is a mode for calcAdjacent, 0 calcs nearby bombs, 1 calcs for nearby 0s for board expansion
			board[x][y] = adjacent;
			if (adjacent == 0) {
				expandBoard(x, y, board);
			}
		}
		flag = 2;
		if (win == 0) {
			flag = 0;
			for (int i = 0; i < width && flag != 1; i++) {
				for (int j = 0; j < height && flag != 1; j++) {
					if (board[i][j] == 10 || board[i][j] == 20) {
						flag = 1;
					}
				}
			}
		}
		if (flag == 0) {
			flag = 3;
			win = 1;
		}
	} while (win != 1 && win != 4);
	printBoard(board, 1);
	cleanup();
	if (flag == 3 && win != 4) {
		cout << "\e[1B";
		cout << "YOU WIN!\r" << endl;
	}
	else if (flag != 4 && win != 4) {
		cout << "\e[1B";
		cout << "YOU LOSE!!!!!!!!!!!!!!\r" << endl;
	}
	if (win == 4) {
		raise(SIGINT);
	}
}

void initBoard(int board[width][height]) {
	int x, y;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			board[i][j] = 10;
		}
	}
	srand(time(NULL));
	for (int i = 0; i < mineCount; i++) { // this assumes that mineCount < height * width
		x = rand() % width;
		y = rand() % height;
		while (board[x][y] == 9) {
			x = rand() % width;
			y = rand() % height;
		}
		board[x][y] = 9;
	}
}

void printBoard(int board[width][height], int lose) {
	int termWidth;
	int termHeight;
	char word[] = "minesweeper";
	struct winsize w;										// gemini
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);					// gemini
	// The ioctl call returns 0 on success, -1 on error		// gemini
	termWidth = w.ws_col;									// gemini aided
	termHeight = w.ws_row;									// gemini aided	
	/*
	 * just wanted to get the terminal size to adjust the logic
	 */
//	cout << "\e[48;5;15m";
//	for (int i = 0; i < termWidth; i++) {
//		cout << " ";
//	}
//	cout << '\r';
//	cout << "\e[" << (termWidth / 2) - (11 / 2) /* 11 is the length of the word "minesweeper" */ << "C"; // hardcoded to my terminal size
//	cout << "\e[1m\e[38;5;16mMinesweeper";
//	cout << "\e[0;0m\e[22m\r" << endl;
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			if (board[i][j] == 10 || ((devBit != 1 && lose != 1) && board[i][j] == 9)) {
				cout << "⬜";
			}
			else if ((devBit == 1 || lose == 1) && board[i][j] == 9 || (board[i][j] == 19 && lose == 1)) {
				cout << "🟥";
			}
			else if (board[i][j] == 0) {
				cout << "  ";
			}
			else if (board[i][j] == 20 || board[i][j] == 19) {
				if (lose != 1) {
					cout << "🚩";
				}
				else {
					cout << "🏳️";
				}
			}
			else if (board[i][j] == 11) {
				cout << "🟨";
			}
			else if (board[i][j] == 30) {
				cout << "⬛";
				board[i][j] = 0;
			}
			else if (board[i][j] == 39 || board[i][j] == 40) {
				cout << "🏁";
				board[i][j] -= 20;
			}
			else if (board[i][j] == 50) {
				cout << "🟧";
			}
			else {
				if (board[i][j] <= 20) {
					switch (board[i][j]) { // print with colors
						case 1:
							cout << "\e[0;34m";
							break;
						case 2:
							cout << "\e[0;32m";
							break;
						case 3:
							cout << "\e[38;5;196m";
							break;
						case 4:
							cout << "\e[38;5;18m";
							break;
						case 5:
							cout << "\e[38;5;88m";
							break;
						case 6:
							cout << "\e[0;36m";
							break;
						case 7:
							cout << "\e[38;5;240m";
							break;
						case 8:
							cout << "\e[38;5;245m";
							break;
					}
				}
				else {
					board[i][j] -= 20;
					switch (board[i][j]) { // print with colors
						case 1:
							cout << "\e[0;44m";
							break;
						case 2:
							cout << "\e[0;42m";
							break;
						case 3:
							cout << "\e[48;5;196m";
							break;
						case 4:
							cout << "\e[48;5;18m";
							break;
						case 5:
							cout << "\e[48;5;88m";
							break;
						case 6:
							cout << "\e[0;46m";
							break;
						case 7:
							cout << "\e[48;5;240m";
							break;
						case 8:
							cout << "\e[48;5;245m";
							break;
					}

				}
				cout << board[i][j] << " " << "\e[0;0m";
//				cout << board[i][j] << " ";
			}
		}
		cout << "\r" << endl;
	}
	cout << "\e[" << height << "A";
}

int userInput(int* x, int* y, int board[width][height]) {
	char flag;
	int temp;
	int validChord;
	char input;
	int tempVal;
	int mouseValx;
	int mouseValy;
	int pressed = 1;
	int logicTemp;
	do {
		do {
			temp = board[*x][*y];
			if ((board[*x][*y] > 8 || board[*x][*y] < 1) && board[*x][*y] != 0 && board[*x][*y] != 19 && board[*x][*y] != 20) {
				if (pressed == 1) {
					board[*x][*y] = 11;
					printBoard(board, 0);
					board[*x][*y] = temp;
				}
				else if (pressed == 0) {
					board[*x][*y] = 50;
					printBoard(board, 0);
					board[*x][*y] = temp;
					pressed = 1;
				}
			}
			else if (board[*x][*y] <= 8 && board[*x][*y] >= 1 || (board[*x][*y] == 19 || board[*x][*y] == 20)) {
				board[*x][*y] += 20;
				printBoard(board, 0);
			}
			else if (board[*x][*y] == 0) {
				board[*x][*y] = 30;
				printBoard(board, 0);
			}
			cin >> input;
			if (input == '\e') {
				cin >> input;
				if (input == '[') {
					cin >> input;
					if (input == 'A') {			// up
						if (*y > 0) {
							*y -= 1;
						}
					}
					else if (input == 'B') {	// down
						if (*y < height - 1) {
							*y += 1;
						}
					}
					else if (input == 'C') {	// right
						if (*x < width - 1) {
							*x += 1;
						}
					}
					else if (input == 'D'){		// left
						if (*x > 0) {
							*x -= 1;
						}
					}
					else if (input == '5') {	// page up, +5 spaces
						cin >> input; // discard extra tilde from the escape sequence (^[[5~)
						if (*y >= 5) {
							*y -= 5;
						}
						else {
							*y = 0;
						}
					}
					else if (input == '6') {	// page down, -5 spaces
						cin >> input; // discard extra tilde from the escape sequence (^[[6~)
						if (*y <= height - 6) {
							*y += 5;
						}
						else {
							*y = height - 1;
						}
					}
					else if (input == 'H') {	// home, -5 spaces
						if (*x >= 5) {
							*x -= 5;
						}
						else {
							*x = 0;
						}
					}
					else if (input == 'F') {	// end, +5 spaces
						if (*x <= width - 6) {
							*x += 5;
						}
						else {
							*x = width - 1;
						}
					}
					else if (input == '<') {	// mouse driven controls
						/*
						 * format: ESC[<z;x;ym
						 * z = 0 if left click
						 * z = 2 if right click
						 * z = 35 if nothing is pressed
						 * z = 34 if movement while right click is pressed
						 * z = 23 if movement while left click is pressed
						 * x denotes the x coordinates (horizontal) (absolute value)
						 * y denotes the y coordinates (vertical) also absolute
						 * M if mouse NOT pressed down
						 * m if mouse IS pressed down
						 */
						tempVal = getMouseVal(&pressed);
						if (tempVal == 35) {	// left click
							mouseValx = getMouseVal(&pressed);
							mouseValx--;
							mouseValx /= 2;		// emojis take 2 spaces horizontally
							mouseValy = getMouseVal(&pressed);
							mouseValy--;		// but not vertically! although there is a small offset
							if (mouseValx >= 0 && mouseValx < width) {
								*x = mouseValx;
							}
							if (mouseValy >= 0 && mouseValy < height) {
								*y = mouseValy;
							}
							pressed = 1;
						}
						else if (tempVal == 0) {
							mouseValx = getMouseVal(&pressed);
							mouseValx--;
							mouseValx /= 2;		// emojis take 2 spaces horizontally
							mouseValy = getMouseVal(&pressed);
							mouseValy--;		// but not vertically! although there is a small offset
							if (mouseValx >= 0 && mouseValx < width) {
								*x = mouseValx;
							}
							if (mouseValy >= 0 && mouseValy < height) {
								*y = mouseValy;
							}
							if (pressed == 1) {
								logicTemp = clickLogic(x, y, board, 0);
								if (logicTemp != 3) {
									return logicTemp;
								}
							}
						}
						else if (tempVal == 2) {
							mouseValx = getMouseVal(&pressed);
							mouseValx--;
							mouseValx /= 2;		// emojis take 2 spaces horizontally
							mouseValy = getMouseVal(&pressed);
							mouseValy--;		// but not vertically! although there is a small offset
							if (mouseValx >= 0 && mouseValx < width) {
								*x = mouseValx;
							}
							if (mouseValy >= 0 && mouseValy < height) {
								*y = mouseValy;
							}
							if (pressed == 1) {
								logicTemp = clickLogic(x, y, board, 1);
								if (logicTemp != 0) {
									return logicTemp;
								}
							}
						}
						else if (tempVal == 34 || tempVal == 32) {
							mouseValx = getMouseVal(&pressed);
							mouseValx--;
							mouseValx /= 2;		// emojis take 2 spaces horizontally
							mouseValy = getMouseVal(&pressed);
							mouseValy--;		// but not vertically! although there is a small offset
							if (mouseValx >= 0 && mouseValx < width) {
								*x = mouseValx;
							}
							if (mouseValy >= 0 && mouseValy < height) {
								*y = mouseValy;
							}
						}
					}
				}
			}
			else if (input == '\x03') { // sigint
				return 4;
			}
			else if (input == '\x1a') {
				cleanup();
				signal(SIGTSTP, SIG_DFL);
				raise(SIGTSTP);
				enable_raw_mode();
				cout << "\e[?25l" << flush;
			}
			else if (input != 'f') {
				logicTemp = clickLogic(x, y, board, 0);
				if (logicTemp != 3) {
					return logicTemp;
				}
			}
			else if (input == 'f' && !(board[*x][*y] >= 0 && board[*x][*y] <= 8)) {
				logicTemp = clickLogic(x, y, board, 1);
				if (logicTemp != 0) {
					return logicTemp;
				}
			}
		} while (true);//(*x < 0 || *x > width);
	} while (true); 
	return 0;
}
int calcAdjacent(int x, int y, int board[width][height], int mode) {
	int count = 0;
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) { // check for out of bounds
				if (mode == 0 && (board[x + i][y + j] == 9 || board[x + i][y + j] == 19)) {
					count++;
				}
				else if (mode == 1 && board[x + i][y + j] == 0) {
					return 1; // i know, i know, jacopini... va bene che compiler optimization tolgono le altre condizioni ma questo è più semplice da leggere
				}
				else if (mode == 2 && (board[x + i][y + j] == 20 || board[x + i][y + j] == 19)) {
					count++;
				}
				else if (mode == 3 && board[x + i][y + j] == 9) {
					count++;
				}
			}
		}
	}
	return count;
}
void expandBoard(int x, int y, int board[width][height]) {	
	int flag, count = 1, adjacent;
	char asdf;
	do {
		flag = 0;
		for (int i = -count; i <= count; i++) {
			for (int j = -count; j <= count; j++) {
				if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) { //&& board[x + i][y + j] != 9 && board[x + i][y + j] != 19) { // check for out of bounds blah blah
					adjacent = calcAdjacent(x + i, y + j, board, 1);
					if (adjacent == 1) {
						if (board[x + i][y + j] == 10 || board[x + i][y + j] == 20) {
							flag = 1;
						}
						if (board[x + i][y + j] != 19) {
							board[x + i][y + j] = calcAdjacent(x + i, y + j, board, 0);
						}
					}
				}
			}
		}
		if (debugBit == 1) {
			printBoard(board, 0);
			cin >> asdf;
		}
		count++;
	} while (flag == 1);
}

void cleanup() {
	cout << "\e[" << height << "B";
	cout << "\e[?1003l\e[?1006l";
	cout << "\e[?25h";
	cout << "\e[?1049l" << flush;
	cout << "\e[1A" << flush;
	disable_raw_mode();
}

int getMouseVal(int* pressed) {
	int tempVal = 0;
	char input;
	do {
		cin >> input;
		if ((int)(input - 48) >= 0 && (int)(input - 48) <= 9) {
			tempVal *= 10;
			tempVal += (int)(input - 48);
		}
		else {
			if (input == 'M') {
				*pressed = 0;
			}
			else if (input == 'm') {
				*pressed = 1;
			}
		}
	} while ((int)(input - 48) >= 0 && (int)(input - 48) <= 9);
	return tempVal;
}
int clickLogic(int* x, int* y, int board[width][height], int flag) {
	int validChord;
	if (flag == 0) {
		if (board[*x][*y] != 0 && board[*x][*y] != 19 && board[*x][*y] != 20 && !(board[*x][*y] >= 1 && board[*x][*y] <= 8)) {
			if (board[*x][*y] == 9) {
				return 1;
			}
			return 0;
		}
		else if (board[*x][*y] >= 1 && board[*x][*y] <= 8) {
			validChord = calcAdjacent(*x, *y, board, 2);
			if (validChord == board[*x][*y]) {
				validChord = calcAdjacent(*x, *y, board, 3);
				if (validChord != 0) {
					return 1;
				}
				for (int i = -1; i < 2; i++) {
					for (int j = -1; j < 2; j++) {
						if (*x + i >= 0 && *x + i < width && *y + j >= 0 && *y + j < height) {
							if (board[*x + i][*y + j] == 10) {
								board[*x + i][*y + j] = calcAdjacent(*x + i, *y + j, board, 0);
								expandBoard(*x + i, *y + j, board);
							} 
						}
					}
				}
			}
		return 0;
		}
	}
	else if (flag == 1 && !(board[*x][*y] >= 1 && board[*x][*y] <= 8)) {
		if (!(board[*x][*y] >= 0 && board[*x][*y] <= 8)) {
			if (board[*x][*y] == 19 || board[*x][*y] == 20) {
				board[*x][*y] -= 10;
			}
			else {
				board[*x][*y] += 10;
			}
			return 2;
		}
		else {
			return 0;
		}
	}
	return 3;
}
void wordArt() {
	char word[] = "minesweeper";
	int Art = 0, termWidth, termHeight;
	do {
		if (Art < 11) {
			word[Art] -= 32;
		}
		else if (Art >= 15 && Art < 26) {
			word[10 - (Art - 15)] -= 32; 
		}
		struct winsize w;										// gemini
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);					// gemini
		// The ioctl call returns 0 on success, -1 on error		// gemini
		termWidth = w.ws_col;									// gemini aided
		termHeight = w.ws_row;									// gemini aided	
		/*
		 * just wanted to get the terminal size to adjust the logic
		 */
		cout << "\e[H";
		cout << "\e[48;5;15m";
		for (int i = 0; i < termWidth; i++) {
			cout << " ";
		}
		cout << '\r';
		cout << "\e[" << (termWidth / 2) - (11 / 2) /* 11 is the length of the word "minesweeper" */ << "C"; // hardcoded to my terminal size
		cout << "\e[1m\e[38;5;16m";
		if (Art == 11 || Art == 13 || Art == 26 || Art == 28) {
			cout << "           "; // that's 11 spaces
		}
		else {
			cout << word;
		}
		cout << "\e[0;0m\e[22m\r" << endl;
		if (Art < 11) {
			word[Art] += 32;
		}
		else if (Art >= 15 && Art < 26) {
			word[10 - (Art - 15)] += 32; 
		}
		Art++;
		if (Art >= 30) {
			Art = 0;
		}
		sleep(1);
	} while (true);
}
