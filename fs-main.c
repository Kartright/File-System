#include "fs-sim.h"
#include "fs-validate.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

/**
 * @brief Used for specifically parsing the buffer command by getting all characters after
 * the B as one single string.
 * 
 * @param str - The command string
 * @param cmd - The command struct to store the buffer string in
 */
int parse_buff(char *str, Command *cmd) {
    // If there is no characters after the B command, return 1
    if (strlen(str) <= 2) return 1;
    char *buffer_string = str + 2;
    
    // Trim newline
    size_t len = strlen(buffer_string);
    if (len > 0 && buffer_string[len - 1] == '\n') len--;  // ignore newline

    if (len > 1024 || len <= 0) return 1;   // too long or too short

    memcpy(cmd->buff, buffer_string, len);
    return 2;
}

/**
 * @brief Pad a string to a given length (determined by the size of padded_str arg)
 * 
 * @param str - The string to pad
 * @param len - length of the original string 
 * @param padded_str - The padded string to fill with characters of the orginal string (up to len)
 */
void pad_string(char *str, int len, char *padded_str) {
    for (size_t i = 0; i < (size_t)len; i++) {
        char c = str[i];
        if (isupper((unsigned char)c)) {
            c = tolower((unsigned char)c);
        }
        padded_str[i] = c;
    }
}

/**
 * @brief Parse a command string from an input file into a command struct
 * 
 * @param str - The command string
 * @param delim - The C string containing delimiter character(s) 
 * @param cmd - The cmd struct to parse information into
 */
void parse_command(char* str, const char* delim, Command *cmd) {
    size_t maxSize = 2; // Max array size
    size_t currSize = 0; // Current array size
    cmd->argv = malloc(maxSize * sizeof(char *)); // Initialize array
    char* token;
    char *str_cpy = strdup(str);
    token = strtok(str, delim);
    for(size_t i = 0; token != NULL; ++i){
        if (currSize == maxSize) {
            // If array is full double the memory allocated to it
            maxSize = maxSize * 2;
            char **argvNew = realloc(cmd->argv, maxSize * sizeof(char *));
            cmd->argv = argvNew;
        }
        cmd->argv[i] = token;

        // If the command type is B, call unique buffer parsing function
        if (i == 0 && (strcmp(token, "B") == 0)) {
            currSize = parse_buff(str_cpy, cmd);
            break;
        }
        
        token = strtok(NULL, delim);
        currSize++;
    }
    cmd->size = currSize; // Set size of argument array
    if (currSize != 0) cmd->type = cmd->argv[0]; // If arg array has elements, set the first as command 'type'
    free(str_cpy);
    return;
}

/**
 * @brief Run the command stored in the given command struct. 
 * 
 * @param cmd - Instance of the command struct that contains information about command to run
 */
void runCommands(Command *cmd) {
    if (!strcmp(cmd->type, "M")) {
        // MOUNT virtual disk
        // args: char *name
        fs_mount(cmd->argv[1]);
    } else if (!strcmp(cmd->type, "C")) {
        // CREATE a file
        // args: char name[5], int size
        char padded_name[5] = {0};
        pad_string(cmd->argv[1], strlen(cmd->argv[1]), padded_name);
        fs_create(padded_name, atoi(cmd->argv[2]));
    } else if (!strcmp(cmd->type, "D")) {
        // DELETE a file
        // args: char name[5]
        char padded_name[5] = {0};
        pad_string(cmd->argv[1], strlen(cmd->argv[1]), padded_name);
        fs_delete(padded_name);
    } else if (!strcmp(cmd->type, "R")) {
        // READ a file
        // args: char name[5], int block_num
        char padded_name[5] = {0};
        pad_string(cmd->argv[1], strlen(cmd->argv[1]), padded_name);
        fs_read(padded_name, atoi(cmd->argv[2]));
    } else if (!strcmp(cmd->type, "W")) {
        // WRITE to a file
        // args: char name[5], int block_num
        char padded_name[5] = {0};
        pad_string(cmd->argv[1], strlen(cmd->argv[1]), padded_name);
        fs_write(padded_name, atoi(cmd->argv[2]));
    } else if (!strcmp(cmd->type, "B")) {
        // update the BUFFER
        // args: uint8_t buff[1024]
        fs_buff(cmd->buff);
    } else if (!strcmp(cmd->type, "L")) {
        // LISTS files and directories in cwd
        fs_ls();
    } else if (!strcmp(cmd->type, "O")) {
        // DEFRAGMENT the disk
        fs_defrag();
    } else if (!strcmp(cmd->type, "Y")) {
        // CHANGE the cwd
        // args: char name[5]
        char padded_name[5] = {0};
        pad_string(cmd->argv[1], strlen(cmd->argv[1]), padded_name);
        fs_cd(padded_name);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    for (size_t i=0; i < 1024; i++) fs_buffer[i] = 0;
    char *input_file = argv[1]; // Name of the input file
    FILE *fd = fopen(input_file, "r"); // Initialize the input file descriptor

    // Open the input file
    if (fd == NULL) {
        return 1;
    }
    
    // For each line in the input file, parse it and run the command if valid
    char* line = NULL; // Command line
    size_t lineNum = 0; // Keeps track of the current line #
    size_t size = 0; // Size of command line
    while (getline(&line, &size, fd) != -1) {
        lineNum++;

        // Create command struct and initalize values
        Command *cmd = malloc(sizeof(Command));
        cmd->input_file = input_file;
        cmd->line_num = lineNum;
        cmd->type = NULL;
        cmd->argv = NULL;
        for (size_t i=0; i < 1024; i++) cmd->buff[i] = 0; // Zero out the buffer
        cmd->size = 0;

        char *line_cpy = strdup(line); // Create copy of command line to parse
        parse_command(line, " \n\"", cmd);
        
        // If the command is valid, run it. Otherwise print error.
        if(validateCommand(cmd)) {
            if ((vd == -1) && (strcmp(cmd->type, "M") != 0)) fprintf(stderr, "Error: No file system is mounted\n");
            else runCommands(cmd);
        } else {
            fprintf(stderr, "Command Error: %s, %ld\n", cmd->input_file, cmd->line_num);
        }

        // Free allocated memory
        free(cmd->argv);
        free(cmd);
        free(line_cpy);
    }
    free(line);
    fclose(fd);
    if (vd != -1) close(vd);
    if (sb != NULL) free(sb);
    if (disk_name != NULL) free(disk_name);
    return 0;
}