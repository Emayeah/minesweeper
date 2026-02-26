#include <iostream>
#include <cstdlib>
#include "prototype.h"
#include "rawmode.h"
//#include <termios.h>
//#include <unistd.h>
#include <csignal>			// throwing sigint if ctrl-c, sigtstp for ctrl-z
#include <sys/ioctl.h>		// for terminal size
#include <future>			// for async
#include <mutex>			// race condition! i was experimenting with the async stuff but of course i got a race condition
#include <thread>	
#include <chrono>			// i guess i'll have to use chrono since usleep won't work for ranges < 1s
using namespace std;
/*
 * 0 = uncovered (nothing)
 * 10 = covered
 * 20 = selection tile
 * 30 = uncovered selection		// black square
 * 50 = clicked selected (orange), good ux
 * 51 = single mine
 * 61 = double mine
 * 71 = triple mine
 * 42 = 0 mine cell with single flag
 * 52 = single mine with single flag
 * 62 = double mine with single flag	// i'll have to use 1f, 2f and 3f for those, there aren't emojis for double and triple
 * 72 = triple mine with single flag	// i'm thinking of red text, a gray background on selected unclicked, wet gray shirt on clicked and white-ish on unselected
 * 43 = 0 mine cell with double flag
 * 53 = single mine with double flag
 * 63 = double mine with double flag
 * 73 = triple mine with double flag
 * 44 = 0 mine cell with triple flag
 * 54 = single mine with triple flag
 * 64 = double mine with triple flag
 * 74 = triple mine with triple flag
 * 2 = selected mine with single flag
 * 3 = selected mine with double flag
 * 4 = selected mine with triple flag
 * 101 - 124 = selection number
 * 201 - 224 = uncovered (bomb nearby)
 */
std::mutex consoleMutex;
std::mutex arrayChangeMutex;
std::mutex blockPrintMutex;
const short devBit = 0;
const short showInfoBit = 0;
volatile sig_atomic_t back_from_sigtstp = 0; // gemini aided, yes global vars bad but i kinda have to do this because signal handlers are limited

int main() {
	short gameMode = 0; // 0 for monoflag, 1 for biflag and 2 for triflag
	short width = 20, height = 20, mineCount = 30;
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);
	signal(SIGTSTP, sigtstp_handler);
	resume();
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
	short firstInput = 1;
	short win2 = 0;
	short win, x = 0, y = 0;
	short adjacent;
	short *board = new short[width * height];						// straight into the heap and not the stack, i need to free the heap to change the size (gemini aided)
	short trueMineCount = 0;
	short flagPlaced = 0;
	initBoard(board, width, height, mineCount, gameMode, &trueMineCount);
	short flag;
	short blockOutput = 0;
	short timer = 0;
	short firstInputFlag;
	short termWidth;
	short termHeight;
	struct winsize w;								// gemini
	std::future<void> idkman = std::async(std::launch::async, wordArt, &board, &width, &height, &mineCount, &gameMode, &win2, &trueMineCount, &flagPlaced, &timer); // gemini aided
	while (true) {
		win = userInput(&x, &y, board, blockOutput, 0, &width, &height, &mineCount, &gameMode, &flagPlaced); // win == 1 that means you lose because it's the game that wins against the player lol
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);		// gemini
		termWidth = w.ws_col;					 	// gemini aided
		termHeight = w.ws_row;
		blockPrintMutex.unlock();
		cout << "\e[1m";				// bold
		cout << "\e[48;5;15m";
		cout << "\e[38;5;16m";
		cout << "\e[0;" << termWidth / 4 << "H" << trueMineCount - flagPlaced;		// why print here if it's printed in wordArt? to avoid the very small delay
		cout << "\e[0;0m";
		if (win == 6) {
			win2 = 6;
			blockOutput = 1;
			blockPrintMutex.lock();
			printSettingsMenu(0, &width, &height, &mineCount, &gameMode);
			win = userInput(&x, &y, board, blockOutput, 1, &width, &height, &mineCount, &gameMode, &flagPlaced);
			if (win == 6 || win == 5) {
				arrayChangeMutex.lock();						// done by me! learned something new
				delete[] board;
				//width += 10;
				//height += 10;
				board = new short[width * height];
				initBoard(board, width, height, mineCount, gameMode, &trueMineCount);
				flagPlaced = 0;
				timer = 0;
				arrayChangeMutex.unlock();
				firstInput = 1;
			}
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);					// gemini
			termWidth = w.ws_col;									// gemini aided
			termHeight = w.ws_row;									// gemini aided	
			blockPrintMutex.unlock();
			blockOutput = 0;
			flushBuffer(&board, &width, &height, &mineCount, &gameMode, &win2);
		}
		else if (win == 5) {
			initBoard(board, width, height, mineCount, gameMode, &trueMineCount);
			flagPlaced = 0;
			timer = 0;
			flushBuffer(&board, &width, &height, &mineCount, &gameMode, &win2);
			blockOutput = 0;
			firstInput = 1;
		}
		if (firstInput == 1 && win == 1) {			// first input is always safe
			srand(time(NULL));
			short tempx, tempy;
			do {
				tempx = rand() % width;
				tempy = rand() % height;
			} while (board[tempy * width + tempx] / 10 >= 5 && board[tempy * width + tempx] / 10 <= 9 || (tempx == x && tempy == y));
			if (board[tempy * width + tempx] / 10 == 4) {
				firstInputFlag = board[tempy * width + tempx] % 10; // we somehow gotta move the mine without overwriting the flag amount
			}
			else {
				firstInputFlag = 0;
			}
			board[tempy * width + tempx] = board[y * width + x];
			board[y * width + x] = 10;
			if (firstInputFlag != 0) {
				board[tempy * width + tempx] += firstInputFlag;
			}
			win = 0;
			firstInput = 0;
		}
		else if (win == 0) {
			firstInput = 0;
		}
		if (win == 0) {
			adjacent = calcAdjacent(x, y, board, 0, width, height);
			/*
			 * 0 is a mode for calcAdjacent, 0 calcs nearby bombs, 1 calcs for nearby 0s for board expansion. there are other modes
			 */
			if (adjacent != 0) {
				adjacent += 100;
			}
			board[y * width + x] = adjacent;
			if (adjacent == 0) {
				expandBoard(x, y, board, width, height, &flagPlaced);
			}
		}
		flag = 2;
		if (win == 0) {
			/*
			 * calculate if the player has won
			 * if the loop finds any flagged or unflagged cells that do not have mines then the player hasn't yet won
			 */
			flag = 0;
			for (short i = 0; i < width && flag != 1; i++) {
				for (short j = 0; j < height && flag != 1; j++) {
					if (board[j * width + i] == 10 || board[j * width + i] / 10 == 4) {
						flag = 1;
					}
				}
			}
		}
		if (flag == 0) {
			blockPrintMutex.lock();
			printBoard(board, 0, width, height, gameMode);
			win2 = 0;
			consoleMutex.lock();								// gemini aided
			cout << "\e[" << termHeight / 2 - 1 << ";" << termWidth / 2 - 4 << "H";
			cout << " YOU WIN!";
			cout << "\e[2B";
			cout << "\e[" << termWidth / 2 - 21 << "G";
			cout << " Click titlebar for new game or ^C to exit!";
			blockOutput = 1;
			consoleMutex.unlock();								// gemini aided
		}
		else if (win == 1) {
			blockPrintMutex.lock();
			printBoard(board, 1, width, height, gameMode);
			win2 = 1;
			consoleMutex.lock();								// gemini aided
			cout << "\e[" << termHeight / 2 - 1 << ";" << termWidth / 2 - 12 << "H";
			cout << " YOU LOSE!!!!!!!!!!!!!!!!";
			cout << "\e[2B";
			cout << "\e[" << termWidth / 2 - 21 << "G";
			cout << " Click titlebar for new game or ^C to exit!";
			blockOutput = 1;
			consoleMutex.unlock(); 								// gemini aided
		}
	}
	cleanup();
	exit(EXIT_SUCCESS);
}

void initBoard(short board[], short width, short height, short mineCount, short gameMode, short *trueMineCount) {
	short x, y, flag;
	*trueMineCount = mineCount;
	for (short i = 0; i < height; i++) {
		for (short j = 0; j < width; j++) {
			board[i * width + j] = 10;
		}
	}
	srand(time(NULL));
	for (short i = 0; i < mineCount; i++) {						// this assumes that mineCount < height * width
		flag = 1;
		do {
			x = rand() % width;
			y = rand() % height;
			if (board[y * width + x] - (gameMode * 10) == 51) {
				flag = 1;
			}
			else if (board[y * width + x] == 10) {
				board[y * width + x] = 51;
				flag = rand() % (gameMode + 1);
				*trueMineCount += flag;
				board[y * width + x] += flag * 10;
				flag = 0;
			}
		} while (flag == 1);
	}
}

void printBoard(short board[], short lose, short width, short height, short gameMode) {
	short termWidth;
	short termHeight;
	short *cellVal;
	short colors[40] = { // color associative array
		4, 2, 196, 21, 88, 6, 240, 245, 56, 154,
		116, 202, 111, 5, 201, 214, 30, 77, 199, 171,
		23, 220, 105, 87, 248, 67, 62, 121, 29, 110,
		208, 190, 135, 123, 161, 41, 169, 159, 69, 99
	};
	struct winsize w;											// gemini
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);						// gemini
	termWidth = w.ws_col;									 	// gemini aided
	termHeight = w.ws_row;										// gemini aided	
	consoleMutex.lock(); // gemini
	cout << "\e[" << ((termHeight - 1) / 2) - (height / 2) + 1 << ";" << (termWidth / 2) - width + 1 << "H";	// move to the center, again, an emoji takes up 2 spaces!
	for (short i = 0; i < width; i++) {
		cout << "🟩";
	}
	cout << "\r\n";
	for (short i = 0; i < height; i++) {
		cout << "\e[" << (termWidth / 2) - width - 2 << "C";	// move to the center, again, an emoji takes up 2 spaces!
		for (short j = 0; j < width; j++) {
			cellVal = &board[i * width + j];
			if (j == 0) {
				cout << "🟩";	// board perimeter, otherwise it's hard to understand what the border is when the board is mostly uncovered
			}
			if (*cellVal == 10 || ((devBit != 1 && lose != 1) && *cellVal < 100 && *cellVal % 10 == 1)) {
				cout << "⬜";
			}
			else if (
					(*cellVal / 10 >= 5 && *cellVal / 10 <= 9) && (devBit == 1 || lose == 1) &&
					*cellVal < 100 && *cellVal % 10 == 1
					) {
				if (gameMode == 0) {
					cout << "🟥";
				}
				else {
					cout << "\e[38;5;255m";
					if (*cellVal / 10 == 5) {
						cout << "\e[48;5;196m";	// bright red
					}
					else if (*cellVal / 10 == 6) {
						cout << "\e[48;5;160m";	// not so bright red
					}
					else if (*cellVal / 10 == 7) {
						cout << "\e[48;5;88m";	// dim, blood red
					}
					else {
						cout << "\e[48;5;0m";	// black
					}
					cout << *cellVal / 10 - 4 << " " << "\e[0;0m";
				}
				if (showInfoBit == 1) {
					cout << *cellVal;
				}
			}
			else if (*cellVal == 0) {
				cout << "  ";
			}
			else if (*cellVal % 10 >= 2 && *cellVal < 100 && *cellVal % 10 <= 6 && (*cellVal > 9 || *cellVal < 2)) {
				/*
				 * basically all of the (non clicked / hovered) flags
				 * the hovered flags are printed below (together with the yellow background)
				 */
				if (gameMode == 0) {
					if (lose != 1) {
						cout << "\e[48;5;255m";
						cout << "🚩";
					}
					else {
						if (*cellVal % 10 - 1 == *cellVal / 10 - 4) {
							cout << "\e[48;5;88m";
						}
						else {
							cout << "\e[48;5;255m";
						}
						cout << "🚩";
					}
					cout << "\e[0;0m";
				}
				else {
					if (lose == 1) {
						cout << "\e[48;5;88m";
						cout << "\e[38;5;250m";
					}
					else {
						cout << "\e[48;5;250m";
						cout << "\e[38;5;1m";
					}
					cout << *cellVal % 10 - 1 << "F";
					cout << "\e[0;0m";
					if (showInfoBit == 1) {	// debugging purposes
						cout << *cellVal;
					}
				}
			}
			else if (*cellVal == 20) {		// highlighted a cell
				cout << "🟨";
			}
			else if (*cellVal == 30) {		// highlighted a blank square
				cout << "⬛";
			}
			else if (*cellVal <= 9 && *cellVal >= 2) {
				if (gameMode == 0) {
					cout << "\e[48;5;220m";	// white background
					cout << "🚩";
					cout << "\e[0;0m";
				}
				else {
					cout << "\e[48;5;220m";
					cout << *cellVal - 1 << "F";	// for multiflag because there are no multi flag emojis
					cout << "\e[0;0m";
				}
			}
			else if (*cellVal == 50) {
				cout << "🟧";				// highlighted but clicked
			}
			else {
				if ((*cellVal > 100 && *cellVal <= 200)) {
					*cellVal -= 100;
					cout << "\e[38;5;";				// foreground
				}
				else if (*cellVal > 200) {
					*cellVal -= 200;
					if (*cellVal > 40) {
						cout << "\e[38;5;202m";		// fallback, white on white is unreadable, but for some reason setting the color to 16m (pitch black) doesn't work (202 = vivid orange)
					}
					else {
						cout << "\e[38;5;16m";
					}
					cout << "\e[48;5;";				// background
				}
				if (*cellVal <= 40) {
					cout << colors[*cellVal - 1];
				}
				else {
					cout << "15";					// white fallback for undefined colors
				}
				cout << "m";
				cout << *cellVal;
				if (*cellVal < 10) {
					cout << " ";
				}
				cout << "\e[0;0m";
				*cellVal += 100;
			}
		}
		cout << "🟩";
		cout << "\r\n";
	}
	cout << "\e[" << (termWidth / 2) - width << "C"; // move to the center, again, an emoji takes up 2 spaces hence the div by 2!
	for (short i = 0; i < width; i++) {
		cout << "🟩";
	}
	consoleMutex.unlock();
}

short userInput(short *x, short *y, short board[], short lose, short openSettings, short *width, short *height, short *mineCount, short *gameMode, short *flagPlaced) {
	short valBak;
	short *cellPos;
	char input;
	short tempVal;
	short mouseValx;
	short mouseValy;
	short pressed = 1;
	short termWidth;
	short termHeight;
	short plusOrMinus;
	struct winsize w;												// disclosing the same lines as gemini is very redundant
	while (true) {
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);						// just know that the code that gets the terminal size is not done by me
		termWidth = w.ws_col;										// i won't have to use gemini because i learned how to do so
		termHeight = w.ws_row;										// but if i need to get the terminal size in a future project
		if (lose == 0) {
			valBak = board[*y * *width + *x];
			cellPos = &board[*y * *width + *x];
			if (valBak > 100) {
				*cellPos += 100;
			}
			else if (valBak % 10 >= 2 && valBak % 10 <= 6) {
				*cellPos = valBak % 10;
			}
			else if (valBak == 0) {
				*cellPos = 30;
			}
			else {
				if (pressed == 1) {
					*cellPos = 20;
				}
				else if (pressed == 0) {
					*cellPos = 50;
					pressed = 1;
				}
			}
			printBoard(board, 0, *width, *height, *gameMode);
			*cellPos = valBak;
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
					if (*y < *height - 1) {
						*y += 1;
					}
				}
				else if (input == 'C') {	// right
					if (*x < *width - 1) {
						*x += 1;
					}
				}
				else if (input == 'D'){		// left
					if (*x > 0) {
						*x -= 1;
					}
				}
				else if (input == '5') {	// page up, +5 spaces
					cin >> input;	// discard extra tilde from the escape sequence (^[[5~)
					if (*y >= 5) {
						*y -= 5;
					}
					else {
						*y = 0;
					}
				}
				else if (input == '6') {	// page down, -5 spaces
					cin >> input;	// discard extra tilde from the escape sequence (^[[6~)
					if (*y <= *height - 6) {
						*y += 5;
					}
					else {
						*y = *height - 1;
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
					if (*x <= *width - 6) {
						*x += 5;
					}
					else {
						*x = *width - 1;
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
					 * M if mouse IS pressed down (or if there is no event change)
					 * m if mouse is NOT pressed down (released)
					 */
					tempVal = getMouseVal(&pressed);
					if (
							tempVal == 0 || tempVal == 16 ||		// lclick || lclick + ctrl
							tempVal == 35 || tempVal == 51 ||		// unpressed || unpressed + ctrl
							tempVal == 2 || tempVal == 34 ||		// rclick || mouse movement while rclick is held
							tempVal == 32							// movement while lclick is held
						) {
						mouseValx = getMouseVal(&pressed) - 1;
						mouseValy = getMouseVal(&pressed); // for some reason the menu settings does not want to play ball unless i do this jankery
						if (mouseValy == 1 && pressed == 1 && (tempVal == 0 || tempVal == 16)) {
							if (mouseValx >= 9) {
								return 5;
							}
							else {
								return 6;
							}
						}
						mouseValy -= 2;
						if (openSettings == 1) {
							valBak = 0;
							if (mouseValy - (termHeight / 2 - 16 / 2) == 2) {
								valBak = 1;
								plusOrMinus = 1;
							}
							else if (mouseValy - (termHeight / 2 - 16 / 2) == 10) {
								valBak = 7;
								plusOrMinus = -1;
							}
							else if (mouseValy - (termHeight / 2 - 16 / 2) == 14) {
								valBak = 10;
							}
							if (valBak != 0) {
								if (mouseValx - (termWidth / 2 - 36 / 2) >= 7 && mouseValx - (termWidth / 2 - 36 / 2) < 10) {
									if ((tempVal == 0 || tempVal == 16) && pressed == 0) {
										if (valBak == 10) {
											*gameMode -= 1;
										}
										else {
											if (tempVal == 16) {
												*width += 9 * plusOrMinus;
											}
											*width += 1 * plusOrMinus;
										}
									}
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 16 && mouseValx - (termWidth / 2 - 36 / 2) < 19) {
									if (valBak != 10 && (tempVal == 0 || tempVal == 16) && pressed == 0) {
										if (tempVal == 16) {
											*height += 9 * plusOrMinus;
										}
										*height += 1 * plusOrMinus;
									}
									valBak += 1;
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 25 && mouseValx - (termWidth / 2 - 36 / 2) < 28) {
									if ((tempVal == 0 || tempVal == 16) && pressed == 0) {
										if (valBak == 10) {
											*gameMode += 1;
										}
										else {
											if (tempVal == 16) {
												*mineCount += 9 * plusOrMinus;
											}
											*mineCount += 1 * plusOrMinus;
										}
									}
									valBak += 2;
								}
								else {
									valBak = 0;
								}
								if ((tempVal == 0 || tempVal == 16) && pressed == 0 && valBak != 0) {
									valBak += 20;
								}
								printSettingsMenu(valBak, width, height, mineCount, gameMode);
							}
							else {
								printSettingsMenu(0, width, height, mineCount, gameMode);
							}
						}
						if (termHeight % 2 == 0) {
							mouseValy++;	// i mean i guess it does the trick
						}
						mouseValy -= termHeight / 2 - *height / 2;
						mouseValx -= (termWidth / 2) - *width;
						mouseValx /= 2;		// emojis take 2 spaces horizontally
						if (mouseValx >= 0 && mouseValx < *width) {
							*x = mouseValx;
						}
						else {
							if (mouseValx < 0) {
								*x = 0;
							}
							else {
								*x = *width - 1;
							}
						}
						if (mouseValy >= 0 && mouseValy < *height) {
							*y = mouseValy;
						}
						else {
							if (mouseValy < 0) {
								*y = 0;
							}
							else {
								*y = *height - 1;
							}
						}
						if (pressed == 1 && lose == 0) {
							if (tempVal == 0 || tempVal == 16) {
								valBak = clickLogic(x, y, board, 0, *width, *height, *gameMode, flagPlaced);
							}
							else if (tempVal == 2) {
								valBak = clickLogic(x, y, board, 1, *width, *height, *gameMode, flagPlaced);
							}
							if (valBak != 3) {
								return valBak;
							}
						}
						if (tempVal == 35 || tempVal == 51) {
							pressed = 1;
						}
					}
				}
			}
		}
		else if (input == 'd' && lose == 0) {
			valBak = clickLogic(x, y, board, 0, *width, *height, *gameMode, flagPlaced);
			if (valBak != 3) {
				return valBak;
			}
		}
		else if (input == 'f' && lose == 0) {
			valBak = clickLogic(x, y, board, 1, *width, *height, *gameMode, flagPlaced);
			if (valBak != 3) {
				return valBak;
			}
		}
	}
	return 0;
}

short calcAdjacent(short x, short y, short board[], short mode, short width, short height) {
	short count = 0;
	short *cellVal;
	for (short i = x - 1; i <= x + 1; i++) {
		for (short j = y - 1; j <= y + 1; j++) {
			if (i >= 0 && i < width && j >= 0 && j < height) { // check for out of bounds
				cellVal = &board[j * width + i];
				if (*cellVal < 100) {		// to avoid checking numbers
					if (mode == 0 && *cellVal / 10 >= 5 && *cellVal / 10 <= 9) {// this is to check the amount of mines nearby (to place a number)
						count += *cellVal / 10 - 4;
					}
					else if (mode == 2 && *cellVal % 10 >= 2 && *cellVal % 10 <= 6) {		// this is used to check if there's the right amount of flags where you're chording
						count += *cellVal % 10 - 1;
					}
					else if ((mode == 3 && *cellVal % 10 == 1) || (mode == 1 && *cellVal == 0)) {
						/*
						 * mode 3 = if you're chording and there's an unflagged mine nearby then you lose
						 * mode 1 = this is to check if there's a nearby blank square, used by expandBoard
						 */
						count++;
					}
				}
			}
		}
	}
	return count;
}

void expandBoard(short x, short y, short board[], short width, short height, short *flagPlaced) {	
	short flag, count = 1, adjacent;
	short *cellVal;
	do {
		flag = 0;
		for (short i = x - count; i <= x + count; i++) {
			for (short j = y - count; j <= y + count; j++) {
				if (i >= 0 && i < width && j >= 0 && j < height) { // check for out of bounds blah blah
					cellVal = &board[j * width + i];
					adjacent = calcAdjacent(i, j, board, 1, width, height);
					if (adjacent != 0) {
						if (*cellVal == 10 || *cellVal / 10 == 4) {				// if you flag a white spot, that flagged spot is overridden when board expansion
							flag = 1;
						}
						if (*cellVal < 100 && !(*cellVal / 10 >= 5 && *cellVal / 10 <= 9)) {
							if (*cellVal / 10 == 4) {
								*flagPlaced -= *cellVal % 10 - 1;
							}
							*cellVal = calcAdjacent(i, j, board, 0, width, height);
							if (*cellVal != 0) {
								*cellVal += 100;
							}
						}
					}
				}
			}
		}
		count++;
	} while (flag == 1);
}

void cleanup() {
	consoleMutex.lock();
	cout << "\e[?1003l\e[?1006l";
	cout << "\e[?1049l";
	cout << "\e[?25h";
	disable_raw_mode();
	cout << "\e[A" << flush;
	consoleMutex.unlock();
}

void resume() {
	consoleMutex.lock();
	cout << "\e[?1003h\e[?1006h";
	cout << "\e[?1049h";
	cout << "\e[?25l";
	cout << "\e[H";				// set cursor to home position
	cout << "\e[48;5;15m";
	cout << "\e[38;5;16m";
	cout << " Settings";
	cout << "\e[0;0m";
	enable_raw_mode();
	consoleMutex.unlock();
}

short getMouseVal(short *pressed) {
	short tempVal = 0;
	char input;
	do {
		cin >> input;
		if (input >= 48 && input <= 57) {		// ascii codes for 0 to 9, 48 being 0 and 57 being 9
			tempVal *= 10;
			tempVal += (short)(input - 48);
		}
		else {
			if (input == 'M') {
				*pressed = 0;
			}
			else if (input == 'm') {
				*pressed = 1;
			}
		}
	} while (input >= 48 && input <= 57);
	return tempVal;
}

short clickLogic(short *x, short *y, short board[], short flag, short width, short height, short gameMode, short *flagPlaced) {
	short validChord;
	short *cellVal;
	cellVal = &board[*y * width + *x];
	if (flag == 0) {														// are you trying to place a flag? flag = 0 -> no (either that is chording or normal clicking), flag = 1 -> flagging
		if (*cellVal != 0 && *cellVal < 100) {								// are you trying to chord? if you're trying to chord then *cellVal would be > 100
			if (*cellVal % 10 == 1 && *cellVal / 10 >= 5 && *cellVal / 10 <= 9) {			
				return 1;													// whoops, you just clicked on a bomb!
			}
			else if (*cellVal % 10 >= 2 && *cellVal % 10 <= 6) {
				return 3;													// do nothing, you clicked on a flag
			}
			return 0;
		}
		else if (*cellVal > 100 && *cellVal <= 200) {						// chording!
			validChord = calcAdjacent(*x, *y, board, 2, width, height);
			if (validChord != 0) {
				validChord += 100;
			}
			if (validChord == *cellVal) {
				validChord = calcAdjacent(*x, *y, board, 3, width, height);
				if (validChord != 0) {
					return 1;			// you chorded near an unflagged mine!
				}
				short temp;
				for (short i = *x - 1; i <= *x + 1; i++) {
					for (short j = *y - 1; j <= *y + 1; j++) {
						if (i >= 0 && i < width && j >= 0 && j < height) {
							cellVal = &board[j * width + i];
							if (*cellVal == 10) {
								*cellVal = calcAdjacent(i, j, board, 0, width, height);
								if (*cellVal != 0) {
									*cellVal += 100;
								}
								expandBoard(i, j, board, width, height, flagPlaced);
							} 
						}
					}
				}
			}
			return 0;
		}
	}
	else if (flag == 1 && *cellVal < 100) {			// flagging
		if (*cellVal / 10 >= 4 && *cellVal / 10 <= 9) {
			*cellVal += 1;
			if (*cellVal % 10 > gameMode + 2) {		// if you try to add 1 flag while you're at the max, that means you're trying to deflag
				if (*cellVal / 10 >= 5 && *cellVal / 10 <= 9) {
					*cellVal /= 10;
					*cellVal *= 10;
					*cellVal += 1;					// reset the amount of flags by deleting the least significant digit and setting it to 1
					*flagPlaced -= gameMode + 1;
				}
				else if (*cellVal / 10 == 4) {
					*cellVal = 10;
					*flagPlaced -= gameMode + 1;
				}
			}
		}
		else {
			if (*cellVal == 10) {
				*cellVal = 42;
			}
			else {
				return 3;	// just in case
			}
		}
		*flagPlaced += 1;
		return 2;			// flagged succesfully
	}
	return 3;				// can't flag
}

void wordArt(short **board, short *width, short *height, short *mineCount, short *gameMode, short *win, short *trueMineCount, short *flagPlaced, short *timer) {
	char word[] = "minesweeper";
	short Art = 0, termWidth, termHeight, flip = 0, oldWidth = 0, oldHeight = 0;
	while (true) {
		if (Art < 11) {
			word[Art] -= 32;
		}
		else if (Art >= 15 && Art < 26) {
			word[10 - (Art - 15)] -= 32; 
		}
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		termWidth = w.ws_col;
		termHeight = w.ws_row;
		if (termWidth != oldWidth || termHeight != oldHeight || back_from_sigtstp == 1) {
			oldWidth = termWidth;
			oldHeight = termHeight;
			flushBuffer(board, width, height, mineCount, gameMode, win);
			back_from_sigtstp = 0;
		}
		consoleMutex.lock();
		cout << "\e[0;10H";	// to not overlap the "settings"
		cout << "\e[48;5;15m";
		cout << "\e[38;5;16m";
		for (short i = 0; i < termWidth - 9; i++) {
			cout << " ";
		}
		cout << "\e[0;" << (termWidth / 2) - (11 / 2) /* 11 is the length of the word "minesweeper" */ << "H";
		cout << "\e[1m\e[38;5;16m";
		if (Art == 11 || Art == 13 || Art == 26 || Art == 28) {
			cout << "           ";					// that's 11 spaces
		}
		else {
			cout << word;
		}
		cout << "\e[0;" << termWidth / 4 << "H" << *trueMineCount - *flagPlaced; // why is it also printed here if it's printed in main? to avoid displacements if the terminal is resized
		cout << "\e[0;" << termWidth - (termWidth / 4) << "H" << *timer;
		cout << "\e[0;0m\e[22m\r" << endl;
		consoleMutex.unlock();
		if (Art < 11) {
			word[Art] += 32;
		}
		else if (Art >= 15 && Art < 26) {
			word[10 - (Art - 15)] += 32; 
		}
		flip++;
		flip %= 8;
		if (flip == 0) {
			Art++;
			*timer += 1;
		}
		if (Art >= 30) {
			Art = 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(125));	// gemini aided
	}
}

void flushBuffer(short **board, short *width, short *height, short *mineCount, short *gameMode, short *win) {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	short termWidth = w.ws_col;
	short termHeight = w.ws_row;
	arrayChangeMutex.lock();
	consoleMutex.lock();
	cout << "\e[H";
	cout << "\e[48;5;15m";
	cout << "\e[38;5;16m";
	cout << " Settings";
	cout << "\e[0;0m";
	cout << "\e[2;0H\e[0J";
	consoleMutex.unlock();
	if (blockPrintMutex.try_lock()) {							// gemini aided
		printBoard(*board, 0, *width, *height, *gameMode);
		blockPrintMutex.unlock();
	}
	else {
		if (*win == 6 || *win == 1 || *win == 0) {
			/*
			 * it is not safe to print the board while the menu settings is open: it tries to read out of bounds (if the settings have changed).
			 * the heap region pointed by board hasn't yet been resized
			 * instead of going for an extremely convoluted (and dumb) workaround, i just disabled the printing if the menu pane is open
			 */
			if (*win != 6) {
				printBoard(*board, *win, *width, *height, *gameMode);
			}
		}
		consoleMutex.lock();
		if (*win == 0) {
			cout << "\e[" << termHeight / 2 << ";" << termWidth / 2 - 4 << "H";
			cout << " YOU WIN!\r" << endl;
			cout << "\e[B";
			cout << "\r\e[" << termWidth / 2 - 22 << "C";
			cout << " Click titlebar for new game or ^C to exit!";
		}
		else if (*win == 1) {
			cout << "\e[" << termHeight / 2 << ";" << termWidth / 2 - 12 << "H";
			cout << " YOU LOSE!!!!!!!!!!!!!!!!\r" << endl;
			cout << "\e[B";
			cout << "\r\e[" << termWidth / 2 - 22 << "C";
			cout << " Click titlebar for new game or ^C to exit!";
		}
		else if (*win == 6) {
			cout << "\e[H";
			cout << "\e[48;5;15m";
			cout << "\e[38;5;16m";
			cout << " < Back  ";
			cout << "\e[0;0m";
			consoleMutex.unlock();
			printSettingsMenu(0, width, height, mineCount, gameMode);
			consoleMutex.lock();
		}
		cout << flush;
	}
	consoleMutex.unlock();
	arrayChangeMutex.unlock();
}

void printSettingsMenu(short update, short *width, short *height, short *mineCount, short *gameMode) {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	short termWidth = w.ws_col;
	short termHeight = w.ws_row;
	// menu wide 36 and tall 16
	consoleMutex.lock();
	cout << "\e[H";
	cout << "\e[48;5;15m";
	cout << "\e[38;5;16m";
	cout << " < Back  ";
	cout << "\e[48;5;233m";
	short temp = 0;
	for (short i = 0; i < 16; i++) {
		cout << "\e[" << termHeight / 2 - 16 / 2 + i + temp << ";" << termWidth / 2 - 36 / 2 + 1 << "H";
		if (i == 1) {
			for (short k = 0; k < 4; k++) {
				cout << " ";
			}
			cout << "\e[38;5;16m";
			cout << "\e[48;5;15m";
			cout << "  Width   ";
			cout << "Height ";
			cout << "Mine count ";
			cout << "\e[48;5;233m";
			for (short k = 0; k < 4; k++) {
				cout << " ";
			}
			temp = 1;
			cout <<  "\e[" << termHeight / 2 - 16 / 2 + i + 1 << ";" << termWidth / 2 - 36 / 2 + 1 << "H";
		}
		for (short j = 0; j < 36; j++) {
			cout << " ";
		}
	}
	short tempNum;
	short divCount;
	cout << "\e[38;5;16m";
	cout << "\e[48;5;15m";
	for (short i = 0; i < 4; i++) {
		cout << "\e[" << termHeight / 2 - 16 / 2 + 3 + i * 4 << ";" << termWidth / 2 - 36 / 2 + 2 << "H";
		for (short j = 0; j < 3; j++) {
			if (update != 0) {
				if (update < 20 && update == (j + 3 * i) + 1) {
					cout << "\e[48;5;82m";
				}
				else if (update >= 20 && update < 40 && (update - 20) == (j + 3 * i) + 1) {
					cout << "\e[48;5;28m";
				}
			}
			if (*width < 3) {
				*width = 3;
			}
			else if (*width > (termWidth / 2) - 10) {
				*width = (termWidth / 2) - 10;
			}
			if (*height < 3) {
				*height = 3;
			}
			else if (*height > termHeight - 7) {
				*height = termHeight - 7;
			}
			if (*width * *height - 1 < *mineCount) {
				*mineCount = *width * *height - 1;
			}
			else if (*mineCount < 1) {
				*mineCount = 1;
			}
			if (*gameMode < 0) {
				*gameMode = 0;
			}
			else if (*gameMode > 4) {
				*gameMode = 4;
			}
			cout << "\e[6C";
			if (i == 1) {
				if (j == 0) {
					tempNum = *width;
				}
				else if (j == 1) {
					tempNum = *height;
				}
				else if (j == 2) {
					tempNum = *mineCount;
				}
				if (tempNum > 99) {
					cout << "\e[D ";
				}
				else if (tempNum < 10) {
					cout << " ";
				}
				cout << tempNum;
				cout << " ";
				if (tempNum > 99) {
					cout << "\e[D";
				}
			}
			else if (i == 0) {
				cout << " + ";
			}
			else if (i == 2) {
				cout << " - ";
			}
			else if (i == 3) {
				if (j == 0) {
					cout << " - ";
				}
				else if (j == 1) {
					cout << "\e[3D";
					cout << " " << *gameMode + 1 << " mine";
					if (*gameMode != 0) {
						cout << "s ";
					}
					else {
						cout << "  ";
					}
				}
				else if (j == 2) {
					cout << "\e[3D";
					cout << " + ";
				}
			}
			cout << "\e[48;5;15m";
		}
		cout << "\r\e[4B";
	}
	cout << "\e[0;0m";
	consoleMutex.unlock();
}

void sigtstp_handler(int sig) {
	cleanup();
	consoleMutex.lock();
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGTSTP);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	signal(SIGTSTP, SIG_DFL);
	cout << flush;
	raise(SIGTSTP);
	back_from_sigtstp = 1;
	signal(SIGTSTP, sigtstp_handler);
	consoleMutex.unlock();
	resume();
}

void sigint_handler(int sig) {
	cleanup();
	consoleMutex.lock();
	cout << endl;
	exit(EXIT_SUCCESS);
}
