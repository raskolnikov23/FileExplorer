#include "main.h"

char** paths;                       // stores paths in order for successful ascension
int** selected;                     // stores selection index of parent directories
int** entryOffset;                  // stores entry offsets for scrolling

char* selectedPath;                 // currently highlighted entry's path
char* entryPath;                    // for storing path of currently processed entry

int pathDepth;                      // current subdirectory level
int rows;                           // terminal window row count
int cols;                           // terminal window col count
int allowedEntryCount;              // how many entries fit on screen
int colBuffer;

#define ENTRYPOINT_DIRECTORY "/"    // starting directory of program


int main() 
{
    Initialization();

    while (1)
    {
        ProcessInput();
        RefreshScreen();  
    }

    return 0;
}

void Initialization() 
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

    // printf("\x1b[2J");             // clear screen
    // printf("\x1b[H");              // go to top
    printf("\x1b[?25l"); // hide cursor
    // fflush(stdout);

    // Memory allocation 
    dir.entries = malloc(1000);                        
    selected = malloc(1000);
    paths = malloc(1000);
    selectedPath = malloc(1000);
    entryPath = malloc(1000);
    entryOffset = malloc(1000);
    pathDepth = 0;
    entryOffset[pathDepth] = malloc(1000);
    *entryOffset[pathDepth] = 0;
    selected[pathDepth] = malloc(1000);
    *selected[pathDepth] = 0;
    GetTerminalSize();
    ClearScreen();
    allowedEntryCount = rows - 3;
    colBuffer = cols;

    paths[pathDepth] = malloc(1000);
    *paths[pathDepth] = '/';
    OpenDirectory(ENTRYPOINT_DIRECTORY);     // Entry point of the Program  
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
                        if (*selected[pathDepth] < *entryOffset[pathDepth]) *entryOffset[pathDepth]-=1;
                        break;

                    case 'B': 
                        if (*selected[pathDepth] < dir.entryCount-1) *selected[pathDepth] += 1;
                        if (*selected[pathDepth] > (allowedEntryCount-1) + *entryOffset[pathDepth]) *entryOffset[pathDepth]+=1;
                        break;

                    case 'C':
                        if (IsDirectory(*selected[pathDepth]) == 0) 
                        {
                            pathDepth+=1;

                            selected[pathDepth] = malloc(1000);
                            *selected[pathDepth] = 0;

                            entryOffset[pathDepth] = malloc(1000);
                            *entryOffset[pathDepth] = 0;    

                            OpenDirectory(selectedPath);
                        }
                        else return;
                        break;

                    case 'D':
                        if (pathDepth > 0) 
                        {
                            OpenDirectory(paths[pathDepth-1]);
                            *selected[pathDepth] = 0;               // reset pointer at previous depth
                            pathDepth-=1;
                            break;
                        }
                }

                 
                // update selected path after every arrow press 
                strcpy(selectedPath, paths[pathDepth]);                                     // reinit with current dir
                if (pathDepth != 0) selectedPath = strcat(selectedPath, "/");               // divide with slash only if not at basedir
                int tempnum = *selected[pathDepth];
                if (tempnum > 10000) tempnum = 0;
                strcat(selectedPath, dir.entries[tempnum]->d_name);            // append name of file
            }
        }

        else if (c == 'q') exit(0);
    }
}

void RefreshScreen() 
{
    GetTerminalSize();

    /* at which row we start printing */ 
    if (allowedEntryCount >= dir.entryCount) printf("\x1b[%dB\r", rows - dir.entryCount-2);
    else printf("\x1b[%dB\r", rows - allowedEntryCount-2); 

    

    /* print & refresh entries on screen */
    for (int i = 0; i <= allowedEntryCount-1; i++) 
    {
        // usleep(10000);      // for debug
        GetTerminalSize();
        
        if (i > dir.entryCount-1) break;   

        /* if selected is out of bounds */
        if (*selected[pathDepth] > (allowedEntryCount-1) + *entryOffset[pathDepth])   
            *selected[pathDepth] = (allowedEntryCount-1) + *entryOffset[pathDepth];             

        /* if window height has changed */
        if (allowedEntryCount != rows - 3)             
        {
            ClearScreen();

            /* if it has become taller */
            if (allowedEntryCount < rows - 3)
            {
                *entryOffset[pathDepth] -= (rows - 3) - allowedEntryCount;    // subtract added rows from entry offset
                if (*entryOffset[pathDepth] < 0) *entryOffset[pathDepth] = 0;   // set it to 0 if has become negative
            }

            allowedEntryCount = rows - 3;   // update displayed entry count
                
            // if all entries fit, set offset to 0   
            if (allowedEntryCount-1 >= dir.entryCount-1) *entryOffset[pathDepth] = 0;

            return;          
        }

        // if terminal width has changed 
        if (colBuffer != cols)
        {
            ClearScreen();
            colBuffer = cols;
            return;
        }
        
        printf("\x1b[K\r");    // deletes line & returns

        // calculate displayedEntry 
        char displayedEntry[200];
        strcpy((char*)&displayedEntry, dir.entries[i + *entryOffset[pathDepth]]->d_name);
        displayedEntry[cols - 20] = '\0';
        
        int one = strlen(dir.entries[i + *entryOffset[pathDepth]]->d_name);
        int two = strlen(displayedEntry);

        if (one > two)
        {
            displayedEntry[cols - 20] = '.';
            displayedEntry[cols - 19] = '.';
            displayedEntry[cols - 18] = '\0';
        }

        /* HIGHLIGHT entry if selected */ 
        if (i + *entryOffset[pathDepth] == *selected[pathDepth]) 
        {
            printf("\t\t\x1b[2D\x1b[7m");                           // tab and starts inverted text
            printf("%s\r", displayedEntry);                 // prints entry name
            printf("\x1b[m");                                       // normal text
        }
        else printf("\t\t\x1b[2D%s\r", displayedEntry);     // print name of file/folder
        
        /* print entry index */
        if (i + 1 + *entryOffset[pathDepth] < 10) printf("\t\x1b[1C%d\r", i+1 + *entryOffset[pathDepth]);               
        else printf("\t%d\r", i+1 + *entryOffset[pathDepth]); 

        if (IsDirectory(i + *entryOffset[pathDepth]) == 0) printf("\t\x1b[4DF\r");            // prints F symbol to folders

        printf("\x1b[1B\r");                                        // move down one line
        fflush(stdout);
    }

    DrawStatusBar();

    printf("\x1b[H");                     // go to top
    allowedEntryCount = rows - 3;
    colBuffer = cols;
    fflush(stdout);
}

void OpenDirectory(char* path) 
{
    paths[pathDepth] = malloc(1000);

    if (paths[pathDepth] == NULL) paths[pathDepth] = malloc(100);           // allocate to depth pointer when descending
    if (selected[pathDepth] == NULL) selected[pathDepth] = malloc(1000);     // allocate to selection pointer when descending

    if ((dir.folder = opendir(path)) == NULL)
    {
        perror("opendir");
        return;
    }

    dir.entryCount = 0;

    while ((dir.entries[dir.entryCount] = readdir(dir.folder)) != NULL)          // copy entries
    {   
        // if (dir.entries[dir.entryCount]->d_name[0] != '.' &&                    // filter hidden folders
        //     dir.entries[dir.entryCount]->d_type != DT_LNK)                      // filter symbolic links
                dir.entryCount+=1;
    }

    if (dir.entryCount == 0) main(); // temporary?

    if (selectedPath == NULL)
    {
        // update selected path 
        strcpy(selectedPath, paths[pathDepth]);                             // reinit with current dir
        if (pathDepth != 0) selectedPath = strcat(selectedPath, "/");       // divide with slash only if not at basedir
        int selectedNum = *selected[pathDepth];                             // number of currently selected entry in the specified depth
        if (selectedNum > 10000) selectedNum = 0;
        strcat(selectedPath, dir.entries[selectedNum]->d_name);             // append name of file
    }

    strcpy(paths[pathDepth], path);
    ClearScreen();            
}

void DrawStatusBar()
{
    // displayed path's length according to terminal width  
    char* displayedPath = malloc(strlen(paths[pathDepth])+1);
    strcpy(displayedPath, paths[pathDepth]);  

    if ((cols - 20) < strlen(paths[pathDepth]))
    {
        displayedPath[cols - 20] = '.';
        displayedPath[cols - 19] = '.';
        displayedPath[cols - 18] = '\0';
    }

    printf("\x1b[%dB\r", 1);                        // move down one line
    printf("\x1b[K\r");                             // erase line and go to line start
    printf("\tPATH: \x1b[7m%s", displayedPath);     // inverted print
    printf("\x1b[m\r");                             // normal color text

    free(displayedPath);
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
    return 1;
}

void GetTerminalSize()
{
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    rows = w.ws_row;
    cols = w.ws_col;
}

void ClearScreen()
{
    printf("\x1b[H\r");      // go to top left

    GetTerminalSize();

    for (int i = 0; i < rows; i++)
    {
        printf("\x1b[K\r");
        printf("\x1b[1B\r");
    }

    printf("\x1b[H\r");      // go to top left
}

void ExitProgram() 
{
    system("reset");
}