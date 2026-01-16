#ifndef FS_SIM_H
#define FS_SIM_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char name[5];         // name of the file/directory
    uint8_t isused_size;  // state of inode and size of the file/directory
    uint8_t start_block;  // index of the first block of the file/directory
    uint8_t isdir_parent; // type of inode and index of the parent inode
} Inode;

typedef struct {
    uint8_t free_block_list[16];
    Inode inode[126];
} Superblock;

typedef struct {
    char *input_file;   // Name of file the command originated from
    size_t line_num;    // Line number the command is on
    char *type;         // Command type ex. "M"
    char **argv;        // Command arg array
    uint8_t buff[1024]; // buffer (ONLY USED IN BUFFER COMMAND)
    size_t size;        // # of args (including the command)
} Command;

/**
 * @brief Mounts the file system residing on the specified virtual disk.
 * 
 * @param new_disk_name - name of disk to be mounted
 */
void fs_mount(char *new_disk_name);

/**
 * @brief Creates a new file or directory in the current working directory 
 * with the given name and the given number of blocks, 
 * and stores the attributes in the first available inode. 
 * 
 * @param name - Name of the file or directory to create
 * @param size - # of contiguous block the file will require (0 if directory)
 */
void fs_create(char name[5], int size);

/**
 * @brief Deletes the file or directory with the given name in the current working directory. 
 * 
 * @param name - name of the file or directory to be deleted
 */
void fs_delete(char name[5]);

/**
 * @brief Opens the file with the given name 
 * and reads the block num-th block of the file into the buffer.
 * 
 * @param name - name of the file to read
 * @param block_num - index of the block to read [0, size-1]
 */
void fs_read(char name[5], int block_num);

/**
 * @brief Opens the file with the given name 
 * and writes the content of the buffer to the block num-th block of the file.
 * 
 * @param name - name of file to open
 * @param block_num - block number of file to write to
 */
void fs_write(char name[5], int block_num);

/**
 * @brief Flushes the buffer by zeroing it and writes the new bytes into the buffer.
 * 
 * @param buff - buffered values from input to send to file system buffer
 */
void fs_buff(uint8_t buff[1024]);

/**
 * @brief Lists all files and directories that exist in the current directory.
 */
void fs_ls(void);

/**
 * @brief Re-organizes the data blocks such that there is no free block between the used blocks,
 * and between the superblock and the used blocks.
 */
void fs_defrag(void);

/**
 * @brief Changes the current working directory
 * to a directory with the specified name in the current working directory.
 * 
 * @param name - name of the directory to change to, ".." for previous directory
 */
void fs_cd(char name[5]);

extern int vd; // Virtual Disk file descriptor
extern int cwd; // Current working directory (root directory is 127)
extern char *disk_name; // Name of current mounted disk
extern uint8_t fs_buffer[1024]; // File system buffer
extern Superblock *sb; // Superblock of current virtual disk

#endif