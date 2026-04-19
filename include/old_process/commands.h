#ifndef COMMANDS_H
#define COMMANDS_H

#define CALC_HASH 22.203122
#define HELP_HASH 27.243135
#define NEOFETCH_HASH 33.24342524392227
#define CLEAR_HASH 22.31242037
#define MEM_HASH 32.2432
#define TIME_HASH 39.283224
#define DEBUG_HASH 23.24214026
#define FRACTURE_HASH 25.372022
#define QUIT_HASH 36.402839





void init_shell(void);
char* get_first_word(char* input); 
char* get_remaining_words(char* input); 
double calculate_command_hash(char* command);
void init_char_table(void);
void process_input(char* command);
void cmd_help(void);
void cmd_clear(void);
void cmd_neofetch(void);  // Your OS logo + info!

#endif