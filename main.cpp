#include <iostream>
#include <cstdlib>
#include "prototype.h" // width, height and minecount are stored here!
using namespace std;
/*
 * 0 = uncovered (nothing)
 * 1 - 8 = uncovered (bomb nearby)
 * 9 = mine
 * 10 = covered
 * 20 = covered with flag
 */
int main() {
	int win, x, y;
	do {
		int board[width][height];
		initBoard(board);
		displayBoard(board);
		win = userInput(&x, &y, board);
		if (win != 0) {
			expandBoard(x, y, board);
		}
	} while (win != 0);
}

void initBoard(int board[width][height]) {
	int x, y;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			board[i][j] = 10;
		}
	}
	for (int i = 0; i < mineCount; i++) { // this assumes that mineCount < height * width
		srand(time(NULL));
		x = rand() % width;
		srand(time(NULL));
		y = rand() % height;
		while (board[x][y] == 9) {
			x++;
			if (x >= width) { // avoid overflow in extremely unlucky scenarios
				x %= width;
				y++;
				if (y >= height) {
					y %= height;
				}
			}
		}
		board[x][y] = 9;
	}
}

void displayBoard(int board[width][height]) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			cout << "|";
			if (board[i][j] == 10 || board[i][j] == 9) {
				cout << "█"; // black box
			}
			else if (board[i][j] == 0) {
				cout << " ";
			}
			else if (board[i][j] == 20) { // weird but flag looking character
				cout << "╕";
			}
			else {
				cout << board[i][j];
			}
		}
		cout << "|" << endl;
	}
}

int userInput(int* x, int* y, int board[width][height]) {
	do {
		do {
			cout << "Insert horizontal tile location: ";
			cin >> *x;
		} while (*x < 0 || *x > width);
		do {
			cout << "Insert vertical tile location: ";
			cin >> *y;
		} while (*y < 0 || *y > height);
	} while (board[*x][*y] != 10 && board[*x][*y] != 9);
	if (board[*x][*y] == 9) {
		return 1;
	}
	return 0;
}

void expandBoard(int x, int y, int board[width][height]) {
	int count = 0;
	for (int i = -1; i < 1; i++) {
		for (int j = -1; j < 1; j++) {
			if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height && (x != 0 && y != 0)) { // check for out of bounds and skip calculation for the center point
				if (board[x + i][y + j] == 9) {
					count++;
				}
			}
		}
	}
	board[x][y] = count;
	if (count == 0) {
		// TODO
	}
}
