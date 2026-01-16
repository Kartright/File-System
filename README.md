# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Name : Benjamin Hall
# SID : 1757660
# CCID : bbhall
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Overview
This project contains 3 .c files and 2 .h files. The fs-sim.h & .c files contain the function definitions and descriptions for the commands that simulate the virtual file system. fs-main.c contains the main function of the program and other functions required to parse commands from an input file, send then to validation, and run the appropriate fs-sim function (if valid). The fs-validate.h & .c files contain a function definitions and descriptions that will validate the command parameters for each type of command function in fs-sim.c to ensure it can be run by the file simulator; it also contains a validateCommand function that will automatically check which command is being parsed and run the appropriate validate function.

# Design
## fs-sim
### fs_mount()
#### System Calls
- **open()**   
- **lseek()**   
- **read()**   
- **close()**   

fs_mount is called to mount a virtual disk into memory by storing its disk name, file descriptor, current working directory, and a copy of its superblock in memory as a superblock struct. This function uses the **open()** system call open the disk file with the provided name in read/write mode and get its file descriptor; error checking is done to make sure a disk with the provided name exists. **lseek()** is then used to set the file pointer to the start of the file before calling **read()** to read the first 1024 bytes of the disk (the superblock) into a new superblock struct. The program then performs a consistency check to make sure the disk's memory is consistent. If the disk fails the consistency check an error is printed, the disk is closed using the **close()** system call, and the new superblock struct memory is freed. If the disk passes the consistency check and there was a previously mounted disk, the previously mounted disk is closed with the **close()** system call, and other global memory pertaining to the old disk is also freed (cwd is also set to root directory).

### fs_create()
#### System Calls  
- **lseek()**   
- **write()**     

fs_create is called to create a new file or direcotry in the current working directory of the mounted disk. The function first performs some validation steps to ensure that a new file with the provided name can be created on the disk. This process includes calling the file_exists() function that checks the superblock for a file with the given name in the current working directory, and if it does it will return the Inode index of the file, otherwise it returns 0. Next, the function check the free block list in the superblock struct to see if there is a contiguous block of memory large enough for the new file, if there is, the file can be created and the Inode for the new file is populated with the proper information (skipped for directories). This step also includes calling the set_fbl_bits() function to set the bits corresponding to the new file block in the freeblock list of the superblock struct to 1 (skipped for directories). Finally, all the changes to the in-memory superblock struct are commited to the mounted disk by calling the write_superblock() function which uses the system call **lseek()** to set the file pointer to the beginning of the disk before using the system call **write()** to write the modified superblock to the first 1024 bytes of the disk.

### fs_delete()
#### System Calls  
- **lseek()**   
- **write()**   

fs_delete calls file_exists() (explained in fs_create) to first check that the file or directory to be deleted exists. The delete_file() function is then called which uses the system call **lseek()** to move the file pointer to the first memory block of the file and then call **write()** in a loop for each contigious block of the file to zero out all 1024 bytes of each block. The blocks of delted file are also zeroed out in the free block list of the superblock struct using hte set_fbl_bits() function. If the file being deleted is a directory it will recursively call file_delete to remove all files and direcotries contained within the directory being deleted. The Inode for each file and directory is also zeroed out in the superblock struct so that write_superblock() can used **lseek()** and **write()** to commit the new changes in the superblock to the disk at the end of the fs_delete() function.

### fs_read()
#### System Calls  
- **lseek()**   
- **read()**  

The fs_read() function first checks to make sure a file with the provided name exists and is not a directory (using file_exists) and ensures that the block number to be read is within the file size. If both pass, the system calls **lseek()** and **read()** are used to first move the file pointer to the block to read from, and then read the 1024 bytes of that block into the memory buffer.

### fs_write()
#### System Calls  
- **lseek()**   
- **write()**  

The fs_write() function first checks to make sure a file with the provided name exists and is not a direcotry (using file_exists) and ensures that the block number to be written to is within the file size. If both pass, the system calls **lseek()** and **write()** are used to first move the file pointer to the block to write to, and then write the 1024 bytes of the memory buffer to that block.

### fs_buff()
#### System Calls  
**NONE**   

fs_buff() requires no system calls and simply zeroes out the memory buffer before copying they bytes from the valid command buffer command into the zeroed out memory buffer.

### fs_ls()
#### System Calls  
**NONE**   

fs_ls() also requries no system calls. This function will prin, to stdout, the names of each file and directory in the current working directory (including "." and "..") along with their repective sizes. This only requries the up-to-date in-memory superblock struct to perform the function. 

### fs_defrag()
#### System Calls  
- **lseek()**   
- **read()**   
- **write()**  

fs_defrag() moves contiguous groups of memory blocks pertaining to files to the first available free block in memory. When a group of block is found and to be moved, the system calls **lseek()**, **read()**, and **write()** are called multiple times in a loop to carefully move the blocks to their new position on the disk and zero out an unused blocks. In each loop the file pointer is moved to a block to be moved using **lseek()**, all 1024 bytes of the block are read into a temporary buffer using **read()**, the file pointer is then moved back to the block that was read from to zero it out using **lseek()** and **write()**, and then the file pointer is moved to the new location for the block using **lseek()** so the data in the temporary buffer can be written to the new block location using **write()** (this is done for each block of a file starting at the first block). Using set_fbl_bits() the bits of the old block locations are zeroed out in the superblock struct and the new block locations are set to 1. The new start block of the file is written to its Inode in the superblock stuct and write_superblock() is called to used **lseek()** and **write()** to commit the changes of the in-memory superblock to the virtual disk. This process is done in a loop until there there is no more free holes in the disk block structure.

### fs_cd()
**NONE**   

fs_cd() requires no system calls, it simply changes the global "cwd" variable to the index of the directory with the provided name. file_exists() is called to ensure that a directory with the provied name does infact exist in the current working directory.

## fs-main
#### System Calls  
- **close()**   

fs-main parses the commands from and input file and runs the requried function after a successful validation step. The library functions fopen() and fclose() are used to open and close the input file that contains all of the commands to run. The system call **close()** is used right before the end of the program to ensure that the current mounted disk (if applicable) is properly closed.

## fs-validate
#### System Calls
**NONE**

The fs-validate function do not use any system calls and simply take a command struct and make sure that it contains valid information for the command it is executing.

## Command Struct
### Properties
- char *input_file;   // Name of file the command originated from   
- size_t line_num;    // Line number the command is on   
- char *type;         // Command type ex. "M"   
- char **argv;        // Command arg array   
- uint8_t buff[1024]; // buffer (ONLY USED IN BUFFER COMMAND)   
- size_t size;        // # of args (including the command)   

The command struct is primarily used in the fs-main and fs-validate files to easily store information about a command being parsed including, the name of the file it comes from, the line it appears on, the type of command it is, an array of its arguments, a buffer, and the number of arguments it contains. This makes parsing commands, validating commands, and passing command information between functions much simpler and more readable.

# Testing
(All tests were performed using "valgrind --tool=memcheck --leak-check=yes" to check for memory leaks and errors)
The main method for testing was using the test.py pthon script provided with the assignment. This made it easy to see if an error was related to disk management, error messages, or printing to stdout. To further narrow down specific issues a separate test input file was used that would be modified as needed to test any specific problems. A new makefile target was created called "cleandisk" that would delete disks and make new ones using ./create_fs so that fresh disks could be used each time a test was done using the non-provided test input file. The test.py python script was also temporarliy modified to run valgrind to quickly test that all of the provided test cases did not cause any memory leaks or errors.

# References
Function "breifs" for the provided fuctions were copied from the assignment description.  
Checking if c string can be converted to int: https://man7.org/linux/man-pages/man3/strtol.3.html   
Reading individual bits from a uint8_t type: https://stackoverflow.com/questions/19626652/how-to-read-specific-bits-of-an-unsigned-int   
