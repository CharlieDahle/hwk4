#define MAXBUF 256 // max number of characteres allowed on command line

char **split(char *str, char *delim);
void configureMode(char **tokens);
void prompt();
int arraySize(char **array);
void mode1(char *firstString, char **tokens, int newSize);
void mode2(char *firstString, char **tokens, int newSize);