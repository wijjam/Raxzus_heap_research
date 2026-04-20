#ifndef RAXZUS_SHELL_H
#define RAXZUS_SHELL_H




int checkString(char* input, char* compareString);
void process_input(char* input);
void cmd_help(void);
void cmd_clear(void);
void cmd_neofetch(void);
void cmd_memtest(void);

typedef struct {
    const char* name;
    void (*function)()
} command;

#endif