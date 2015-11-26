#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "minls.h"
filesystem *fileSystem;

int main(int argc, char *argv[]) {
   int verbose = 0;
   int i = 1; 
   int partition = 0;
   int subpartition = 0;
   char* imagefile = 0;
   char* path = 0;

   if(argc == 1 || (argv[i][0] == '-' && argv[i][1] == 'h')) {
      printf(
         "usage: minls  [ -v ] [ -p num [-s num ] ] imagefile [ path ]\n"
         "Options:\n"     
         "-p part    --- select partition for filesystem (default: none)\n"
         "-s sub     --- select subpartition for filesystem (default: none)\n"
         "-h help    --- print usage information and exit\n"
         "-v verbose --- increase verbosity level\n"
         );
      return 0;
   }


   if(argv[i][0] == '-' && argv[i][1] == 'v') {
      printf("Verbose\n");
      verbose = 1;
      i++;
   }

   if(i < argc && argv[i][0] == '-' && argv[i][1] == 'p') {
      if(i + 2 > argc) {
         fprintf(stderr, "Not enough arguments for %s flag\n", argv[i]);
         return -1;
      }
      i++;
      partition = atoi(argv[i++]);
      fileSystem->part = partition;
      printf("Partition: %d\n", partition);
      if(i < argc && argv[i][0] == '-' && argv[i][1] == 's') {
         printf("Getting here\n");
         if(i + 2 > argc) {
            fprintf(stderr, "Not enough arguments for %s flag\n", argv[i]);
            return -1;
         }
         i++; 
         subpartition = atoi(argv[i++]);
         fileSystem->subPart = subpartition;
         printf("Subpartition: %d\n", subpartition);
      }
      else {
         fileSystem->subPart = NULL;
      }
   }
   else {
      fileSystem->part = NULL;
   }

   if(i < argc) {
      imagefile = argv[i++];
      /*open file image and set the FILE pointer to the field*/
      fileSystem->imageFile = fopen(imagefile, "rb");
      if (fileSystem->imageFile != 1) {
         fprintf(stderr, "Cannot open the image file %s\n", fileSystem->imageFile);
      }
      fprintf(stderr, "Imagefile is %s\n", imagefile);
   }

   if(i < argc) {
      path = argv[i++];
      fileSystem->path = path;
      fprintf(stderr, "Path is %s\n", path);
   }
   /*Set the bootblock*/
   fileSystem->bootblock = 0;
   /*Partition => call findPartition() for part && subpart (disk starts at first
    sector of part for the subpart but everything else is the same)
    Superblock
    get inode
    print listing
    close file*/
   if (fileSystem->part != NULL) {
      findPartition(fileSystem->part);
      if (fileSystem->subPart != NULL;) {
         findPartition(fileSystem->subPart);
      }
   }
   return 0;
}

void findPartition(int partition) {
   /*
    1. check if byte 510 == PMAGIC510 && byte 511 == PMAGIC511
        to see if the partition table is valid before proceeding
    2. Go to address PTABLE_OFFSET in the disk to get to partition table
    3. check the type says it is a MINIX partition
    4. We want to go to the first sector => lfirst * SECTOR_SIZE*/
   
   partition table[4];
   /*fseek to byte 510
    fread(set value for byte 510, 1, 1, fileSystem->imageFile);
    fseek to byte 511
    fread(set value for byte 511, 1, 1, fileSystem->imageFile);
   if(byte 510 != PMAGIC510 && byte 511 != PMAGIC511) {
      fprintf(stderr, "Not a valid partition table\n");
      return;
   }*/
   fseek(fileSystem->imageFile, PTABLE_OFFSET, SEEK_SET);
   fread(table, sizeof(partition), 4, fileSystem->imageFile);
   if (table[partition].type != MINIXPART) {
      fprintf(stderr, "Not a MINIX partition table\n");
      return;
   }
   if (verbose) {
      /*Print the partition table*/
   }
   /*Set the bootblock to the first sector => lfirst sector in the case
    that there is subpart or for when you want to find the superblock*/
   fileSystem->bootblock = table[partition].lFirst * SECTOR_SIZE;
   
}

void findSuperblock() {
   /*Check the magic number to make sure it is a minix filesystem
    Set the zonesize = superblock->blocksize <<superblock->log_zone_size*/
   fseek(fileSystem->imageFile, BLOCK_SIZE + fileSystem->bootblock, SEEK_SET);
   fread(fileSystem->superBlock, sizeof(superBlock), 1, fileSystem->imageFile);
   
   if (fileSystem->superBlock->magic != MIN_MAGIC) {
      fprintf(stderr, "Not a MINIX filesystem. Incorrect magic number\n");
      return;
   }
   if (verbose) {
      /*Print the superblock*/
   }
}

/*Using the path, figure out how to get the innode
 check if the innode is the directory.*/








