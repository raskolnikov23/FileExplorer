#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

void RefreshScreen();
void Initialization();
void ExitProgram(); 
void ProgramLoop();
void ProcessInput();
void OpenDirectory();
int IsDirectory();
void GetTerminalSize();
void DrawStatusBar();
void ClearScreen();

struct directory {
    int entryCount;
    struct dirent **entries;
    DIR *folder; 
};

struct termios orig_termios;
struct directory dir;