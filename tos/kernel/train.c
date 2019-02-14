/*
Name:    Richa Mirashi
SFSU ID: 917251678
*/


#include <kernel.h>
#define SLEEP_TICKS 3


/* Helper function for string concatenation */
char* str_concat(char* str1, char* str2) {
    int size_1 = k_strlen(str1);
    int size_2 = k_strlen(str2);
    char* concatenated_str = malloc(size_1 + size_2 + 1);
    int i = 0;
    while(*str1){
        concatenated_str[i] = *str1;
        i++;
        str1++;
    }
    while(*str2){
        concatenated_str[i] = *str2;
        i++;
        str2++;
    }
    concatenated_str[i] = '\0';
    return concatenated_str;
}


/* Helper function to return contact string */
char* get_contact(char* command){
    int command_length = k_strlen(command);
    char* contact = malloc(command_length);
    int index; 
    for(index = 0; index < command_length-1; index++){
        contact[index] = command[index+1];
    }

    contact[index] = '\0';
    return contact;
}


/* Sends a message to the com port */
void send_to_com_port(char* command){
    COM_Message msg;

    char* command_with_CR = str_concat(command, "\015");

    msg.output_buffer = command_with_CR;
    msg.input_buffer = NULL;
    msg.len_input_buffer = 0;

    send(com_port, &msg);
}


/* Clears the output memory buffer */
void clear_memory_buffer(){
    char* clear_memory_buffer_command = "R";
    send_to_com_port(clear_memory_buffer_command);
}


/*
    Sets switch to either Red or Green depending on the command 
    R = Red
    G = Green
*/
void set_switch(int window_id, char* switch_command){
    wm_print(window_id, "\nSetting switch %c to %c.\n", *(switch_command+1), *(switch_command+2));
    send_to_com_port(switch_command);
    clear_memory_buffer();
    sleep(SLEEP_TICKS);
}


/* Sets the outer switches and tracks for Zamboni not to run out of track for all configurations */
void set_outer_train_tracks(int window_id){
    wm_print(window_id, "\nSetting up switches and train tracks of outer loop.\n");

    int outer_switches_count = 5;
    char* outer_switches[] = { "M5G", "M8G", "M4G", "M1G", "M9R" };

    for(int switch_index = 0; switch_index < outer_switches_count; switch_index++){
        set_switch(window_id, outer_switches[switch_index]);
    }
    
}


/* Probes the contact and returns 1 if a vehicle is on the contact */
int probe_contact(int window_id, char* probe_command, int timer){
    int probe_counter = 0;
    int probe_status = 0;
    COM_Message msg;
    char buffer[3];

    char* contact = get_contact(probe_command);
    wm_print(window_id, "Probing contact %s: %s\n", contact, probe_command);

    char* command_with_CR = str_concat(probe_command, "\015");
    
    while(probe_counter++ < timer){
        msg.output_buffer = command_with_CR;
        msg.input_buffer = buffer;
        msg.len_input_buffer = 3;

        send(com_port, &msg);

        probe_status = buffer[1] - '0';
        clear_memory_buffer();

        if(probe_status == 1) break;

    }

    return probe_status;

}


/* Detects the configuration */
int detect_configuration(int window_id){
    wm_print(window_id, "\nLocating Train and Wagon.\n");

    int configuration_count = 4;

    char* train_position[] = {"C8", "C12", "C2", "C5"};
    char* wagon_position[] = {"C11", "C2", "C11", "C12"};

    int train_position_status = 0;
    int wagon_position_status = 0;

    for(int configuration = 0; configuration < configuration_count; configuration++){
        train_position_status = probe_contact(window_id, train_position[configuration], 3);

        if(train_position_status == 0) continue;

        if(train_position_status == 1){
            wagon_position_status = probe_contact(window_id, wagon_position[configuration], 3);
            
            if(wagon_position_status == 1){
                char* train_contact = get_contact(train_position[configuration]);
                char* wagon_contact = get_contact(wagon_position[configuration]);
                wm_print(window_id, "\nTrain is located on contact %s.\n", train_contact);
                wm_print(window_id, "Wagon is located on contact %s.\n", wagon_contact);
                return configuration + 1;
            }
        }
    }
    
    return 0;
}


/* Changes train speed to either 0, 4 or 5 */
void change_train_speed(int window_id, char* train_command){
    wm_print(window_id, "\nChanging the speed of the train to %c.\n", *(train_command + 4));
    send_to_com_port(train_command);
    clear_memory_buffer();
}


/* Reverses the train direction */
void change_train_direction(int window_id, char* direction_command){
    wm_print(window_id, "\nReversing the train direction: %s.\n", direction_command);
    send_to_com_port(direction_command);
    clear_memory_buffer();
}


/* Configuration 1 with and without Zamboni */
void configuration_1(int window_id, int zamboni){
    set_switch(window_id, "M7G");
    set_switch(window_id, "M5R");
    set_switch(window_id, "M6R");
    set_switch(window_id, "M2R");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 6.\n");
        probe_contact(window_id, "C6", 40);
    }
    
    change_train_speed(window_id, "L20S5");
    
    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 10.\n");
        probe_contact(window_id, "C10", 40);
    }
    
    set_switch(window_id, "M8R");

    wm_print(window_id, "\nCheck if Train and Wagon are on contact 12.\n");
    probe_contact(window_id, "C12", 50);
    change_train_speed(window_id, "L20S0");
    set_switch(window_id, "M8G");
    change_train_direction(window_id, "L20D");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 6.\n");
        probe_contact(window_id, "C6", 40);
    }
    
    change_train_speed(window_id, "L20S5");
   
    wm_print(window_id, "\nCheck if Train and Wagon are on contact 7.\n");
    probe_contact(window_id, "C7", 20);
    change_train_speed(window_id, "L20S0");
    change_train_direction(window_id, "L20D");
    change_train_speed(window_id, "L20S5");
    
    wm_print(window_id, "\nCheck if Train and Wagon are on contact 8.\n");
    probe_contact(window_id, "C8", 20);
    change_train_speed(window_id, "L20S0");
    wm_print(window_id, "\nConfiguration 1 is completed!");

}


/* Configuration 2 with and without Zamboni */
void configuration_2(int window_id, int zamboni){
    set_switch(window_id, "M7R");
    set_switch(window_id, "M2G");
    set_switch(window_id, "M6G");

    change_train_direction(window_id, "L20D");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 3.\n");
        probe_contact(window_id, "C3", 40);
    }

    set_switch(window_id, "M1R");
    change_train_speed(window_id, "L20S5");
    
    wm_print(window_id, "\nCheck if Train is on contact 1.\n");
    probe_contact(window_id, "C1", 40);
    set_switch(window_id, "M1G");
    
    wm_print(window_id, "\nCheck if Train and Wagon are on contact 6.\n");
    probe_contact(window_id, "C6", 20);
    set_switch(window_id, "M5R");

    wm_print(window_id, "\nCheck if Train and Wagon are on contact 9.\n");
    probe_contact(window_id, "C9", 40);
    set_switch(window_id, "M5G");
    
    wm_print(window_id, "\nCheck if Train and Wagon are on contact 12.\n");
    probe_contact(window_id, "C12", 20);
    change_train_speed(window_id, "L20S0");

    wm_print(window_id, "\nConfiguration 2 is completed!");

}


/* Configuration 3 with and without Zamboni */
void configuration_3(int window_id, int zamboni){
    set_switch(window_id, "M6G");
    set_switch(window_id, "M7R");
    set_switch(window_id, "M2G");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 10.\n");
        probe_contact(window_id, "C10", 40);
    }

    set_switch(window_id, "M5R");
    change_train_speed(window_id, "L20S5");

    wm_print(window_id, "\nCheck if Train is on contact 12.\n");
    probe_contact(window_id, "C12", 40);
    change_train_speed(window_id, "L20S0");
    set_switch(window_id, "M5G");
    change_train_direction(window_id, "L20D");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 14.\n");
        probe_contact(window_id, "C14", 40);
    }
    
    change_train_speed(window_id, "L20S5");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 3.\n");
        probe_contact(window_id, "C3", 40);
    }

    set_switch(window_id, "M1R");

    wm_print(window_id, "\nCheck if Train and Wagon are on contact 2.\n");
    probe_contact(window_id, "C2", 40);
    set_switch(window_id, "M1G");
    change_train_speed(window_id, "L20S0");

    wm_print(window_id, "\nConfiguration 3 is completed!");

}


/* Configuration 4 with and without Zamboni */
void configuration_4(int window_id, int zamboni){
    set_switch(window_id, "M3R");

    change_train_speed(window_id, "L20S5");
    wm_print(window_id, "\nCheck if Train is on contact 6.\n");
    probe_contact(window_id, "C6", 20);
    change_train_speed(window_id, "L20S0");
    change_train_direction(window_id, "L20D");
    change_train_speed(window_id, "L20S5");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 10.\n");
        probe_contact(window_id, "C10", 40);
    }
    
    set_switch(window_id, "M8R");

    wm_print(window_id, "\nCheck if Train is on contact 11.\n");
    probe_contact(window_id, "C11", 40);
    change_train_speed(window_id, "L20S0");
    set_switch(window_id, "M8G");

    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 10.\n");
        probe_contact(window_id, "C10", 40);
    }

    change_train_speed(window_id, "L20S5");
    
    if(zamboni){
        wm_print(window_id, "\nCheck if Zamboni is on contact 4.\n");
        probe_contact(window_id, "C4", 40);
    }
    set_switch(window_id, "M4R");

    wm_print(window_id, "\nCheck if Train and Wagon are on contact 5.\n");
    probe_contact(window_id, "C5", 40);
    set_switch(window_id, "M4G");
    change_train_speed(window_id, "L20S0");

    wm_print(window_id, "\nConfiguration 4 is completed!");

}


/* 
    The Train process:
    1. Sets the outer switches first not to run Zamboni out of track
    2. Detects the configuration (from 1 to 4)
    3. Detects if Zamboni is running on the tracks
    4. Executes the configuration accordingly
*/
void train_process(PROCESS self, PARAM param){
    int window_id = wm_create(10, 3, 60, 20);
    wm_print(window_id, "Train application started.\n");

    set_outer_train_tracks(window_id);

    int configuration = detect_configuration(window_id);
   
    if(configuration > 0 && configuration < 5) wm_print(window_id, "Configuration %d is detected.\n", configuration);
    else wm_print(window_id, "Configuration %d is not detected.\n", configuration);
    
    wm_print(window_id, "\nCheck if Zamboni is running on tracks.\n");
    int zamboni = probe_contact(window_id, "C4", 40);
    if(zamboni) wm_print(window_id, "Zamboni detected!\n");
    else wm_print(window_id, "Zamboni is not detected.\n");

    if(configuration == 1) configuration_1(window_id, zamboni);
    else if(configuration == 2) configuration_2(window_id, zamboni);
    else if(configuration == 3) configuration_3(window_id, zamboni);
    else if(configuration == 4) configuration_4(window_id, zamboni);
    else wm_print(window_id, "Invalid configuration.\n");

    become_zombie();

}


void init_train(){
    create_process(train_process, 5, 42, "Train Process");
    resign();
}
