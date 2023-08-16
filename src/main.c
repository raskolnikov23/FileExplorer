#include "main.h"

char** paths;                   // stores paths in order for successful ascension
int** selected;                 // stores selection index of parent directories
char* selectedPath;             // currently highlighted entry's path
int pathDepth;                  // current subdirectory level
int* rows;                      // terminal window row count
int* cols;                      // terminal window col count
int* allowedEntryCount;         // how many entries fit on screen
char* entryPath;                // for storing path of currently processed entry
int entryOffset;

#define ENTRYPOINT_DIRECTORY "/"    // starting directory of program

int main() 
{
    TerminalSetup();
    Initialization();

    while (1)
    {
        ProcessInput();
        RefreshScreen();  
    }

    return 0;
}

void RefreshScreen() 
{
    GetTerminalSize();

    /* at which row we start printing */ 
    if (*allowedEntryCount >= dir.entryCount) printf("\x1b[%dB\r", *rows - dir.entryCount-2);
    else printf("\x1b[%dB\r", *rows - *allowedEntryCount-2);    

    /* print & refresh entries on screen */
    for (int i = 0; i <= *allowedEntryCount-1; i++) 
    {
        // usleep(1000);                                   // for debug
        GetTerminalSize();

        if (i > dir.entryCount-1) break;                

        /* if window height has changed */
        if (*allowedEntryCount != *rows - 3)             
        {
            printf("\x1b[2J");                                  // clear screen
            printf("\x1b[H");                                   // go to top left

            *allowedEntryCount = *rows - 3;                     // update displayed entry count

            /* if selected is out of bounds */
            if (*selected[pathDepth] > *allowedEntryCount-1)   
                *selected[pathDepth] = *allowedEntryCount-1;

            return;                                                 // end function
        }
        
        printf("\x1b[K\r");                                         // deletes line & returns

        /* calculate displayedEntry */ 
        char displayedEntry[200];
        strcpy(displayedEntry, dir.entries[i + entryOffset]->d_name);
        displayedEntry[*cols - 20] = '\0';

        int one = strlen(dir.entries[i + entryOffset]->d_name);
        int two = strlen(&displayedEntry);

        if (one > two)
        {
            displayedEntry[*cols - 20] = '.';
            displayedEntry[*cols - 19] = '.';
            displayedEntry[*cols - 18] = '\0';
        }

        /* HIGHLIGHT entry if selected */ 
        if (i + entryOffset == *selected[pathDepth]) 
        {
            printf("\t\t\x1b[2D\x1b[7m");                           // tab and starts inverted text
            printf("%s\r", displayedEntry);                 // prints entry name
            printf("\x1b[m");                                       // normal text
        }
        else printf("\t\t\x1b[2D%s\r", displayedEntry);     // print name of file/folder
        
        /* print entry index */
        if (i + 1 < 10) printf("\t\x1b[1C%d\r", i+1);               
        else printf("\t%d\r", i+1); 

        if (IsDirectory(i) == 0) printf("\t\x1b[4DF\r");            // prints F symbol to folders

        printf("\x1b[1B\r");                                        // move down one line
    }

    DrawStatusBar();

    printf("\x1b[H");                                               // go to top
    *allowedEntryCount = *rows - 3;
    fflush(stdout);
}

void Initialization() 
{
    /* Memory allocation */
    rows = malloc(1000);
    cols = malloc(1000);
    allowedEntryCount = malloc(1000);
    dir.entries = malloc(1000);                        
    selected = malloc(1000);
    paths = malloc(1000);
    selectedPath = malloc(1000);
    entryPath = malloc(1000);
    pathDepth = 0;
    

    GetTerminalSize();

    *allowedEntryCount = *rows - 3;

    OpenDirectory(ENTRYPOINT_DIRECTORY);     // Entry point of the Program
}

void ExitProgram() 
{
    // tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    system("reset");
}

void TerminalSetup() 
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(ExitProgram);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;    
    raw.c_cc[VTIME] = 1; 

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    //clearscreen
    system("clear");

    // printf("\x1b[?25l");                                // hide cursor 

}

void ProcessInput() 
{
    int nread;
    char c;

    if ((nread = read(STDIN_FILENO, &c, 1)) == 1) 
    {
        if (c == '\x1b') 
        {
            char seq[3];

            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);

            if (seq[0] == '[') 
            {
                switch (seq[1])
                {
                    case 'A': 
                        if (*selected[pathDepth] > 0) *selected[pathDepth] -= 1;
                        if (*selected[pathDepth] < entryOffset) entryOffset--;
                        break;

                    case 'B': 
                        if (*selected[pathDepth] < dir.entryCount-1) *selected[pathDepth] += 1;
                        if (*selected[pathDepth] > (*allowedEntryCount-1)+entryOffset) entryOffset++;
                        break;

                    case 'C':
                        if (IsDirectory(*selected[pathDepth]) == 0) 
                        {
                            pathDepth++;
                            OpenDirectory(selectedPath);
                        }
                        else return;
                        break;

                    case 'D':
                        if (pathDepth > 0) 
                        {
                            OpenDirectory(paths[pathDepth-1]);
                            *selected[pathDepth] = 0;               // reset pointer at this depth
                            pathDepth--;
                            break;
                        }
                }

                
                /* update selected path after every arrow press */
                strcpy(selectedPath, paths[pathDepth]);                             // reinit with current dir
                if (pathDepth != 0) selectedPath = strcat(selectedPath, "/");       // divide with slash only if not at basedir
                int selectedNum = *selected[pathDepth];                             // number of currently selected entry in the specified depth
                strcat(selectedPath, dir.entries[selectedNum]->d_name);             // append name of file
            }
        }

        else if (c == 'q') exit(0);
    }
}

void OpenDirectory(char* path)
{
    if (paths[pathDepth] == NULL) paths[pathDepth] = malloc(100);           // allocate to depth pointer when descending
    if (selected[pathDepth] == NULL) selected[pathDepth] = malloc(100);     // allocate to selection pointer when descending

    if (dir.entryCount) printf("\x1b[2J");                                  // clear screen if previous entries exist
    dir.entryCount = 0;
    entryOffset = 0;

    dir.folder = opendir(path);
    if (dir.folder == NULL) 
    {
        perror("Unable to read path");
        return;
    }

    while ((dir.entries[dir.entryCount] = readdir(dir.folder)) != NULL)       // copy entries
    {
        if (dir.entries[dir.entryCount]->d_name[0] != '.' &&                // filter hidden folders
            dir.entries[dir.entryCount]->d_type != DT_LNK)                   // filter symbolic links
            dir.entryCount++;                         
    }

    /* update current path */                
    strcpy(paths[pathDepth], path);
}

int IsDirectory(int entry) 
{                    
    strcpy(entryPath, paths[pathDepth]);
    if (pathDepth > 0) strcat(entryPath, "/");
    strcat(entryPath, dir.entries[entry]->d_name);

    struct stat path_stat;
    if (stat(entryPath, &path_stat) == 0)
        if S_ISDIR(path_stat.st_mode)
            return 0;
}

void GetTerminalSize()
{
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    *rows = w.ws_row;
    *cols = w.ws_col;
}

void DrawStatusBar()
{
    /* displayed path's length according to terminal width */ 
    char displayedPath[200];  
    strcpy(displayedPath, paths[pathDepth]);
    displayedPath[*cols - 20] = '\0';       

    int one = strlen(paths[pathDepth]);
    int two = strlen(&displayedPath);

    if (one > two)
    {
        displayedPath[*cols - 20] = '.';
        displayedPath[*cols - 19] = '.';
        displayedPath[*cols - 18] = '\0';
    }

    printf("\x1b[%dB\r", 1);                        // move down one line
    printf("\x1b[K\r");                             // erase line and go to line start
    printf("\tPATH: \x1b[7m%s", displayedPath);     // inverted print
    printf("\x1b[m\r");                             // normal color text
}