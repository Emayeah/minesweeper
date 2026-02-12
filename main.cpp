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
	int board[width][height];
	initBoard(board);
	displayBoard(board);
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

void displayBoard(int board[height][width]) {
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
