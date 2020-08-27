/*
NAME: Marcus Pearce, Ryan Lam
EMAIL: marcuspearce7@g.ucla.edu, ryan.lam53@gmail.com
ID: 305115848, 705124474
*/

// CS 111 Project 3a


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>

#include "ext2_fs.h"

struct ext2_super_block superblock;
struct ext2_group_desc groupdesc;
char* blockbitmapbuf;
char* inodebitmapbuf;

int fd;
__u32 blocksize;



// external function because needs to be called recursively until get to actual block 
// referenced piazza post 736 -> explained logical block offset for indirect blocks
// levelIndirect -> e.g. 1 means indirect, 2 means doubly indirect, etc
void processIndirect(int levelIndirect, __u32 blockNum, __u32 inodeNum, __u32 logicalOffset)
{
    __u32 numPointers = 256;        // given in piazza -> dinit will hold 256 pointers to indirect blocks, etc
    
    // pread entire block
    __u32 *blockContents = malloc(blocksize * sizeof(__u32));
    int offset = (blockNum * blocksize);
    if(pread(fd, blockContents, numPointers * sizeof(__u32), offset) < 0)
    {
        fprintf(stderr, "Failed to read indirect block reference\n");
        exit(2);
    }

    // go thru each pointer
    for(__u32 i = 0; i < numPointers; i++)
    {
        // if that pointer holds a value
        if(blockContents[i])
        {
            printf("INDIRECT,%u,%u,%u,%u,%u\n",
                inodeNum,
                levelIndirect,
                logicalOffset,
                blockNum,
                blockContents[i]
            );
            
            // if still more than one pointer away, call recursively
            if(levelIndirect > 1)  
            {
                processIndirect(levelIndirect - 1, blockContents[i], inodeNum, logicalOffset);
            }
            
        }

        // calculate the offsets -> depends on level (as shown in piazza)
        if(levelIndirect == 1)
            logicalOffset++;
        else if(levelIndirect == 2)
            logicalOffset += 256;
        else if(levelIndirect == 3)
            logicalOffset += 256*256;
    }

    free(blockContents);
}



// Helper function to format inode times
void formattime(char* buf, __u32 time)
{
    time_t temp = time;
    struct tm* timestruct = gmtime(&temp);
    strftime(buf, 50, "%m/%d/%y %H:%M:%S", timestruct);
}


int main(int argc, char** argv)
{
    // ********** PROCESS ARGUMENTS **********

    if(argc != 2)
    {
        fprintf(stderr, "Incorrect arguments\n");
        exit(1);
    }
    if(argv[1][0] == '-')
    {
        fprintf(stderr, "Incorrect arguments\n");
        exit(1);
    }
    __u32 i;

    // Open disk image
    char* fs_name = argv[1];
    fd = open(fs_name, O_RDONLY);
    if(fd < 0)
    {
        fprintf(stderr, "Failed to open file\n");
        exit(2);
    }



    // *********** SUPERBLOCK PROCESSING ***********

    if(pread(fd, &superblock, sizeof(struct ext2_super_block), 1024) < 0)
    {
        fprintf(stderr, "Failed to read superblock\n");
        exit(2);
    }
    if(superblock.s_magic != EXT2_SUPER_MAGIC)
    {
        fprintf(stderr, "Superblock not found\n");
        exit(2);
    }
    blocksize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;   

    printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", 
    superblock.s_blocks_count,
    superblock.s_inodes_count,
    blocksize,
    superblock.s_inode_size,
    superblock.s_blocks_per_group,
    superblock.s_inodes_per_group,
    superblock.s_first_ino
    );

    
    

    // *********** GROUP DESCRIPTOR PROCESSING, ASSUME ONLY 1 GROUP ***********

    if(pread(fd, &groupdesc, sizeof(struct ext2_group_desc), 2*blocksize) < 0)
    {
        fprintf(stderr, "Failed to read group descriptor table\n");
        exit(2);
    }
    printf("GROUP,0,%u,%u,%u,%u,%u,%u,%u\n",
    superblock.s_blocks_count,
    superblock.s_inodes_count,
    groupdesc.bg_free_blocks_count,
    groupdesc.bg_free_inodes_count,
    groupdesc.bg_block_bitmap,
    groupdesc.bg_inode_bitmap,
    groupdesc.bg_inode_table
    );



    // *********** BLOCK BITMAP PROCESSING ***********

    __u32 num_block_bitmap_bytes = superblock.s_blocks_count / 8;
    blockbitmapbuf = (char*) malloc(sizeof(char)*num_block_bitmap_bytes);

    if(pread(fd, blockbitmapbuf, num_block_bitmap_bytes, groupdesc.bg_block_bitmap*blocksize) < 0)
    {
        fprintf(stderr, "Failed to read block bitmap\n");
        exit(2);
    }

    for(i = 0; i < num_block_bitmap_bytes; i++)
    {
        if(!(blockbitmapbuf[i] & 0x01 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 1);
        if(!(blockbitmapbuf[i] & 0x02 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 2);
        if(!(blockbitmapbuf[i] & 0x04 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 3);
        if(!(blockbitmapbuf[i] & 0x08 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 4);
        if(!(blockbitmapbuf[i] & 0x10 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 5);
        if(!(blockbitmapbuf[i] & 0x20 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 6);
        if(!(blockbitmapbuf[i] & 0x40 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 7);
        if(!(blockbitmapbuf[i] & 0x80 ? 1 : 0))
            printf("BFREE,%d\n", i*8 + 8);
    }





    // *********** INODE BITMAP PROCRESSING ***********

    __u32 num_inode_bitmap_bytes = superblock.s_inodes_count / 8;
    inodebitmapbuf = (char*) malloc(num_inode_bitmap_bytes*sizeof(char));
    if(pread(fd, inodebitmapbuf, num_inode_bitmap_bytes, groupdesc.bg_inode_bitmap * blocksize) < 0)
    {
        fprintf(stderr, "Error: failed to read inode bitmap\n");      
        exit(2);
    }
    // loop thru each char in string (each byte) and do bitwise manipulation to get each byte
    int mask;
    for(i = 0; i < num_inode_bitmap_bytes; i++)
    {
        char c = inodebitmapbuf[i];
        mask = 0x01;

        for(int j = 0; j < 8; j++)    
        {
            // isolate each individual bit
            mask = mask << j;      
            int bit = c & mask;

            if(!bit)        // bit is 0 -> free
            {
                int numInode = (i * 8) + j + 1;     // inodes start at 1
                printf("IFREE,%u\n", numInode);
            }
        }    
    }




    // *********** INODE TABLE PROCESSING ***********
    
    for(i = 1; i < superblock.s_inodes_count+1; i++)
    {
        struct ext2_inode inode;
        int offset = (groupdesc.bg_inode_table*blocksize)+((i-1)*superblock.s_inode_size);
        if(pread(fd, &inode, superblock.s_inode_size, offset) < 0)
        {
            fprintf(stderr, "Failed to read inode table entry\n");
            exit(2);
        }
        if(inode.i_mode && inode.i_links_count)
        {
            // Determine filetypes
            char filetype;
            if(S_ISLNK(inode.i_mode))
                filetype = 's';
            else if(S_ISREG(inode.i_mode))
                filetype = 'f';
            else if(S_ISDIR(inode.i_mode))
                filetype = 'd';
            else   
                filetype = '?';

            // Format times
            char ctimebuf[50];
            formattime(ctimebuf, inode.i_ctime);

            char mtimebuf[50];
            formattime(mtimebuf, inode.i_mtime);

            char atimebuf[50];
            formattime(atimebuf, inode.i_atime);

            printf("INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", 
            i, 
            filetype, 
            inode.i_mode & 0xFFF, 
            inode.i_uid, 
            inode.i_gid,
            inode.i_links_count,
            ctimebuf,
            mtimebuf,
            atimebuf,
            inode.i_size,
            inode.i_blocks
            );

            // next 15 fields block addresses -> only print if type 'f' or type 'd' OR 's' with filesize > 60
            if(filetype == 'f' || filetype == 'd' || (filetype == 's' && inode.i_size > 60))
            {
                for(int j = 0; j < 15; j++)
                {
                    printf(",%u", inode.i_block[j]);
                }
            }
            printf("\n");




            // *** DIRECTORY ENTRIES ***

            if(filetype == 'd')
            {
                // note: given in spec that 12 direct entries (first 12)
                for(__u32 j = 0; j < 12; j++)
                {
                    // if there is directory entry at block, process it
                    if(inode.i_block[j])
                    {
                        // increment by each directory entry's length until went through the whole block
                        __u32 logicalOffset = 0;
                        while(logicalOffset < blocksize)
                        {
                            struct ext2_dir_entry dirEntry;

                            int offsetDirEntry = (inode.i_block[j] * blocksize) + logicalOffset;
                            if(pread(fd, &dirEntry, sizeof(struct ext2_dir_entry), offsetDirEntry) < 0)
                            {
                                fprintf(stderr, "Failed to read directory entry\n");
                                exit(2);
                            }

                            // if there is an inode, make an entry
                            if(dirEntry.inode)
                            {
                                printf("DIRENT,%u,%u,%u,%u,%u,'%s'\n",
                                    i,
                                    logicalOffset,
                                    dirEntry.inode,
                                    dirEntry.rec_len,
                                    dirEntry.name_len,
                                    dirEntry.name
                                );
                            }

                            // increment by length of directory entry
                            logicalOffset += dirEntry.rec_len;
                        }   

                    }
                }
            }

            
            // *** INDIRECT BLOCK REFERENCES ***

            // for each, if there are contents at index 12, 13, 14, go thru respective indirect block reference
            // note logical offsets are given in piazza  @736

            
            // indirect block
            if(inode.i_block[12])
            {
                processIndirect(1, inode.i_block[12], i, 12);
            }

            // doubly indirect block
            if(inode.i_block[13])
            {
                processIndirect(2, inode.i_block[13], i, 268);
            }

            // triply indirect block
            if(inode.i_block[14])
            {
                processIndirect(3, inode.i_block[14], i, 268 + (256*256));
            }
            
        }
    }
    
    free(blockbitmapbuf);
    free(inodebitmapbuf);
    close(fd);
    exit(0);
}