#include <iostream>
#include <cstdlib>
#include "prototype.h" // width, height and minecount are stored here!
using namespace std;
/*
 * 0 = uncovered (nothing)
 * 1 - 8 = uncovered (bomb nearby)
 * 9 = mine
 * 10 = covered
 * 11 = ask for confirmation
 * 19 = mine with flag
 * 20 = covered with flag
 */
const int devBit = 0;
const int debugBit = 0;

int main() {
	int firstInput = 1;
	int win, x, y;
	int adjacent;
	int board[width][height];
	initBoard(board);
	int flag;
	do {
		printBoard(board, 0);
		win = userInput(&x, &y, board); // win == 1 that means you lose because it's the game that wins against the player lol
		if (firstInput == 1 && win == 1) { // first input is always safe
			srand(time(NULL));
			int tempx, tempy;
			do {
				tempx = rand() % width;
				tempy = rand() % height;
			} while (board[tempx][tempy] != 9);
			board[tempx][tempy] = 9;
			board[x][y] = 10;
			win = 0;
		}
		firstInput = 0;
		if (win == 0) {
			adjacent = calcAdjacent(x, y, board, 0); // 0 is a mode for calcAdjacent, 0 calcs nearby bombs, 1 calcs for nearby 0s for board expansion
			board[x][y] = adjacent;
			if (adjacent == 0) {
				expandBoard(x, y, board);
			}
		}
		flag = 1;
		if (win == 0) {
			flag = 0;
			for (int i = 0; i < height && flag != 1; i++) {
				for (int j = 0; j < width && flag != 1; j++) {
					if (board[i][j] == 10 || board[i][j] == 20) {
						flag = 1;
					}
				}
			}
		}
		if (flag == 0) {
			flag = 1;
			win = 1;
		}
	} while (win != 1);
	printBoard(board, 1);
	if (flag == 1) {
		cout << "YOU WIN!" << endl;
	}
	else {
		cout << "YOU LOSE!!!!!!!!!!!!!!" << endl;
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
	for (int j = 0; j < width; j++) {
		for (int i = 0; i < height; i++) {
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
			else {
				cout << board[i][j] << " ";
			}
		}
		cout << " -- " << j << endl;
	}
	for (int i = 0; i < width; i++) {
		cout << "| ";
	}
	cout << endl;
	int div = 1, temp, count = 1;
	do {
		div *= 10;
		temp = width / div;
		if (temp != 0) {
			count++;
		}
	} while (temp != 0);
	div /= 10;
	for (int c = 0; c < count; c++) {
		for (int i = 0; i < width; i++) {
			cout << (i / div) % 10 << " "; 
		}
		cout << endl;
		div /= 10;
	}
}

int userInput(int* x, int* y, int board[width][height]) {
	char flag;
	int temp;
	do {
		do {
			cout << "Insert horizontal tile location: ";
			cin >> *x;
			if (*x < 0 || *x > width) {
				cout << "Invalid horizontal coordinate" << endl;
			}
		} while (*x < 0 || *x > width);
		do {
			cout << "Insert vertical tile location: ";
			cin >> *y;
			if (*y < 0 || *y > width) {
				cout << "Invalid vertical coordinate" << endl;
			}
		} while (*y < 0 || *y > height);
		if (board[*x][*y] != 10 && board[*x][*y] != 9 && board[*x][*y] != 20 && board[*x][*y] != 19) {
			cout << "Invalid cell selected" << endl;
		}
	} while (board[*x][*y] != 10 && board[*x][*y] != 9 && board[*x][*y] != 20 && board[*x][*y] != 19);
	temp = board[*x][*y];
	board[*x][*y] = 11;
	printBoard(board, 0);
	if (temp >= 19) {
		cout << "Are you sure you want to deflag? (y, any other for no) ";
		cin >> flag;
		if (flag == 'y' || flag == 'Y') {
			flag = 'f';
		}
	}
	else {
		cout << "Are you sure? (y, f for flag, any other for no) ";
		cin >> flag;
	}
	board[*x][*y] = temp;	
	if (flag >= 97) {
		flag -= 32;
	}
	if (flag == 'F') {
		if (board[*x][*y] >= 19) {
			board[*x][*y] -= 10;
		}
		else {
			board[*x][*y] += 10;
		}
		return 2;
	}
	else if (flag == 'Y' && board[*x][*y] == 9) {
		return 1;
	}
	return 0;
}
int calcAdjacent(int x, int y, int board[width][height], int mode) {
	int count = 0;
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height && (x + i != x || y + j != x)) { // check for out of bounds and skip calculation for the center point
				if (mode == 0 && board[x + i][y + j] == 9) {
					count++;
				}
				else if (mode == 1 && board[x + i][y + j] == 0) {
					return 1; // i know, i know, jacopini... va bene che compiler optimization tolgono le altre condizioni ma questo è più semplice da leggere
				}
			}
		}
	}
	return count;
}
void expandBoard(int x, int y, int board[width][height]) {	
	int flag, count = 1, adjacent;
	char asdf[3];
	do {
		flag = 0;
		for (int i = -count; i <= count; i++) {
			for (int j = -count; j <= count; j++) {
				if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height && board[x + i][y + j] != 9 && board[x + i][y + j] != 19) { // check for out of bounds blah blah
					adjacent = calcAdjacent(x + i, y + j, board, 1);
					if (adjacent == 1) {
						if (board[x + i][y + j] >= 10) {
							flag = 1;
						}
						board[x + i][y + j] = calcAdjacent(x + i, y + j, board, 0);
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

