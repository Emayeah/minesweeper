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
 * 82 = selected mine with single flag
 * 83 = selected mine with double flag
 * 84 = selected mine with triple flag
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
	short **board2 = &board;
	short trueMineCount = 0;
	short flagPlaced = 0;
	initBoard(board, width, height, mineCount, gameMode, &trueMineCount);
	short flag;
	short blockOutput = 0;
	short timer = 0;
	short firstInputFlag;
	short termWidth;
	short termHeight;
	struct winsize w;											// gemini
	std::future<void> idkman = std::async(std::launch::async, wordArt, board2, &width, &height, &mineCount, &gameMode, &win2, &trueMineCount, &flagPlaced, &timer); // gemini aided
	do {
		win = userInput(&x, &y, board, blockOutput, 0, &width, &height, &mineCount, &gameMode, &flagPlaced); // win == 1 that means you lose because it's the game that wins against the player lol
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);						// gemini
		termWidth = w.ws_col;									 	// gemini aided
		blockPrintMutex.unlock();
		cout << "\e[1m";				// bold
		cout << "\e[48;5;15m";
		cout << "\e[38;5;16m";
		cout << "\e[0;" << termWidth / 4 << "H" << trueMineCount - flagPlaced;		// why prshort here if it's printed in wordArt? to avoid the very small delay
		cout << "\e[0;0m";
		if (win == 6) {
			win2 = 6;
			blockOutput = 1;
			blockPrintMutex.lock();
			printSettingsMenu(0, &width, &height, &mineCount, &gameMode);
			win = userInput(&x, &y, board, blockOutput, 1, &width, &height, &mineCount, &gameMode, &flagPlaced);
			if (win == 6) {
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
			blockPrintMutex.unlock();
			blockOutput = 0;
			flushBuffer(board, &width, &height, &mineCount, &gameMode, &win2);
		}
		else if (win == 5) {
			initBoard(board, width, height, mineCount, gameMode, &trueMineCount);
			flagPlaced = 0;
			timer = 0;
			flushBuffer(board, &width, &height, &mineCount, &gameMode, &win2);
			blockOutput = 0;
			firstInput = 1;
		}
		else if (win == 2) {
			flagPlaced += 1;
		}
		else if (win == 8) {
			flagPlaced -= 1 + gameMode;
		}
		if (firstInput == 1 && win == 1) {			// first input is always safe
			srand(time(NULL));
			short tempx, tempy;
			do {
				tempx = rand() % width;
				tempy = rand() % height;
			} while (board[tempy * width + tempx] / 10 >= 5 && board[tempy * width + tempx] / 10 <= 7 || (tempx == x && tempy == y));
			if (board[tempy * width + tempx] / 10 == 4) {
				firstInputFlag = board[tempy * width + tempx] % 10;
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
			flag = 0;
			for (short i = 0; i < width && flag != 1; i++) {
				for (short j = 0; j < height && flag != 1; j++) {
					if (board[j * width + i] == 10 || board[j * width + i] / 10 == 4) {
						flag = 1;
					}
				}
			}
		}
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);					// gemini
		termWidth = w.ws_col;									// gemini aided
		termHeight = w.ws_row;									// gemini aided	
		if (flag == 0) {
			blockPrintMutex.lock();
			printBoard(board, 0, width, height, gameMode);
			win2 = 0;
			consoleMutex.lock();								// gemini aided
			cout << "\e[H";
			cout << "\e[" << termHeight / 2 << "B";
			cout << "\e[" << termWidth / 2 - 4 <<"C";
			cout << " YOU WIN!\r" << endl;
			cout << "\e[B";
			cout << "\r\e[" << termWidth / 2 - 22 << "C";
			cout << " Click titlebar for new game or ^C to exit!";
			blockOutput = 1;
			consoleMutex.unlock();								// gemini aided
		}
		else if (win == 1) {
			blockPrintMutex.lock();
			printBoard(board, 1, width, height, gameMode);
			win2 = 1;
			consoleMutex.lock();								// gemini aided
			cout << "\e[H";
			cout << "\e[" << termHeight / 2 <<"B";
			cout << "\e[" << termWidth / 2 - 12 <<"C";
			cout << " YOU LOSE!!!!!!!!!!!!!!!!\r" << endl;
			cout << "\e[B";
			cout << "\r\e[" << termWidth / 2 - 22 << "C";
			cout << " Click titlebar for new game or ^C to exit!";
			blockOutput = 1;
			consoleMutex.unlock(); // gemini
		}
	} while (true);
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
	struct winsize w;											// gemini
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);						// gemini
	termWidth = w.ws_col;									 	// gemini aided
	termHeight = w.ws_row;										// gemini aided	
	consoleMutex.lock(); // gemini
	cout << "\e[H";
	cout << "\e[" << ((termHeight - 1) / 2) - (height / 2) - 1 << "B";		// move to the center
	cout << "\e[" << (termWidth / 2) - width << "C";						// move to the center, again, an emoji takes up 2 spaces!
	for (short i = 0; i < width; i++) {
		cout << "🟩";
	}
	cout << "\r\n";
	for (short i = 0; i < height; i++) {
		cout << "\e[" << (termWidth / 2) - width - 2 << "C";				// move to the center, again, an emoji takes up 2 spaces!
		for (short j = 0; j < width; j++) {
			// cout << board[i * width + j] << " ";
			if (j == 0) {
				cout << "🟩";
			}
			if (board[i * width + j] == 10 || ((devBit != 1 && lose != 1) && board[i * width + j] < 100 && board[i * width + j] % 10 == 1)) {
				cout << "⬜";
			}
			else if (
					devBit == 1 || (board[i * width + j] / 10 >= 5 && board[i * width + j] / 10 <= 7 && lose == 1) &&
					board[i * width + j] < 100 && board[i * width + j] % 10 == 1
					) {
				if (gameMode == 0) {
					cout << "🟥";
				}
				else {
					cout << "\e[38;5;255m";
					if (board[i * width + j] / 10 == 5) {
						cout << "\e[48;5;196m";
					}
					else if (board[i * width + j] / 10 == 6) {
						cout << "\e[48;5;160m";
					}
					else if (board[i * width + j] / 10 == 7) {
						cout << "\e[48;5;88m";
					}
					else {
						cout << "\e[48;5;0m";
					}
					cout << board[i * width + j] / 10 - 4 << " " << "\e[0;0m";
				}
				if (showInfoBit == 1) {
					cout << board[i * width + j];
				}
			}
			else if (board[i * width + j] == 0) {
				cout << "  ";
			}
			else if (board[i * width + j] % 10 >= 2 && board[i * width + j] < 100 && board[i * width + j] % 10 <= 4 && board[i * width + j] / 10 != 8) {
				/*
				 * basically all of the (non clicked / hovered) flags
				 */
				if (gameMode == 0) {
					if (lose != 1) {
						cout << "\e[48;5;255m";
						cout << "🚩";
					}
					else {
						cout << "\e[48;5;88m";
						cout << "🏳️";
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
					cout << board[i * width + j] % 10 - 1 << "F";
					cout << "\e[0;0m";
					if (showInfoBit == 1) {
						cout << board[i * width + j];
					}
				}
			}
			else if (board[i * width + j] == 20) {
				cout << "🟨";
			}
			else if (board[i * width + j] == 30) {
				cout << "⬛";
			}
			else if (board[i * width + j] / 10 == 8) {
				if (gameMode == 0) {
					cout << "\e[48;5;220m";
					cout << "🚩";
					cout << "\e[0;0m";
				}
				else {
					cout << "\e[48;5;242m";
					cout << board[i * width + j] % 10 - 1 << "F";
					cout << "\e[0;0m";
				}
			}
			else if (board[i * width + j] == 50) {
				cout << "🟧";
			}
			else {
				if ((board[i * width + j] > 100 && board[i * width + j] <= 200)) {
					board[i * width + j] -= 100;
					cout << "\e[3";							// foreground
				}
				else if (board[i * width + j] > 200) {
					board[i * width + j] -= 200;
					if (board[i * width + j] >= 10) {
						cout << "\e[38;5;232m";					// white on white is unreadable, but for some reason setting the color to 16m (pitch black) doesn't work
					}
					cout << "\e[4";							// background
				}
				switch (board[i * width + j]) {				// prshort with colors, filled in colors for selected (background)
					case 1:	
						cout << "8;5;4m";					// light blue
						break;
					case 2:
						cout << "8;5;2m";					// green
						break;
					case 3:
						cout << "8;5;196m";				// red
						break;
					case 4:
						cout << "8;5;21m";				// blue, deep blue on the purplish side
						break;
					case 5:
						cout << "8;5;88m";				// maroon red
						break;
					case 6:
						cout << "8;5;6m";					// light blue
						break;
					case 7:
						cout << "8;5;240m";				// black			
						break;
					case 8:
						cout << "8;5;245m";				// gray
						break;
					default:
						cout << "8;5;15m";				// white
						break;
				}
				cout << board[i * width + j];
				if (board[i * width + j] < 10) {
					cout << " ";
				}
				cout << "\e[0;0m";
				board[i * width + j] += 100;
//				cout << board[i * width + j] << " ";
			}
		}
		cout << "🟩";
		cout << "\r" << endl;
	}
	cout << "\e[" << (termWidth / 2) - width << "C"; // move to the center, again, an emoji takes up 2 spaces!
	for (short i = 0; i < width; i++) {
		cout << "🟩";
	}
	cout << "\e[H";
	cout << "\e[" << ((termHeight - 1) / 2) - (height / 2) << "A"; // move to the enter
	consoleMutex.unlock();
}

short userInput(short* x, short* y, short board[], short lose, short openSettings, short *width, short *height, short *mineCount, short *gameMode, short *flagPlaced) {
	char flag;
	short valBak;
	short *cellPos;
	short validChord;
	char input;
	short tempVal;
	short mouseValx;
	short mouseValy;
	short pressed = 1;
	short logicTemp;
	short termWidth;
	short termHeight;
	short printId;
	struct winsize w;												// disclosing the same lines as gemini is very redundant
	while (true) {
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);						// just know that the code that gets the terminal size is not done by me
		// The ioctl call returns 0 on success, -1 on error			// but if i need to get the terminal size in a future project
		termWidth = w.ws_col;										// i won't have to use gemini because i learned how to do so
		termHeight = w.ws_row;
		if (lose == 0) {
			valBak = board[*y * *width + *x];
			cellPos = &board[*y * *width + *x];
			if (valBak > 100) {
				*cellPos += 100;
			}
			else if (valBak % 10 >= 2 && valBak % 10 <= 4) {
				*cellPos = 80 + (valBak % 10);
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
					 * M if mouse NOT pressed down
					 * m if mouse IS pressed down
					 */
					tempVal = getMouseVal(&pressed);
					if (tempVal == 35 || tempVal == 51) {	// unpressed || unpressed + ctrl
						mouseValx = getMouseVal(&pressed);
						mouseValx--;
						mouseValy = getMouseVal(&pressed);
						mouseValy--; // for some reason the menu settings does not want to play ball unless i do this jankery
						if (openSettings == 1) {
							printId = 0;
							if (mouseValy - (termHeight / 2 - 15 / 2) == 2)  {
								if (mouseValx - (termWidth / 2 - 36 / 2) >= 7 && mouseValx - (termWidth / 2 - 36 / 2) < 10) {
									printId = 1;
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 16 && mouseValx - (termWidth / 2 - 36 / 2) < 19) {
									printId = 2;
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 25 && mouseValx - (termWidth / 2 - 36 / 2) < 28) {
									printId = 3;
								}
							}
							else if (mouseValy - (termHeight / 2 - 15 / 2) == 10) {
								if (mouseValx - (termWidth / 2 - 36 / 2) >= 7 && mouseValx - (termWidth / 2 - 36 / 2) < 10) {
									printId = 7;
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 16 && mouseValx - (termWidth / 2 - 36 / 2) < 19) {
									printId = 8;
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 25 && mouseValx - (termWidth / 2 - 36 / 2) < 28) {
									printId = 9;
								}
							}
							else if (mouseValy - (termHeight / 2 - 15 / 2) == 13) {
								if (mouseValx - (termWidth / 2 - 36 / 2) >= 7 && mouseValx - (termWidth / 2 - 36 / 2) < 10) {
									printId = 21;
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 25 && mouseValx - (termWidth / 2 - 36 / 2) < 28) {
									printId = 22;
								}
							}
							printSettingsMenu(printId, width, height, mineCount, gameMode);
						}
						mouseValy++; // i mean i guess it does the trick
						if (termHeight % 2 != 0) {
							mouseValy--;
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

						pressed = 1;
					}
					else if (tempVal == 0 || tempVal == 16) { // left click || left click + ctrl
						mouseValx = getMouseVal(&pressed);
						mouseValx--;
						mouseValy = getMouseVal(&pressed);
						if ((mouseValy == 0 || mouseValy == 1) && pressed == 1) {
							if (mouseValx >= 9) {
								return 5;
							}
							else {
								return 6;
							}
						}
						mouseValy--;
						if (openSettings == 1) {
							printId = 0;
							if (mouseValy - (termHeight / 2 - 15 / 2) == 2)  {
								if (mouseValx - (termWidth / 2 - 36 / 2) >= 7 && mouseValx - (termWidth / 2 - 36 / 2) < 10) {
									if (pressed == 0) {
										printId = 11;
									}
									else {
										if (tempVal == 16) {
											*width += 9;
										}
										*width += 1;
										printId = 1;
									}
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 16 && mouseValx - (termWidth / 2 - 36 / 2) < 19) {
									if (pressed == 0) {
										printId = 12;
									}
									else {
										if (tempVal == 16) {
											*height += 9;
										}
										*height += 1;
										printId = 2;
									}
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 25 && mouseValx - (termWidth / 2 - 36 / 2) < 28) {
									if (pressed == 0) {
										printId = 13;
									}
									else {
										if (tempVal == 16) {
											*mineCount += 9;
										}
										*mineCount += 1;
										printId = 3;
									}
								}
							}
							else if (mouseValy - (termHeight / 2 - 15 / 2) == 10) {
								if (mouseValx - (termWidth / 2 - 36 / 2) >= 7 && mouseValx - (termWidth / 2 - 36 / 2) < 10) {
									if (pressed == 0) {
										printId = 17;
									}
									else {
										if (tempVal == 16) {
											*width -= 9;
										}
											*width -= 1;
										printId = 7;
									}
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 16 && mouseValx - (termWidth / 2 - 36 / 2) < 19) {
									if (pressed == 0) {
										printId = 18;
									}
									else {
										if (tempVal == 16) {
											*height -= 9;
										}
										*height -= 1;
										printId = 8;
									}
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 25 && mouseValx - (termWidth / 2 - 36 / 2) < 28) {
									if (pressed == 0) {
										printId = 19;
									}
									else {
										if (tempVal == 16) {
											*mineCount -= 9;
										}
										*mineCount -= 1;
										printId = 9;
									}
								}
							}
							else if (mouseValy - (termHeight / 2 - 15 / 2) == 13) {
								if (mouseValx - (termWidth / 2 - 36 / 2) >= 7 && mouseValx - (termWidth / 2 - 36 / 2) < 10) {
									if (pressed == 0) {
										printId = 31;
									}
									else {
										if (tempVal == 16) {
											*gameMode -= 9;
										}
										*gameMode -= 1;
										printId = 21;
									}
								}
								else if (mouseValx - (termWidth / 2 - 36 / 2) >= 25 && mouseValx - (termWidth / 2 - 36 / 2) < 28) {
									if (pressed == 0) {
										printId = 32;
									}
									else {
										if (tempVal == 16) {
											*gameMode += 9;
										}
										*gameMode += 1;
										printId = 22;
									}
								}
							}
							printSettingsMenu(printId, width, height, mineCount, gameMode);
						}
						mouseValy++;
						if (termHeight % 2 != 0) {
							mouseValy--;
						}
						mouseValx -= (termWidth / 2) - *width;
						mouseValx /= 2;		// emojis take 2 spaces horizontally
						mouseValy -= termHeight / 2 - *height / 2;
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
							logicTemp = clickLogic(x, y, board, 0, *width, *height, *gameMode, flagPlaced);
							if (logicTemp != 3) {
								return logicTemp;
							}
						}
					}
					else if (tempVal == 2) { // right click
						mouseValx = getMouseVal(&pressed);
						mouseValx--;
						mouseValx -= (termWidth / 2) - *width;
						mouseValx /= 2;		// emojis take 2 spaces horizontally
						mouseValy = getMouseVal(&pressed);
						if (termHeight % 2 != 0) {
							mouseValy--;		// but not vertically! although there is a small offset
						}
						mouseValy -= termHeight / 2 - *height / 2;
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
							logicTemp = clickLogic(x, y, board, 1, *width, *height, *gameMode, flagPlaced);
							if (logicTemp != 0) {
								return logicTemp;
							}
						}
					}
					else if (tempVal == 34 || tempVal == 32) { // pressed down and moving
						mouseValx = getMouseVal(&pressed);
						mouseValx--;
						mouseValx -= (termWidth / 2) - *width;
						mouseValx /= 2;		// emojis take 2 spaces horizontally
						mouseValy = getMouseVal(&pressed);
						if (termHeight % 2 != 0) {
							mouseValy--;		// but not vertically! although there is a small offset
						}
						mouseValy -= termHeight / 2 - *height / 2;
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
					}
				}
			}
		}
		else if (input == 'd' && lose == 0) {
			logicTemp = clickLogic(x, y, board, 0, *width, *height, *gameMode, flagPlaced);
			if (logicTemp != 3) {
				return logicTemp;
			}
		}
		else if (input == 'f' && !(board[*y * *width + *x] >= 100 && board[*y * *width + *x] <= 200) && lose == 0) {
			logicTemp = clickLogic(x, y, board, 1, *width, *height, *gameMode, flagPlaced);
			if (logicTemp != 0) {
				return logicTemp;
			}
		}
	}
	return 0;
}

short calcAdjacent(short x, short y, short board[], short mode, short width, short height) {
	short count = 0, tempCount;
	short* temp;
	for (short i = -1; i < 2; i++) {
		for (short j = -1; j < 2; j++) {
			if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) { // check for out of bounds
				temp = &board[(y + j) * width + (x + i)];
				if (*temp < 100) {		// to avoid checking numbers
					if (mode == 0 && *temp / 10 >= 5 && *temp / 10 <= 7) {// this is to check the amount of mines nearby (to place a number)
						tempCount = (*temp / 10 - 4);
						count += tempCount;
					}
					else if (mode == 1 && *temp == 0) {								// this is to check if there's a nearby blank square, used by expandBoard
						return 1; // i know, i know, jacopini... va bene che compiler optimization tolgono le altre condizioni ma questo è più semplice da leggere
					}
					else if (mode == 2 && *temp % 10 >= 2 && *temp % 10 <= 5) {		// this is used to check if there's the right amount of flags where you're chording
						count += *temp % 10 - 1;
					}
					else if (mode == 3 && *temp % 10 == 1) {							// if you're chording and there's an unflagged mine nearby then you lose
						count++;
					}
				}
			}
		}
	}
	return count;
}

void expandBoard(short x, short y, short board[], short width, short height, short *flagPlaced) {	
	short flag, count = 1, adjacent, temp;
	short* temp2;
	char asdf;
	do {
		flag = 0;
		for (short i = -count; i <= count; i++) {
			for (short j = -count; j <= count; j++) {
				if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) { // check for out of bounds blah blah
					temp2 = &board[(y + j) * width + (x + i)];
					adjacent = calcAdjacent(x + i, y + j, board, 1, width, height);
					if (adjacent == 1) {
						if (*temp2 == 10 || *temp2 / 10 == 4) {				// if you flag a white spot, that flagged spot is overridden when board expansion
							flag = 1;
						}
						if (*temp2 < 100 && !(*temp2 / 10 >= 5 && *temp2 / 10 <= 7)) {
							temp = calcAdjacent(x + i, y + j, board, 0, width, height);
							if (temp != 0) {
								temp += 100;
							}
							if (board[(y + j) * width + (x + i)] / 10 == 4) {
								*flagPlaced -= *temp2 % 10 - 1;
							}
							*temp2 = temp;
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

short clickLogic(short* x, short* y, short board[], short flag, short width, short height, short gameMode, short *flagPlaced) {
	short validChord;
	short *temp2;
	temp2 = &board[*y * width + *x];
	if (flag == 0) {														// are you trying to place a flag? flag = 0 -> no (either that is chording or normal clicking), flag = 1 -> flagging
		if (*temp2 != 0 && *temp2 < 100) {									// are you trying to chord? if you're trying to chord then temp2 would be > 100
			if (*temp2 % 10 == 1 && *temp2 / 10 >= 5 && *temp2 / 10 <= 7) {			
				return 1;													// whoops, you just clicked on a bomb!
			}
			else if (*temp2 % 10 >= 2 && *temp2 % 10 <= 4) {
				return 3;													// do nothing, you clicked on a flag
			}
			return 0;
		}
		else if (*temp2 > 100 && *temp2 <= 200) {								// chording!
			validChord = calcAdjacent(*x, *y, board, 2, width, height);
			if (validChord != 0) {
				validChord += 100;
			}
			if (validChord == *temp2) {
				validChord = calcAdjacent(*x, *y, board, 3, width, height);
				if (validChord != 0) {
					return 1;			// you chorded near an unflagged mine!
				}
				short temp;
				for (short i = -1; i < 2; i++) {
					for (short j = -1; j < 2; j++) {
						if (*x + i >= 0 && *x + i < width && *y + j >= 0 && *y + j < height) {
							if (board[(*y + j) * width + (*x + i)] == 10) {
								temp = calcAdjacent(*x + i, *y + j, board, 0, width, height);
								if (temp != 0) {
									temp += 100;
								}
								board[(*y + j) * width + (*x + i)] = temp; 	
								expandBoard(*x + i, *y + j, board, width, height, flagPlaced);
							} 
						}
					}
				}
			}
			return 0;
		}
	}
	else if (flag == 1 && *temp2 < 100) {							// flagging
		if (*temp2 / 10 >= 4 && *temp2 / 10 <= 7) {
			*temp2 += 1;
			if (*temp2 % 10 > gameMode + 2) {		// if you try to add 1 flag while you're at the max, that means you're trying to deflag
				if (*temp2 / 10 >= 5 && *temp2 / 10 <= 7) {
					*temp2 /= 10;
					*temp2 *= 10;
					*temp2 += 1;
				}
				else if (*temp2 / 10 == 4) {
					*temp2 = 10;
				}
			}
		}
		else {
			if (*temp2 == 10) {
				*temp2 = 42;
			}
			else {
				return 0;						// just in case
			}
		}
		return 2;								// flagged succesfully
	}
	return 3;									// can't flag
}

void wordArt(short** board, short *width, short *height, short *mineCount, short *gameMode, short *win, short *trueMineCount, short *flagPlaced, short *timer) {
	char word[] = "minesweeper";
	short Art = 0, termWidth, termHeight, flip = 0, oldWidth = 0, oldHeight = 0;
	do {
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
			flushBuffer(*board, width, height, mineCount, gameMode, win);
			back_from_sigtstp = 0;
		}
		consoleMutex.lock();
		cout << "\e[0;10H";	// to not overlap the "settings"
		cout << "\e[48;5;15m";
		cout << "\e[38;5;16m";
		for (short i = 0; i < termWidth - 9; i++) {
			cout << " ";
		}
		cout << "\e[0;" << (termWidth / 2) - (11 / 2)	/* 11 is the length of the word "minesweeper" */ << "H";
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
	} while (true);
}

void flushBuffer(short board[], short *width, short *height, short *mineCount, short *gameMode, short *win) {
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
		printBoard(board, 0, *width, *height, *gameMode);
		blockPrintMutex.unlock();
	}
	else {
		if (*win == 6 || *win == 1 || *win == 0) {
			/*
			 * it is not safe to prshort the board while the menu settings is open: it tries to read out of bounds (if the settings have changed).
			 * the heap region pointed by board hasn't yet been resized
			 * instead of going for an extremely convoluted (and dumb) workaround, i just disabled the printing if the menu pane is open
			 */
			if (*win != 6) {
				printBoard(board, *win, *width, *height, *gameMode);
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
	// menu wide 36 and tall 15
	consoleMutex.lock();
	cout << "\e[H";
	cout << "\e[48;5;15m";
	cout << "\e[38;5;16m";
	cout << " < Back  ";
	cout << "\e[48;5;233m";
	short temp = 0;
	for (short i = 0; i < 15; i++) {
		cout << "\e[" << termHeight / 2 - 15 / 2 + i + temp << ";" << termWidth / 2 - 36 / 2 + 1 << "H";
		if (i == 1) {
			for (short k = 0; k < 6; k++) {
				cout << " ";
			}
			cout << "\e[38;5;16m";
			cout << "\e[48;5;15m";
			cout << "Width";
			cout << "\e[48;5;233m";
			for (short k = 0; k < 3; k++) {
				cout << " ";
			}
			cout << "\e[38;5;16m";
			cout << "\e[48;5;15m";
			cout << "Height";
			cout << "\e[48;5;233m";
			for (short k = 0; k < 2; k++) {
				cout << " ";
			}
			cout << "\e[38;5;16m";
			cout << "\e[48;5;15m";
			cout << "Mine count";
			cout << "\e[48;5;233m";
			for (short k = 0; k < 4; k++) {
				cout << " ";
			}
			temp = 1;
			cout <<  "\e[" << termHeight / 2 - 15 / 2 + i + 1 << ";" << termWidth / 2 - 36 / 2 + 1 << "H";
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
		cout << "\e[" << termHeight / 2 - 15 / 2 + 3 + i * 4 << ";" << termWidth / 2 - 36 / 2 + 2 << "H";
		for (short j = 0; j < 3; j++) {
			if (update != 0) {
				if (update < 10 && update == (j + 3 * i) + 1) {
					cout << "\e[48;5;82m";
				}
				else if (update > 10 && update < 20 && (update - 10) == (j + 3 * i) + 1) {
					cout << "\e[48;5;28m";
				}
			}
			if (*width < 3) {
				*width = 3;
			}
			if (*height < 3) {
				*height = 3;
			}
			if (*width * *height - 1 < *mineCount) {
				*mineCount = *width * *height - 1;
			}
			if (*mineCount < 1) {
				*mineCount = 1;
			}
			if (i == 1) {
				if (j == 0) {			// all of this is way more complex than it should be
					tempNum = *width;
					divCount = 0;
					do {
						tempNum /= 10;
						divCount++;
					} while (tempNum > 0);
					cout << "\e[6C";
					cout << "\e[" << (divCount / 2) - 1 << "D";
					if (divCount > 3) {
						cout << "\e[" << (divCount / 2) - 1 << "D";
					}
					if (divCount % 2 != 0) {
						cout << " ";
					}
					else {
						cout << "\e[C";
					}
					cout << *width;
					cout << " ";
					cout << "\e[" << 6 - (divCount - 1) / 2 << "C";
				}
				else if (j == 1) {
					tempNum = *height;
					divCount = 0;
					do {
						tempNum /= 10;
						divCount++;
					} while (tempNum > 0);
					cout << "\e[" << (divCount / 2) - 1 << "D";
					if (divCount > 3) {
						cout << "\e[" << (divCount / 2) - 1 << "D";
					}
					if (divCount % 2 != 0) {
						cout << " ";
					}
					else {
						cout << "\e[C";
					}
					cout << *height;
					cout << " ";
					cout << "\e[" << 6 - (divCount - 1) / 2 << "C";
				}
				else if (j == 2) {
					tempNum = *mineCount;
					divCount = 0;
					do {
						tempNum /= 10;
						divCount++;
					} while (tempNum > 0);
					cout << "\e[" << (divCount / 2) - 1 << "D";
					if (divCount > 3) {
						cout << "\e[" << (divCount / 2) - 1 << "D";
					}
					if (divCount % 2 != 0) {
						cout << " ";
					}
					else {
						cout << "\e[C";
					}
					cout << *mineCount;
					cout << " ";
					cout << "\e[" << 6 - (divCount - 1) / 2 - 1 << "C";
				}
			}
			else if (i == 0) {
				cout << "\e[6C";
				cout << " + ";
			}
			else if (i == 2) {
				cout << "\e[6C";
				cout << " - ";
			}
			cout << "\e[48;5;15m";
		}
		if (i == 3) {
			cout << "\e[A";
			cout << "\e[6C";
			if (update == 21) {
				cout << "\e[48;5;82m";
			}
			else if (update == 31) {
				cout << "\e[48;5;28m";
			}
			else {
				cout << "\e[38;5;16m";
				cout << "\e[48;5;15m";
			}
			cout << " - ";
			cout << "\e[3C";
			cout << "\e[38;5;16m";
			cout << "\e[48;5;15m";
			if (*gameMode < 0) {
				*gameMode = 0;
			}
			else if (*gameMode > 2) {
				*gameMode = 2;
			}
			cout << " " << *gameMode + 1 << " mine";
			if (*gameMode != 0) {
				cout << "s";
			}
			else {
				cout << " ";
			}
			cout << " ";
			if (update == 22) {
				cout << "\e[48;5;82m";
			}
			else if (update == 32) {
				cout << "\e[48;5;28m";
			}
			else {
				cout << "\e[38;5;16m";
				cout << "\e[48;5;15m";
			}
			cout << "\e[3C";
			cout << " + ";
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
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	signal(SIGINT, SIG_DFL);
	cout << flush;
	raise(SIGINT);
}
