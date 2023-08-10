#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>



void RefreshScreen();
void Initialization();
void ExitProgram(); 
void TerminalSetup();
void ProgramLoop();
void ProcessInput();
void OpenDirectory();
int FolderCheck();
int IsFolder();
void GetTerminalSize();

struct termios orig_termios;

struct column {
    int entryCount;
    struct dirent **entries;
    DIR *folder;
};

struct column col;

enum controls {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

char* currentPath = "";
// char* selectedPath;