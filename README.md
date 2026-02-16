# minesweeper assignment

### TUI minesweeper with mouse support!

prof guardi ./relazione.txt

**No support for non unix like OSes due to the reliance on posix functions!!!**

Features:
1. keyboard driven controls (arrow keys, F for flag, any other for clicking, PG-UP and PG-DOWN for 5 steps vertically, HOME and END for 5 steps horizontally)
2. mouse driven controls (you play exactly how you would play a normal game of minesweeper)
3. support for chording (just a left click, you don't have to press both (pressing both may result in undefined behavior), i'll add support for it maybe)
4. emojis as a display due to them being virtually supported by every single terminal emulator
5. all in a TUI!
6. handling of ^C and ^Z
7. extremely minimal use of ai (gemini), used only in: getting the terminal size, doing async threads, enabling non canonical (raw) mode and mutex (to avoid async race conditions). the use of ai is properly disclosed with comments and all the rest is done by me! gemini has never seen a line of code as input
