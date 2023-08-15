#include "main.h"
#include <sys/ioctl.h>



int** selected;
char* selectedPath;
char** paths;
int pathDepth;

int* rows;
int* cols;
int* allowedEntryCount;
int displayedEntryCount;

#define ENTRYPOINT_DIRECTORY "/"

int main() 
{
    TerminalSetup();
    Initialization();

    while (1)
    {
        ProcessInput();
        RefreshScreen();  

        /* selected entry path updater */
        strcpy(selectedPath, currentPath);
        if (pathDepth > 0) selectedPath = strcat(selectedPath, "/");        // divide with slash only if not at basedir
        int sel = *selected[pathDepth];
        strcat(selectedPath, col.entries[sel]->d_name);
    }

    return 0;
}

void RefreshScreen() 
{
    GetTerminalSize();

    /* where to put cursor, so that entries printed at bottom */ 
    if (*allowedEntryCount >= col.entryCount)
        printf("\x1b[%dB\r", *rows - col.entryCount-2);
    else 
        printf("\x1b[%dB\r", *rows - *allowedEntryCount-2);    


    /* MAIN LOOP */
    for (int i = 0; i <= *allowedEntryCount-1; i++) 
    {
        usleep(1000);                                   // for debug
        GetTerminalSize();

        if (i > col.entryCount-1) break;                // if allowed > entry count


        /* if window size has changed */ 
        if (*allowedEntryCount != *rows - 3)
        {
            printf("\x1b[2J");                          // clears screen
            printf("\x1b[H");                           // go to top

            *allowedEntryCount = *rows - 3;

            if (*selected[pathDepth] > *allowedEntryCount-1)
                *selected[pathDepth] = *allowedEntryCount-1;

            return;
        }
        
        printf("\x1b[K\r");                                         // deletes line & returns

        /* HIGHLIGHT entry if selected */ 
        if (i == *selected[pathDepth]) 
        {
            printf("\t\t\x1b[2D\x1b[7m");                                    // tab and starts inverted text
            printf("%s\r", col.entries[i]->d_name);           // prints entry name
            printf("\x1b[m");                                       // normal text
        }
        else printf("\t\t\x1b[2D%s\r", col.entries[i]->d_name);              // print name of file/folder
            
        if (i + 1 < 10) printf("\t\x1b[1C%d\r", i+1); 
        else printf("\t%d\r", i+1); 

        if (IsDirectory(i) == 0) printf("\t\x1b[4DF\r");              // prints F to the left of folders



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
    rows = malloc(2000);
    cols = malloc(2000);
    allowedEntryCount = malloc(2000);
    col.entries = malloc(10000);                        
    selected = malloc(999);
    paths = malloc(10000);
    currentPath = malloc(100);
    selectedPath = malloc(100);

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
                    case 'B': 
                             if (seq[1] == 'A') *selected[pathDepth] -= 1;
                        else if (seq[1] == 'B') *selected[pathDepth] += 1;

                        if (*selected[pathDepth] < 0) 
                            *selected[pathDepth] = 0;
                            // if selected > entrycount >> scroll up;



                        // if allowed > entries, check folder entry count
                        if (*allowedEntryCount > col.entryCount)
                        {
                            if (*selected[pathDepth] >= col.entryCount)
                                *selected[pathDepth] -= 1;
                        }
                        // if allowed = or < entries, check allowed
                        else 
                        {
                            if (*selected[pathDepth] >= *allowedEntryCount)
                                *selected[pathDepth] -= 1;
                        }


                        break;
                            // if selected < entrycount >> scroll down;

                    case 'C':
                        if (IsDirectory(*selected[pathDepth]) == 0) 
                        {
                            pathDepth++;
                            paths[pathDepth] = malloc(100);
                            OpenDirectory(selectedPath);
                            break;
                        }
                        else return;

                    case 'D':
                        if (pathDepth > 0) 
                        {
                            OpenDirectory(paths[pathDepth-1]);
                            pathDepth--;
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
    if (pathDepth == 0) paths[pathDepth] = malloc(100);
    if (col.entryCount) printf("\x1b[2J");              // if previous entries exist

    selected[pathDepth] = malloc(999);
    col.entryCount = 0;
    col.folder = opendir(dir);

    if (col.folder == NULL) 
    {
        perror("Unable to read dir");
        return;
    }



    while ((col.entries[col.entryCount] = readdir(col.folder)) != NULL)                   // copy entries
    {
        if (col.entries[col.entryCount]->d_namlen == NULL) return;
        // if (col.entries[col.entryCount]->d_reclen == 0) return;

        if (col.entries[col.entryCount]->d_name[0] != '.' &&                // filter hidden folders
            col.entries[col.entryCount]->d_type != DT_LNK)                   // filter symbolic links
            col.entryCount++;  
             

             // NEED TO FILTER OUT FOLDERS WITH NOTHING INSIDE

                                     
    }
    // if (col.entryCount == 0) return;

    /* update path */
    strcpy(currentPath, dir);                        
    
    strcpy(paths[pathDepth], currentPath);      // what is the difference?
}

int IsDirectory(int entry) 
{
    char* entryPath;                    
    entryPath = malloc(100);                                   
    strcpy(entryPath, currentPath);
    strcat(entryPath, "/");
    strcat(entryPath, col.entries[entry]->d_name);

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
    printf("\x1b[%dB\r", 1);                    // move down one line
    printf("\x1b[K\r");                         // erase line and go to line start
    printf("\tPATH: \x1b[7m%s", currentPath);   // inverted print
    printf("\x1b[m\r");                         // normal color text
}