#include "fs-validate.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/**
 * @brief Validate the given command based on the command it will execute.
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int validateCommand(Command *cmd) {
    // Return invalid for empty commands
    if (cmd->type == NULL || cmd->size == 0) return 0;
    
    // Run the commands respective validate function
    if (!strcmp(cmd->type, "M")) {
        // MOUNT virtual disk
        return fs_mount_valid(cmd);
    } else if (!strcmp(cmd->type, "C")) {
        // CREATE a file
        return fs_create_valid(cmd);
    } else if (!strcmp(cmd->type, "D")) {
        // DELETE a file
        return fs_delete_valid(cmd);
    } else if (!strcmp(cmd->type, "R")) {
        // READ a file
        return fs_read_valid(cmd);
    } else if (!strcmp(cmd->type, "W")) {
        // WRITE to a file
        return fs_write_valid(cmd);
    } else if (!strcmp(cmd->type, "B")) {
        // update the BUFFER
        return fs_buff_valid(cmd);
    } else if (!strcmp(cmd->type, "L")) {
        // LISTS files and directories in cwd
        return fs_ls_valid(cmd);
    } else if (!strcmp(cmd->type, "O")) {
        // DEFRAGMENT the disk
        return fs_defrag_valid(cmd);
    } else if (!strcmp(cmd->type, "Y")) {
        // CHANGE the cwd
        return fs_cd_valid(cmd);
    } else {
        // If the command type doesn't match any of the expected value, it is invalid
        return 0;
    }
}

/**
 * @brief Validate a MOUNT command.
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_mount_valid(Command *cmd) {
    // args: char *name
    // First check # of args
    if (cmd->size != 2) return 0;

    return 1;
}

/**
 * @brief Validate a CREATE command.
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_create_valid(Command *cmd) {
    // args: char name[5], int size
    // First check # of args
    if (cmd->size != 3) return 0;

    // Check if length of name is > 5
    if (strlen(cmd->argv[1]) > 5) return 0;

    // Check if second arg can be converted to an int
    char *endptr;
    long val;
    errno = 0;
    val = strtol(cmd->argv[2], &endptr, 10);
    if (errno == ERANGE) return 0; // Resulting value out of range
    if (endptr == cmd->argv[2]) return 0; // No digits found
    if (*endptr != '\0') return 0; // Further characters after number

    // Check if file size is < 0 or > 127
    if (val < 0 || val > 127) return 0;

    // Valid command
    return 1;
}

/**
 * @brief Validate a DELETE command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_delete_valid(Command *cmd) {
    // args: char name[5]
    // First check # of args
    if (cmd->size != 2) return 0;

    // Check if length of name is > 5
    if (strlen(cmd->argv[1]) > 5) return 0;

    return 1;
}

/**
 * @brief Validate a READ command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_read_valid(Command *cmd) {
    // args: char name[5], int block_num
    // First check # of args
    if (cmd->size != 3) return 0;

    // Check if length of name is > 5
    if (strlen(cmd->argv[1]) > 5) return 0;

    // Check if second arg can be converted to an int
    char *endptr;
    long val;
    errno = 0;
    val = strtol(cmd->argv[2], &endptr, 10);
    if (errno == ERANGE) return 0; // Resulting value out of range
    if (endptr == cmd->argv[2]) return 0; // No digits found
    if (*endptr != '\0') return 0; // Further characters after number

    // Check if block index is < 0 or > 126
    if (val < 0 || val > 126) return 0;

    return 1;
}

/**
 * @brief Validate a WRITE command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_write_valid(Command *cmd) {
    // args: char name[5], int block_num
    // First check # of args
    if (cmd->size != 3) return 0;

    // Check if length of name is > 5
    if (strlen(cmd->argv[1]) > 5) return 0;
    
    // Check if second arg can be converted to an int
    char *endptr;
    long val;
    errno = 0;
    val = strtol(cmd->argv[2], &endptr, 10);
    if (errno == ERANGE) return 0; // Resulting value out of range
    if (endptr == cmd->argv[2]) return 0; // No digits found
    if (*endptr != '\0') return 0; // Further characters after number

    // Check if block index is < 0 or > 126
    if (val < 0 || val > 126) return 0;

    return 1;
}

/**
 * @brief Validate a BUFFER update command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_buff_valid(Command *cmd) {
    // args: uint8_t buff[1024]
    // First check # of args, return if no characters are being passed to buffer
    if (cmd->size == 1) return 0;
    
    return 1;
}

/**
 * @brief Validate a LIST files command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_ls_valid(Command *cmd) {
    // First check # of args
    if (cmd->size != 1) return 0;

    return 1;
}

/**
 * @brief Validate a DEFRAGMENT command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_defrag_valid(Command *cmd) {
    // First check # of args
    if (cmd->size != 1) return 0;

    return 1;
}

/**
 * @brief Validate a CHANGE DIRECTORY command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_cd_valid(Command *cmd) {
    // args: char name[5]
    // First check # of args
    if (cmd->size != 2) return 0;

    // Check if length of name is > 5
    if (strlen(cmd->argv[1]) > 5) return 0;

    return 1;
}