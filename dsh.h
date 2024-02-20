#define MAXBUF 256 // max number of characteres allowed on command line

// TODO: Any global variables go below
// int thisIsGlobal = 10; // delete before submission

// TODO: Function declarations go below
char **split(char *str, char *delim);
void configureMode(char **tokens);
void prompt();
int arraySize(char **array);