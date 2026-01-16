#include "fs-sim.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// GLOBAL VARIABLES
int vd = -1; // Initialize file descriptor for the virtual disk
int cwd; // Current working directory of mounted file system
char *disk_name = NULL; // Name of current mounted disk
uint8_t fs_buffer[1024]; // File system buffer
Superblock *sb = NULL; // Superblock of current virtual disk

/**
 * @brief Writes current superblock in memory to the virtual disk
 */
void write_superblock() {
    lseek(vd, 0, SEEK_SET);
    write(vd, sb, 1024);
}

/**
 * @brief checks if file or directory of a given name exists in the current working directory.
 * 
 * @param name - Name to perform check on
 * @return Integer value index of the inode if it exists, -1 otherwise
 */
int file_exists(char name[5]) {
    for (size_t i=0; i < 126; i++) {
        Inode *inode_temp = &sb->inode[i];
        uint8_t isused_temp = inode_temp->isused_size & (1 << 7);
        uint8_t parent_temp = inode_temp->isdir_parent & ~(1 << 7);
        uint8_t isdir = inode_temp->isdir_parent & (1 << 7);
        // If current inode shares the same name and parent directory, return true
        if ((isused_temp) && (memcmp(name,inode_temp->name,5) == 0) && (parent_temp == cwd)) {
            return i;
        }
    }
    return -1; // No file with given name exists in the cwd
}

/**
 * @brief Sets the blocks between start_idx and end_idx to 1 or 0
 * 
 * @param start_idx - Index of first block to set
 * @param size - Size of the block array to update
 * @param set - If 1 set bits to 1, if 0 set bits to 0
 */
void set_fbl_bits(int start_idx, int size, int set) {
    for (int i=start_idx; i < start_idx+size; i++) {
        if (i == 0) continue;   // skip super block bit
        int byte = i / 8;
        int bit  = 7 - (i % 8);
        if (set)
            sb->free_block_list[byte] |=  (1 << bit);
        else
            sb->free_block_list[byte] &= ~(1 << bit);
    }
}

/**
 * @brief deletes files and directories. If inode is directory, recursively delete all files in the directory.
 * 
 * @param inode_idx - Index of the Inode of the file or directory to be deleted
 */
void delete_file(int inode_idx) {
    // Get inode to be deleted
    //printf("deleteing inode: %d\n", inode_idx); // TESTING PRINT STATEMENT
    Inode *inode = &sb->inode[inode_idx];
    uint8_t size = inode->isused_size & ~(1 << 7);
    uint8_t start_block = inode->start_block;
    uint8_t isdir = inode->isdir_parent & (1 << 7);
    uint8_t parent = inode->isdir_parent & ~(1 << 7);

    // Zero buffer
    uint8_t zero_buff[1024] = {0};

    if (isdir) {
        // DIRECTORY
        // Delete all files in the directory
        for (size_t i=0; i < 126; i++) {
            if (i == inode_idx) continue; // Skip the current directory inode
            if ((sb->inode[i].isused_size & (1 << 7)) && ((sb->inode[i].isdir_parent & ~(1 << 7)) == inode_idx)) {
                delete_file(i);
            }
        }
    } else {
        // FILE
        lseek(vd, (1024)*start_block, SEEK_SET); // Move virtual disk position to start block
        for (size_t i=0; i < size; i ++) write(vd, zero_buff, 1024); // Zero out all blocks
        set_fbl_bits(start_block, size, 0); // "Un"set bits in free block array
    }

    // Zero out Inode in superblock struct
    for (size_t i=0; i < 5; i++) inode->name[i] = 0;
    inode->isused_size = 0;
    inode->start_block = 0;
    inode->isdir_parent = 0;

    return;
}

/**
 * @brief checks the consitency of the current virtual disk
 * 
 * @param super_block - pointer to superblock to perform a consitency check on
 * @return Integer value that corresponds to the smallest error code encountered
 */
int consistency_check(Superblock *super_block) {
    int alloced_blocks[128]; // Array of 1s and 0s to track which blocks are allocated by inodes
    int fbl_error = 0;
    for (size_t i=0; i < 128; i++) alloced_blocks[i] = 0;

    // 1. If the state of an inode is free, then all bits in this inode must be zero. Otherwise, the name attribute
    // stored in the inode must start with a nonzero byte.
    for (size_t i=0; i < 126; ++i) {
        // get inode properties
        Inode *inode = &super_block->inode[i];
        uint8_t isused = inode->isused_size & (1 << 7);
        uint8_t size = inode->isused_size & ~(1 << 7);
        uint8_t start_block = inode->start_block;
        uint8_t isdir = inode->isdir_parent & (1 << 7);
        uint8_t parent = inode->isdir_parent & ~(1 << 7);

        // check if inode is used
        if (isused) {
            // IN USE
            if ((uint8_t)inode->name[0] == 0) return 1; // inode is in use and name is invalid
        } else {
            // FREE
            for (size_t k=0; k < 5; k++){
                if ((uint8_t)inode->name[k] != 0) return 1;
            }
            if (inode->isused_size != 0) return 1;
            if (inode->start_block != 0) return 1;
            if (inode->isdir_parent != 0) return 1;
        }
    }

    // 2. The start block of every inode that is in use and pertains to a file (i.e. when the isdir bit is not set)
    // must have a value between 1 and 127 inclusive. Moreover, the size of every inode that is in use and
    // pertains to a file must be such that its last block is also between 1 and 127.
    for (size_t i=0; i < 126; ++i) {
        // get inode properties
        Inode *inode = &super_block->inode[i];
        uint8_t isused = inode->isused_size & (1 << 7);
        uint8_t size = inode->isused_size & ~(1 << 7);
        uint8_t start_block = inode->start_block;
        uint8_t isdir = inode->isdir_parent & (1 << 7);

        // check if inode is used
        if (isused) {
            // IN USE
            // Check if inode pertains to a file
            if (!isdir) {
                // FILE
                if (start_block < 1 || start_block > 127) return 2; // Start block index out of range
                if ((start_block + (size - 1)) > 127) return 2; // End block index out of range
                // Updated alloced_blocks list when blocks of file within valid range
                for (size_t k=start_block; k < start_block + size; k++) {
                    if (alloced_blocks[k] == 1) {
                        fbl_error = 1; // Block is allocated to more than one file
                    } else {
                        alloced_blocks[k] = 1;
                    }
            }
            }       
        }
    }

    // 3. The size and start block of an inode pertaining to a directory (i.e. the directory bit is set) must
    // be zero.
    for (size_t i=0; i < 126; ++i) {
        // get inode properties
        Inode *inode = &super_block->inode[i];
        uint8_t isused = inode->isused_size & (1 << 7);
        uint8_t size = inode->isused_size & ~(1 << 7);
        uint8_t start_block = inode->start_block;
        uint8_t isdir = inode->isdir_parent & (1 << 7);

        // check if inode is used
        if (isused) {
            // Check if inode pertains to a directory
            if (isdir) {
                // DIRECTORY
                if (size != 0 || start_block != 0) return 3; // Directory size and/or start_block is not 0
            }
        }
    }

    // 4. For every inode that is in use, the index of its parent inode cannot be the same as its own index and
    // cannot be 126. Moreover, if the index of the parent inode is between 0 and 125 inclusive, then the
    // parent inode must be in use and marked as a directory.    
    for (size_t i=0; i < 126; ++i) {
        // get inode properties
        Inode *inode = &super_block->inode[i];
        uint8_t isused = inode->isused_size & (1 << 7);
        uint8_t parent = inode->isdir_parent & ~(1 << 7);

        // check if inode is used
        if (isused) {
            if (i == parent || parent == 126) return 4; // inode idx = parent idx OR parent idx = 126
            if (parent >= 0 && parent <= 125) {
                Inode *parent_node = &super_block->inode[parent];
                if (!(parent_node->isused_size & (1 << 7))) return 4; // parent inode not in use
                if (!(parent_node->isdir_parent & (1 << 7))) return 4; // parent inode not directory
            }
        }
    }

    // 5. The name of every file/directory must be unique in each directory (names do not need to be unique
    // across the entire file system).
    for (size_t i=0; i < 126; i++) {
        Inode *inode1 = &super_block->inode[i];
        uint8_t isused1 = inode1->isused_size & (1 << 7);
        uint8_t parent1 = inode1->isdir_parent & ~(1 << 7);
        if (!isused1) continue;
        for (size_t k=0; k < 126; k++) {
            Inode *inode2 = &super_block->inode[k];
            uint8_t isused2 = inode2->isused_size & (1 << 7);
            uint8_t parent2 = inode2->isdir_parent & ~(1 << 7);
            // Check that Inode 2 is in use and not the same node as inode1
            if ((isused2) && (i != k)) {
                // If two names are the same and of the same parent return error code 5
                if ((memcmp(inode1->name,inode2->name,5) == 0) && (parent1 == parent2)) return 5;
            }
        }
    }

    // 6. Blocks that are marked free in the free-space list cannot be allocated to any file. Similarly, blocks that
    // are marked in use in the free-space list must be allocated to exactly one file
    
    if (fbl_error) return 6;
    for (int i=0; i < 128; i++) {
        if (i == 0) continue;   // skip super block bit
        int byte = i / 8;
        int bit  = 7 - (i % 8);
        if (!(super_block->free_block_list[byte] & (1 << bit))) {
            // Block is marked as free
            if (alloced_blocks[i] == 1) return 6; // Block is marked as free in fbl but is used by inode
        }
    }

    return 0;
}

/**
 * @brief Mounts the file system residing on the specified virtual disk.
 * 
 * @param new_disk_name - name of disk to be mounted
 */
void fs_mount(char *new_disk_name) {
    // First, check if virtual disk with the given name exists in the cwd
    // If it exists, mount the virtual disk
    int vd_new;
    if ((vd_new = open(new_disk_name,O_RDWR)) == -1) {
        fprintf(stderr, "Error: Cannot find disk %s\n", new_disk_name);
        return;
    }

    Superblock *sb_new = malloc(sizeof(Superblock)); // Allocate memory for the superblock
    lseek(vd_new, 0, SEEK_SET);
    read(vd_new, sb_new, 1024); // Load the superblock of the new virtual disk

    // Perform consistency check on the virtual disk and print error if neccessary
    int error = consistency_check(sb_new);
    if (error != 0) {
        fprintf(stderr, "Error: File system in %s is inconsistent (error code: %d)\n", new_disk_name, error);
        close(vd_new);
        free(sb_new);
        return;
    }

    // If no error is encountered, free old global vars and assign new ones
    if (vd != -1) close(vd);
    if (sb != NULL) free(sb);
    if (disk_name != NULL) free(disk_name);
    vd = vd_new;
    sb = sb_new;
    disk_name = strdup(new_disk_name);
    cwd = 127;

    return;
}

/**
 * @brief Creates a new file or directory in the current working directory 
 * with the given name and the given number of blocks, 
 * and stores the attributes in the first available inode. 
 * 
 * @param name - Name of the file or directory to create
 * @param size - # of contiguous block the file will require (0 if directory)
 */
void fs_create(char name[5], int size) {
    // Find first available inode
    Inode *inode = NULL;
    size_t idx = 0;
    size_t i = 0;
    while ((inode == NULL) && (i < 126)) {
        uint8_t isused = sb->inode[i].isused_size & (1 << 7);
        if (!isused) {
            inode = &sb->inode[i];
            idx = i;
        }
        i++;
    }
    // if idx = 126 then there are no free inodes, print an error
    if (i == 126) {
        fprintf(stderr, "Error: Superblock in disk %s is full, cannot create %s\n", disk_name, name);
        return;
    }
    
    // CHECK FOR NAMING DUPLICATES
    if (file_exists(name) >= 0) {
        fprintf(stderr, "Error: File or directory %s already exists\n", name);
        return;
    }
    if (memcmp(name, ".\0\0\0\0", 5) == 0 || memcmp(name, "..\0\0\0", 5) == 0) {
        fprintf(stderr, "Error: File or directory %s already exists\n", name);
        return;
    }

    // CHECK FOR CONTIGUOUS BLOCK GROUP (only if not creating a directory)
    int block_count = 0; // Track # of contiguous blocks found after found_block flag
    int start_block_idx = -1; // Stores index of start block for a valid contiguous group of memory
    if (size > 0) {
    for (size_t i=0; i < 128; i++) {
        if (i == 0) continue;   // skip super block bit
        int byte = i / 8;
        int bit  = 7 - (i % 8);
        if (!(sb->free_block_list[byte] & (1 << bit))) {
            // Block is marked as free
            block_count += 1;
            if (block_count == size) {
                start_block_idx = (int)(i - size + 1);
                break;
            }
        } else {
            // Block is in use
            block_count = 0; // Reset count of contiguous blocks
        }
    }    
    // Print error if not enough contiguous blocks in memory
    if (start_block_idx == -1) {
        fprintf(stderr, "Error: Cannot allocate %d blocks on %s\n", size, disk_name);
        return;
    }
    }

    // ALL TESTS PASSED, ASSIGN INODE TO FILE OR DIRECTORY
    strncpy(inode->name, name, 5); // Set the name
    inode->isused_size = (uint8_t)size; // Set the size
    inode->isused_size |= (1 << 7); // Set the is used bit
    if (size == 0) inode->start_block = 0; // Set the start block
    else inode->start_block = start_block_idx;
    inode->isdir_parent = (uint8_t)cwd; // Set the parent inode
    if (size == 0) inode->isdir_parent |= (1 << 7); // Set the is directory bit if size = 0

    if (size > 0) set_fbl_bits(start_block_idx, size, 1); // Update fbl bits

    // Update the superblock in the virtual disk
    write_superblock();

    return;
}

/**
 * @brief Deletes the file or directory with the given name in the current working directory. 
 * 
 * @param name - name of the file or directory to be deleted
 */
void fs_delete(char name[5]) {
    // First, check if file or directory with the name exists in the cwd
    int idx = file_exists(name); // Inode index of the file to delete
    if (idx == -1) {
        fprintf(stderr, "Error: File or directory %s does not exist\n", name);
        return;
    }

    delete_file(idx); // Delete the file

    write_superblock(); // Write changes to the virtual disk

    return;
}

/**
 * @brief Opens the file with the given name 
 * and reads the block num-th block of the file into the buffer.
 * 
 * @param name - name of the file to read
 * @param block_num - index of the block to read [0, size-1]
 */
void fs_read(char name[5], int block_num) {
    // First, check if file or directory with the name exists in the cwd
    int idx = file_exists(name); // Inode index of the file to delete
    if (idx == -1) {
        fprintf(stderr, "Error: File %s does not exist\n", name);
        return;
    }

    Inode *inode = &sb->inode[idx]; // Inode of file to be read
    uint8_t size = inode->isused_size & ~(1 << 7);
    uint8_t start_block = inode->start_block;
    uint8_t isdir = inode->isdir_parent & (1 << 7);
    uint8_t parent = inode->isdir_parent & ~(1 << 7);

    // Print error and return if the file trying to be read is a directory
    if (isdir) {
        fprintf(stderr, "Error: File %s does not exist\n", name);
        return;
    }
    // If block_num is not in the range of the file, print an error
    if (block_num < 0 || block_num > size-1) {
        fprintf(stderr, "Error: %s does not have block %d\n", name, block_num);
        return;
    }

    // If no errors, read the block into the buffer
    lseek(vd, (1024)*(start_block+block_num), SEEK_SET);
    read(vd, fs_buffer, 1024);

    return;
}

/**
 * @brief Opens the file with the given name 
 * and writes the content of the buffer to the block num-th block of the file.
 * 
 * @param name - name of file to open
 * @param block_num - block number of file to write to
 */
void fs_write(char name[5], int block_num) {    
    // First, check if file or directory with the name exists in the cwd
    int idx = file_exists(name); // Inode index of the file to write to
    if (idx == -1) {
        fprintf(stderr, "Error: File %s does not exist\n", name);
        return;
    }

    Inode *inode = &sb->inode[idx]; // Inode of file to be written to
    uint8_t size = inode->isused_size & ~(1 << 7);
    uint8_t start_block = inode->start_block;
    uint8_t isdir = inode->isdir_parent & (1 << 7);
    uint8_t parent = inode->isdir_parent & ~(1 << 7);

    // Print error and return if the file trying to be written to is a directory
    if (isdir) {
        fprintf(stderr, "Error: File %s does not exist\n", name);
        return;
    }
    // If block_num is not in the range of the file, print an error
    if (block_num < 0 || block_num > size-1) {
        fprintf(stderr, "Error: %s does not have block %d\n", name, block_num);
        return;
    }

    // If no errors, write to the block from the buffer
    lseek(vd, (1024)*(start_block+block_num), SEEK_SET);
    write(vd, fs_buffer, 1024);

    return;
}

/**
 * @brief Flushes the buffer by zeroing it and writes the new bytes into the buffer.
 * 
 * @param buff - buffered values from input to send to file system buffer
 */
void fs_buff(uint8_t buff[1024]) {
    for (size_t i=0; i < 1024; i++) fs_buffer[i] = 0;
    memcpy(fs_buffer, buff, 1024);
}

/**
 * @brief Lists all files and directories that exist in the current directory.
 */
void fs_ls(void) {
    // Print number of children in cwd
    int num_of_children_cwd = 2;
    for (size_t i=0; i < 126; i++) {
        if ((sb->inode[i].isdir_parent & ~(1 << 7)) == cwd && (sb->inode[i].isused_size & (1 << 7))) num_of_children_cwd++;
    }
    printf("%-5s %3d\n", ".", num_of_children_cwd);

    // Print number of children in directory one level up from cwd if not root
    int num_of_children_prevwd = 2;
    if (cwd == 127) {
        printf("%-5s %3d\n", "..", num_of_children_cwd); // cwd is root
    } else {
        for (size_t i=0; i < 126; i++) {
            if ((sb->inode[i].isdir_parent & (1 << 7)) && i == cwd && (sb->inode[i].isused_size & (1 << 7))) {
                int prevwd = sb->inode[i].isdir_parent & ~(1 << 7);
                for (size_t k=0; k < 126; k++) {
                    if ((sb->inode[k].isdir_parent & ~(1 << 7)) == prevwd && (sb->inode[k].isused_size & (1 << 7))) num_of_children_prevwd++;
                }
                break;
            }
        }
        printf("%-5s %3d\n", "..", num_of_children_prevwd);
    }
    
    // Print files and directories in cwd
    for (size_t i=0; i < 126; i++) {
        Inode *inode = &sb->inode[i];
        char name[5];
        for (size_t j=0; j < 5; j++) name[j] = inode->name[j];
        uint8_t isused = inode->isused_size & (1 << 7);
        uint8_t size = inode->isused_size & ~(1 << 7);
        uint8_t isdir = inode->isdir_parent & (1 << 7);
        uint8_t parent = inode->isdir_parent & ~(1 << 7);

        if (isdir && isused) {
            // DIRECTORY
            if (parent == cwd) {
                // Get # of children
                int num_of_children = 2;
                for (size_t k=0; k < 126; k++) {
                    if ((sb->inode[k].isdir_parent & ~(1 << 7)) == i && (sb->inode[k].isused_size & (1 << 7))) num_of_children++;
                }
                printf("%-5s %3d\n", name, num_of_children);
            }
        } else {
            // FILE
            // Print files that are in the cwd
            if ((parent == cwd) && (isused)) {
                printf("%-5s %3d KB\n", name, size);
            }
        }
    }
    return;
}

/**
 * @brief Re-organizes the data blocks such that there is no free block between the used blocks,
 * and between the superblock and the used blocks.
 */
void fs_defrag(void) {
    // Find index of lowest available block and first used block
    size_t fbl_idx = 0;
    int done = 0;
    while(!done) {
        int lowest_avail_idx = -1;
        int next_used_idx = -1;
        for (size_t i=fbl_idx; i < 128; i++, fbl_idx++) {
            if (i == 0) continue;   // skip super block bit
            int byte = i / 8;
            int bit  = 7 - (i % 8);
            if (!(sb->free_block_list[byte] & (1 << bit))) {
                // Block is marked as free
                // Store the index of the first free block found if one has not already been found
                if (lowest_avail_idx == -1) lowest_avail_idx = i;
            } else {
                // Block is used
                // Store the index of the first used block found after the first free block
                if (lowest_avail_idx != -1 && next_used_idx == -1) {
                    next_used_idx = i;
                    break;
                }
            }
        }
        
        // If next used_idx was not set in the loop, then there are no more blocks to move
        if (next_used_idx == -1) {
            done = 1;
            continue;
        }
    
        // Get Inode referring to first available block
        Inode *inode;
        int inode_idx = 0;
        for(size_t i=0; i < 126; i++) {
            uint8_t isused = sb->inode[i].isused_size & (1 << 7);
            uint8_t isdir  = sb->inode[i].isdir_parent & (1 << 7);
            uint8_t start_block = sb->inode[i].start_block;
            if ((start_block == next_used_idx) && !(isdir) && (isused)) {
                inode = &sb->inode[i];
                inode_idx = i;
                break;
            }
        }
        //printf("Moving blocks of inode: %d\n", inode_idx); //TESTING PRINT STATEMENT
        // Move the block(s)
        uint8_t size = inode->isused_size & ~(1 << 7);
        uint8_t start_block = inode->start_block;
        uint8_t temp_buff[1024];
        uint8_t zero_buff[1024] = {0};
        for (size_t i=0; i < size; i++) {
            lseek(vd, (1024)*(start_block+i), SEEK_SET);
            read(vd, temp_buff, 1024); // Get data of block to move
            lseek(vd, (1024)*(start_block+i), SEEK_SET);
            write(vd, zero_buff, 1024); // Clear the block that is being moved
            lseek(vd, (1024)*(lowest_avail_idx+i), SEEK_SET);
            write(vd, temp_buff, 1024); // Write the data to the new block location
        }

        // "Un"set bits in free block array for old blocks
        set_fbl_bits(start_block, size, 0);

        // Set bits in free block array for new blocks
        set_fbl_bits(lowest_avail_idx, size, 1);
    
        inode->start_block = lowest_avail_idx; // Set the new start block for the inode
        // Updated inode in virtual disk
        write_superblock();

        // Set index for next loop to one block after the block that was moved
        fbl_idx = lowest_avail_idx + size;
    }
    return;
}

/**
 * @brief Changes the current working directory
 * to a directory with the specified name in the current working directory.
 * 
 * @param name - name of the directory to change to, ".." for previous directory
 */
void fs_cd(char name[5]) {
    if (memcmp(name, ".\0\0\0\0", 5) == 0) {
        // cd to current directory
        return;
    } else if (memcmp(name, "..\0\0\0", 5) == 0) {
        // cd one directory up
        if (cwd == 127) return; // Already in root directory
        // find index of directory one directory up and change cwd
        for (size_t i=0; i < 126; i++) {
            if ((sb->inode[i].isdir_parent & (1 << 7)) && i == cwd) {
                int prevwd = sb->inode[i].isdir_parent & ~(1 << 7);
                cwd = prevwd;
                return;
            }
        }
    } else {
        // Check if directory exists in cwd
        int idx = file_exists(name); // index of file or directory with name
        if (idx == -1 || !(sb->inode[idx].isdir_parent & (1 << 7))) {
            fprintf(stderr, "Error: Directory %s does not exist\n", name);
            return;
        }
        // Change cwd to index of valid directory
        cwd = idx;
        //printf("Current working directory changed to: %d \n", cwd);
        return;
    }
    return;
}