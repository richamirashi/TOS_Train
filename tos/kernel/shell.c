/*
Name:    Richa Mirashi
SFSU ID: 917251678
*/


#include <kernel.h>

#define MAX_HISTORY_LENGTH 20
#define MAX_COMMAND_BUFFER_LENGTH 33

char shell_symbol[7] = "shell>>";


void execute_command(int window_id, char* shell_command, char** commands_history, int command_history_count);


/* Compares two strings and returns 0 if they are equal */
int str_compare(char* str1, char* str2){
    if(str1 == NULL && str2 == NULL)
        return 0;
    int index = 0;
    while(str1[index] == str2[index]){
        if(str1[index] == '\0' || str2[index] == '\0')
        return 0;
        index++;
    }
    return str1[index] - str2[index];
}


/* Checks if given string starts with querried characters */
int starts_with(char* string, char* query){
    if(string == NULL && query == NULL)
        return 0;
    int index = -1;
    while(query[++index] != '\0') { 
        if(string[index] != query[index]) return 0;
    }
    return 1; // return if true
}


/* Copies one string to another */
char* str_cpy(char *to, const char *from){
    int index;
    for(index = 0; from[index] != '\0'; ++index){
        to[index] = from[index];
    }
    to[index] = '\0';
    return to;
}


/* Helper function for removing the leading spaces */
char* trim_leading_ptr(char* str) {
    if(*str == 0) {
        return str;
    }
    if(*str == ' ' || *str == '\t') {
        return trim_leading_ptr(str + 1);
    }
    return str;
}


/* Helper function for removing the trailing spaces */
char* trim_trailing_ptr(char* str) {
    if (*str == '\0') {
        return str;
    }
    char* t_ptr = trim_trailing_ptr(str + 1);

    if (*(t_ptr - 1) == ' ' || *(t_ptr - 1) == '\t') {
        return t_ptr - 1;
    }
    return t_ptr;
}


/* trim leading and trailing whitespaces of a command buffer */
char* trim_command(char* command_buffer){
    char* L_Ptr = trim_leading_ptr(command_buffer);
    char* T_Ptr = trim_trailing_ptr(command_buffer);

    int new_size = T_Ptr - L_Ptr;

    char* trimmed_command = malloc(new_size + 1);

    str_cpy(trimmed_command, L_Ptr);

    *(trimmed_command + new_size) = '\0';

    return trimmed_command;
}


/* Helper function for !<number> command.
Returns the command history index */
int extract_command_history_index(char* command) {
    int number = 0;
    if(command[0] != '!' || command[1] == '\0' ) return -1;
    for( int index = 1; command[index] != '\0'; index++) {
        if(command[index] < '0' || command[index] > '9') return -1;
        number = number* 10 + (command[index] - '0');
    }
    return number;
}


/* Prints heading for all the processes */
void shell_print_process_heading(int window_id){
    wm_print(window_id, "State              Active  Prio   Name\n");
    wm_print(window_id, "------------------------------------------------\n");
}


/* Prints details of all the processes */
void shell_print_process_details(int window_id, PROCESS process){
    static const char *state[] = { "READY          ",
        "ZOMBIE         ",
        "SEND_BLOCKED   ",
        "REPLY_BLOCKED  ",
        "RECEIVE_BLOCKED",
        "MESSAGE_BLOCKED",
        "INTR_BLOCKED   "
    };
    if(!process->used){
        wm_print(window_id, "PCB slot unused!\n");
        return;
    }
    /* State */
    wm_print(window_id, state[process->state]);
    /* Check for active_proc */
    if(process == active_proc)
        wm_print(window_id, "       *   ");
    else
        wm_print(window_id, "           ");
    /* Priority */
    wm_print(window_id, "  %2d", process->priority);
    /* Name */
    wm_print(window_id, "    %s\n", process->name);
}


/* Executes recursive commands that are ; seperated */
void execute_recursive_commands(int window_id, char* command, char** commands_history, int command_history_count){
   
    /* Handles ; on first character of command */
    if(command[0] == ';') {
        wm_print(window_id, "Syntax error!");
        return;
    } 

    /* Code to handle ; separated commands */
    int command_start_index = 0;
    int i;
    for(i = 0; command[i] != '\0'; i++) {
        if(command[i] == ';') {
            command[i] = '\0';
            /* do not run empty string commands */
            if(command[command_start_index] != '\0') {
                execute_command(window_id, &command[command_start_index], commands_history, command_history_count);
                wm_print(window_id, "\n");
            }
            command_start_index = i + 1;
        } 
    }

    /* for last command without ;  */
    /* do not run empty string commands */
    if(command[command_start_index] != '\0') {  
        execute_command(window_id, &command[command_start_index], commands_history, command_history_count);
    }

    /* replace '\0' with ';' */
    for(int j=0; j < i; j++) {
        if(command[j] == '\0') command[j] = ';';
    }

}


/* Main Shell commands */


/* Prints a text explaining all supported TOS commands */
void call_help(int window){
    wm_print(window, "help:       Explains all supported TOS commands.\n");
    wm_print(window, "cls:        Clears the screen/window.\n");
    wm_print(window, "shell:      Launches another shell.\n");
    wm_print(window, "pong:       Launches the PONG game.\n");
    wm_print(window, "echo <msg>: Echoes the msg to the console.\n");
    wm_print(window, "ps:         Prints out all the processes to the console.\n");
    wm_print(window, "history:    Prints all the commands that have been typed.\n");
    wm_print(window, "!<number>:  Repeats the command with the given number.\n");
    wm_print(window, "about:      Prints 'about' message to the console.\n");
}


/* Echoes the string that follows the command to the console */
void execute_echo(int window_id, char* cmd){
    int i = 3; // skip first 4 characters of command "echo"
    int flag = 0;
    while(cmd[++i] != '\0') {
        if(cmd[i] == '\'' || cmd[i] == '\"' )  continue;
        if(cmd[i] == ' ' && flag != 1)  continue;
        
        if(cmd[i] != ' ')  flag = 1;
        wm_print(window_id, "%c", cmd[i]);
        if(cmd[i] == ' ')  flag = 0;
    }
}


/* Prints out the process table */
void list_all_processes(int window_id){
    PCB *process = pcb;

    shell_print_process_heading(window_id);
    
    for(int i = 0; i < MAX_PROCS; i++, process++){
        if (!process->used)
            continue;
        shell_print_process_details(window_id, process);
    }
}


/* Prints all the commands that have been typed into the shell */
void show_history(int window_id, char** commands_history, int command_history_count){
    for(int i = 0; i < command_history_count; i++ ){
        wm_print(window_id, "%d. %s\n", i + 1, commands_history[i]);
    }
}


/* Repeats the command with the given number */
void execute_bang(int window_id, char* shell_command, char** commands_history, int command_history_count){
    int number = extract_command_history_index(shell_command);
    char* command;
    if(number <= -1) {
        wm_print(window_id, "%s: command not found", shell_command);
    } else if(number > command_history_count) {
        wm_print(window_id, "%s: No command found for requested index", shell_command);
    } else {
        command = commands_history[number - 1]; 
        wm_print(window_id, "%s\n", command);
        execute_recursive_commands(window_id, command, commands_history, command_history_count);
    }
}


/* All shell commands are executed from this function.
Inside this function, different helper functions are called to process the shell commands.
*/
void execute_command(int window_id, char* shell_command, char** commands_history, int command_history_count){
    /* help command */
    if(str_compare(shell_command, "help") == 0){
        call_help(window_id);
    }

    /* cls command */
    else if(str_compare(shell_command, "cls") == 0){
        wm_clear(window_id);
    }

    /* shell command */
    else if(str_compare(shell_command, "shell") == 0){
        start_shell();
    }

    /* pong command */
    else if(str_compare(shell_command, "pong") == 0){
        start_pong();
    }

    /* echo command */
    else if(starts_with(shell_command, "echo") && (shell_command[4] == ' ' || shell_command[4] == '\0') ){
        execute_echo(window_id, shell_command);
    }

    /* ps command */
    else if(str_compare(shell_command, "ps") == 0){
        list_all_processes(window_id);
    }

    /* history command */
    else if(str_compare(shell_command, "history") == 0){
        show_history(window_id, commands_history, command_history_count);
    }

    /* ! command */
    else if(shell_command[0] == '!'){
        execute_bang(window_id, shell_command, commands_history, command_history_count);
    }

    /* about command */
    else if(str_compare(shell_command, "about") == 0){
        wm_print(window_id, "Hey there! I am Richa Mirashi. The more I C, the less I see.");
    }

    /* train command */
    else if(str_compare(shell_command, "train") == 0){
        init_train();
    }

    /* if command not found */
    else{
        wm_print(window_id, "%s: command not found", shell_command);
    }

}


/* Takes the user input and sends it to the execute_command() function for further processing */
void shell_process(PROCESS process, PARAM param){
    int window_id = wm_create(10, 3, 60, 20);

    wm_print(window_id, "***WELCOME***\n");

    char* command_buffer = malloc(MAX_COMMAND_BUFFER_LENGTH + 1);
    int command_buffer_index = 0;
    
    char* command;
    int command_history_count = 0;
    char* commands_history[MAX_HISTORY_LENGTH];

    while(1){
        wm_print(window_id, "\n%s", shell_symbol);

        while(1){
            char ch = keyb_get_keystroke(window_id, TRUE);

            /* if the character is a backspace character */
            if(ch == 8){
                if(command_buffer_index != 0){
                    wm_print(window_id, "%c", ch);
                    command_buffer_index--;
                }
                continue;
            }


            /* if the character is a newline character */
            else if(ch == 13){
                if(command_buffer_index == 0){
                    break;
                }
                wm_print(window_id, "\n");

                /* not to allow more than max input length characters */
                if(command_buffer_index >= MAX_COMMAND_BUFFER_LENGTH) {
                    wm_print(window_id, "Input command exceeded the maximum command length of %d.", MAX_COMMAND_BUFFER_LENGTH-1);
                    break;
                }
                
                command_buffer[command_buffer_index] = '\0';

                /* trim leading and trailing whitespaces */
                command = trim_command(command_buffer);
        
                /* Avoids addiing !<number> command into history */
                if(command[0] != '!') {
                    commands_history[command_history_count++] = command;
                }

                execute_recursive_commands(window_id, command, commands_history, command_history_count);
                break;
            }


            /* For rest of the characters */
            else{
                wm_print(window_id, "%c", ch);
                if(command_buffer_index < MAX_COMMAND_BUFFER_LENGTH){
                    command_buffer[command_buffer_index++] = ch;
                }
            }
        }
        command_buffer_index = 0;
    }
}


/* Creates the shell process and resigns the boot process */
void start_shell(){
    create_process(shell_process, 6, 42, "Shell Process");
    resign();
}
