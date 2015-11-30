#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "minls.h"

filesystem stuff;
filesystem *fileSys;
int verbose = 1;

int main(int argc, char *argv[]) {
   int i = 1; 
   int partition = 0;
   int subpartition = 0;
   char* imagefile = 0;
   char* path = 0;

   fileSys = &stuff;

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
      fileSys->part = partition;
      printf("Partition: %d\n", partition);
      if(i < argc && argv[i][0] == '-' && argv[i][1] == 's') {
         printf("Getting here\n");
         if(i + 2 > argc) {
            fprintf(stderr, "Not enough arguments for %s flag\n", argv[i]);
            return -1;
         }
         i++; 
         subpartition = atoi(argv[i++]);
         fileSys->subPart = subpartition;
         printf("Subpartition: %d\n", subpartition);
      }
      else {
         fileSys->subPart = 0;
      }
   }
   else {
      fileSys->part = 0;
   }

   if(i < argc) {
      imagefile = argv[i++];
      /*open file image and set the FILE pointer to the field*/
      fileSys->imageFile = fopen(imagefile, "rb");
      if (fileSys->imageFile == NULL) {
         fprintf(stderr, "Cannot open the image file %s\n", imagefile);
      }
   }

   if(i < argc) {
      path = argv[i++];
      fileSys->path = path;
      fprintf(stderr, "Path is %s\n", path);
   }
   /*Set the bootblock*/
   fileSys->bootblock = 0;
   /*Partition => call findPartition() for part && subpart (disk starts at first
    sector of part for the subpart but everything else is the same)
    Superblock
    get inode
    print listing
    close file*/
   if (fileSys->part >= 0) {
      findPartition(fileSys->part);
      // if (fileSys->subPart < 0) {
      //    findPartition(fileSys->subPart);
      // }
   }
   findSuperBlock();
   return 0;
}

void findPartition(int partitionNum) {
   /*
    1. check if byte 510 == PMAGIC510 && byte 511 == PMAGIC511
        to see if the partition table is valid before proceeding
    2. Go to address PTABLE_OFFSET in the disk to get to partition table
    3. check the type says it is a MINIX partition
    4. We want to go to the first sector => lfirst * SECTOR_SIZE*/
   
   partition table[4];
   /*fseek to byte 510
    fread(set value for byte 510, 1, 1, fileSys->imageFile);
    fseek to byte 511
    fread(set value for byte 511, 1, 1, fileSys->imageFile);
   if(byte 510 != PMAGIC510 && byte 511 != PMAGIC511) {
      fprintf(stderr, "Not a valid partition table\n");
      return;
   }*/
   fseek(fileSys->imageFile, PTABLE_OFFSET, SEEK_SET);
   fread(table, sizeof(partition), 4, fileSys->imageFile);
   if (table[partitionNum].type != MINIXPART) {
      //This is currently broken, maybe we don't know where to look?
      fprintf(stderr, "Not a MINIX partition table\n");
      return;
   }
   
   /*Set the bootblock to the first sector => lfirst sector in the case
    that there is subpart or for when you want to find the superblock*/
   printf("Setting bootblock\n");
   fileSys->bootblock = table[partitionNum].lFirst * SECTOR_SIZE;
   
}

void findSuperBlock() {
  superblock super;
  inode node; 
   /*Check the magic number to make sure it is a minix filesystem
    Set the zonesize = superblock->blocksize <<superblock->log_zone_size*/
   fseek(fileSys->imageFile, BLOCK_SIZE + fileSys->bootblock, SEEK_SET);
   fread(&super, sizeof(superblock), 1, fileSys->imageFile);
   
   if (super.magic != MIN_MAGIC) {
      fprintf(stderr, "Not a MINIX filesystem. Incorrect magic number\n");
      return;
   }
   if (verbose) {
      /*Print the superblock*/
      //DOn't know how to get the zone size??????????
      printf(
         "Superblock Contents:\nStored Fields:\n"
         "\tninodes%13u\n"
         "\ti_blocks%12d\n"
         "\tz_blocks%12d\n"
         "\tfirstdata%11d\n"
         "\tlog_zone_size%7d\n"
         "\tmax_file%12u\n"
         "\tmagic         0x%4x\n"
         "\tzones%15u\n"
         "\tblocksize%11u\n"
         "\tsubversion%10u\n"
         , super.ninodes, super.i_blocks, super.z_blocks
         , super.firstdata, super.log_zone_size, super.max_file
         , super.magic, super.zones, super.blocksize, super.subversion);
   }

   fseek(fileSys->imageFile, fileSys->bootblock + BLOCK_SIZE + BLOCK_SIZE + super.blocksize * super.z_blocks + super.blocksize * super.i_blocks, SEEK_SET);
   fread(&node, sizeof(inode), 1, fileSys->imageFile);

   printf("%u %u %u\n", (unsigned int)sizeof(inode), super.blocksize * super.z_blocks, super.blocksize * super.i_blocks);

   printf("num blocks needed: %u zone size: %u\n",  (super.blocksize * 64) / super.i_blocks, super.blocksize << super.log_zone_size);
   printf("mode %u links %u\n", node.mode, node.links);

}

/*Using the path, figure out how to get the innode
 check if the innode is the directory.*/
