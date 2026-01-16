#ifndef FS_VALIDATE_H
#define FS_VALIDATE_H

#include "fs-sim.h"

/**
 * @brief Validate the given command based on the command it will execute.
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int validateCommand(Command *cmd);

/**
 * @brief Validate a MOUNT command.
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_mount_valid(Command *cmd);

/**
 * @brief Validate a CREATE command.
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_create_valid(Command *cmd);

/**
 * @brief Validate a DELETE command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_delete_valid(Command *cmd);

/**
 * @brief Validate a READ command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_read_valid(Command *cmd);

/**
 * @brief Validate a WRITE command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_write_valid(Command *cmd);

/**
 * @brief Validate a BUFFER update command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_buff_valid(Command *cmd);

/**
 * @brief Validate a LIST files command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_ls_valid(Command *cmd);

/**
 * @brief Validate a DEFRAGMENT command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_defrag_valid(Command *cmd);

/**
 * @brief Validate a CHANGE DIRECTORY command
 * 
 * @param cmd - Instance of the command struct that contains information about command to run.
 * @return Integer value 0 if invalid, 1 if valid.
 */
int fs_cd_valid(Command *cmd);

#endif