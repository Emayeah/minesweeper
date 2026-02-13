#include <termios.h>
#include <unistd.h>
// comments made by me are gonna be marked with an !
struct termios original_settings;

void enable_raw_mode() {
	struct termios raw;
	// 1. Get the current terminal attributes
	tcgetattr(STDIN_FILENO, &original_settings);
	raw = original_settings;

	// 2. Disable canonical mode and echoing
	// ICANON: line-by-line mode
	// ECHO: prints characters as you type
	// ISIG: disables Ctrl-C/Ctrl-Z signals
	raw.c_lflag &= ~(ICANON | IEXTEN | ECHO | ISIG); // ! ~ is a bitwise not operator

	// 3. Disable other input/output processing
	// ! icrnl lowkey useful ('\n' acting as a line feed and not as a carriage return and a line feed)
	// ! idk why inpck needs to be disabled (i mean do i really need it)
	// ! i definitely do not need istrip to be disabled, re-enabled
	// ! eh i guess having ixon disabled is kinda useless, re-enabled it
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK);
	raw.c_oflag &= ~(OPOST);

	// 4. Set read timeout (return after 1 byte, no delay)
	// ! so, basically 0 means that it won't wait for input, hm
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	// 5. Apply the new settings immediately
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void disable_raw_mode() {
	// Restore the terminal to its original state
	tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
}
// ! ok yeah it mostly makes sense, it's stuff that i would have been able to come up with given a bit of time and the proper docs
