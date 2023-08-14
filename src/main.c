#include "main.h"
#include <sys/ioctl.h>



int** selected;
char* selectedPath;
char** paths;
int pathCount = 0;

int* rows;
int* cols;
int* allowedEntryCount;
int displayedEntryCount = 0;

int main() 
{
    TerminalSetup();
    Initialization();

    while (1)
    {
        ProcessInput();
        RefreshScreen();  

        selectedPath = malloc(100);
        strcpy(selectedPath, currentPath);
        if (pathCount > 0) selectedPath = strcat(selectedPath, "/");          // divide with slash only if not at base dir
        int sel = *selected[pathCount];
        strcat(selectedPath, col.entries[sel]->d_name);
    }

    return 0;
}

void RefreshScreen() 
{
    GetTerminalSize();

    // where to put cursor, so that entries printed at bottom
    if (*allowedEntryCount >= col.entryCount)
        printf("\x1b[%dB\r", *rows - col.entryCount - 1);
    else 
        printf("\x1b[%dB\r", *rows - *allowedEntryCount-1);    


    for (int i = 0; i <= *allowedEntryCount; i++) 
    {
        if (i > col.entryCount-1) break;            // break exits for loop

        // when window's vertical size changes
        if (*allowedEntryCount != *rows - 1)
        {
            printf("\x1b[2J");                      // clears screen
            printf("\x1b[H");                       // go to top
            GetTerminalSize();
            *allowedEntryCount = *rows - 1;

            return;                                 // ends RefreshScreen function
            // RefreshScreen();                     // this works better(faster) but can't get it to work


        }



        // usleep(100000);                                           // for debug

        
        printf("\x1b[K\r");                                         // deletes line, returns to left side

        if (i == *selected[pathCount]) 
        {
            printf("\t\x1b[7m");                                    // tab and starts inverted text
            printf("%d %s\r", i, col.entries[i]->d_name);           // prints entry name
            printf("\x1b[m");                                       // normal text
        }
        else printf("\t%s\r", col.entries[i]->d_name);              // print name of file/folder
            
        if (FolderCheck(i) == 0) printf("\t\x1b[4DF");              // prints F to the left of folders

        printf("\x1b[1B\r");                                        // move down one line

        fflush(stdout);

    }



    DrawStatusBar();

    printf("\x1b[H");                                               // go to top
    *allowedEntryCount = *rows - 1;
    fflush(stdout);
}

void Initialization() 
{
    rows = malloc(2000);
    cols = malloc(2000);
    allowedEntryCount = malloc(2000);
    col.entries = malloc(10000);                        
    selected = malloc(999);
    paths = malloc(10000);
    currentPath = malloc(100);

    GetTerminalSize();

    *allowedEntryCount = *rows - 1;

    col.entryCount = 0;

    currentPath = "/";
    OpenDirectory("/");                                 // base dir
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
                    case 'B': 
                        if (seq[1] == 'A') *selected[pathCount] -= 1;
                        else *selected[pathCount] += 1;

                        if (*selected[pathCount] < 0) 
                            *selected[pathCount] = 0;
                            // if selected > entrycount >> scroll up;

                        if (*selected[pathCount] >= col.entryCount) // needs displayedentrycount
                            *selected[pathCount] -= 1;
                            break;
                            // if selected < entrycount >> scroll down;
                        

                    case 'C':
                        if (FolderCheck(*selected[pathCount]) == 0) 
                        {
                            pathCount++;
                            OpenDirectory(selectedPath);
                            break;
                        }

                    case 'D':
                        if (pathCount > 0) 
                        {
                            OpenDirectory(paths[pathCount-1]);
                            pathCount--;
                            break;
                        }
                }
            }
        }

        else if (c == 'q') exit(0);
    }
}

void OpenDirectory(char* dir)
{
    // if previous entries exist
    if (col.entryCount)
    {
        printf("\x1b[2J");
    }

    selected[pathCount] = malloc(999);
    col.entryCount = 0;
    col.folder = opendir(dir);

    if (col.folder == NULL) 
    {
        perror("Unable to read dir");
        return;
    }

    currentPath = malloc(100);                       // OTHERWISE SEGM FAULT
    strcpy(currentPath, dir);                        // update path
    paths[pathCount] = malloc(100);
    strcpy(paths[pathCount], currentPath);

    while (col.entries[col.entryCount] = readdir(col.folder))              // copy entries
    {
        if (col.entries[col.entryCount]->d_name[0] != '.') col.entryCount++;    // filter hidden folders

    }
}

int FolderCheck(int entry)
{
    char* entryPath;                    // maybe there is a better way to find path of entry
    entryPath = malloc(100);            // how much should be allocated?                          
    strcpy(entryPath, currentPath);
    strcat(entryPath, "/");
    strcat(entryPath, col.entries[entry]->d_name);
    
    if (IsFolder(entryPath)) return 0;
    else return 1;
}

int IsFolder(const char* path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
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
        printf("\x1b[%dB\r", 1);                                    // move down one line

    printf("\x1b[K\r");  // erase line and go to line start
    printf("\t\x1b[7m"); // inverted line


    // printf("\t\t\t\t\tcurrent path: %s\r", currentPath);
    // printf("\t\t\t\t\t\t\t\tselected path: %s\r", selectedPath);
    // printf("\t\t\t\t\t\t\t\t\t\t\t\tpath count: %d\r", pathCount);

    printf("\tallowedentrycount: %d", *allowedEntryCount);
    printf("\t\t\trows: %d", *rows);


    printf("\x1b[m\r"); // normal color text
}