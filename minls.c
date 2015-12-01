#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include "minls.h"


filesystem fileSys;
int verbose = 0;
uint32_t zonesize;
uint32_t blocksize;


int main(int argc, char *argv[]) {
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
      fileSys.part = partition;
      printf("Partition: %d\n", partition);
      if(i < argc && argv[i][0] == '-' && argv[i][1] == 's') {
         printf("Getting here\n");
         if(i + 2 > argc) {
            fprintf(stderr, "Not enough arguments for %s flag\n", argv[i]);
            return -1;
         }
         i++; 
         subpartition = atoi(argv[i++]);
         fileSys.subPart = subpartition;
         printf("Subpartition: %d\n", subpartition);
      }
      else {
         fileSys.subPart = -1;
      }
   }
   else {
      fileSys.part = -1;
   }

   if(i < argc) {
      imagefile = argv[i++];
      /*open file image and set the FILE pointer to the field*/
      fileSys.imageFile = fopen(imagefile, "rb");
      if (fileSys.imageFile == NULL) {
         fprintf(stderr, "Cannot open the image file %s\n", imagefile);
      }
   }

   if(i < argc) {
      path = argv[i++];
      fileSys.path = path;
      fprintf(stderr, "Path is %s\n", path);
   }
   /*Set the bootblock*/
   fileSys.bootblock = 0;
   /*Partition => call findPartition() for part && subpart (disk starts at first
    sector of part for the subpart but everything else is the same)
    Superblock
    get inode
    print listing
    close file*/
   if (fileSys.part > -1) {
      findPartition(fileSys.part);
       if (fileSys.subPart > -1) {
          findPartition(fileSys.subPart);
       }
   }
   findSuperBlock();
   findPath();

   return 0;
}

void findPartition(int partitionNum) {
   /*
    1. check if byte 510 == PMAGIC510 && byte 511 == PMAGIC511
    to see if the partition table is valid before proceeding
    2. Go to address PTABLE_OFFSET in the disk to get to partition table
    3. check the type says it is a MINIX partition
    4. We want to go to the first sector => lfirst * SECTOR_SIZE*/
   uint8_t block[BLOCK_SIZE];
   partition table[4];
   partition *partitionTable = NULL;
   fseek(fileSys.imageFile, fileSys.bootblock, SEEK_SET);
   fread((void*)block, BLOCK_SIZE, 1, fileSys.imageFile);
   if (block[510] != PMAGIC510 || block[511] != PMAGIC511) {
      printf("Invalid partition table.\n");
      return;
   }
   fseek(fileSys.imageFile, (fileSys.bootblock) + PTABLE_OFFSET, SEEK_SET);
   fread((void*)table, sizeof(partition), 4, fileSys.imageFile);
   if (table->type != MINIXPART) {
      fprintf(stderr, "Not a MINIX partition table\n");
      return;
   }
   /*There has to be a better way to do this, but it will work for now*/
   partitionTable = (partition*)table;
   partitionTable = partitionTable + partitionNum;
   /*Set the bootblock to the first sector => lfirst sector in the case
    that there is subpart or for when you want to find the superblock*/
   fileSys.bootblock = partitionTable->lFirst * SECTOR_SIZE;
   
}

void findSuperBlock() {
  superblock super;

   /*Check the magic number to make sure it is a minix filesystem
    Set the zonesize = superblock->blocksize <<superblock->log_zone_size*/
   fseek(fileSys.imageFile, BLOCK_SIZE + fileSys.bootblock, SEEK_SET);
   fread(&super, sizeof(superblock), 1, fileSys.imageFile);
   blocksize = super.blocksize;
   zonesize = blocksize << super.log_zone_size;

   fileSys.super = super;
   
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
         "\tlog_zone_size%7d (zone size: %d)\n"
         "\tmax_file%12u\n"
         "\tmagic         0x%4x\n"
         "\tzones%15u\n"
         "\tblocksize%11u\n"
         "\tsubversion%10u\n"
         , super.ninodes, super.i_blocks, super.z_blocks
         , super.firstdata, super.log_zone_size
         , zonesize , super.max_file
         , super.magic, super.zones, blocksize, super.subversion);
   }


}

void findPath() {
   inode node;
   fileent file;
   time_t aTime;
   time_t mTime;
   time_t cTime;

   fseek(fileSys.imageFile, fileSys.bootblock + blocksize + blocksize + 
      blocksize * fileSys.super.i_blocks + blocksize * fileSys.super.z_blocks
      , SEEK_SET);
   fread(&node, sizeof(inode), 1, fileSys.imageFile);

   if(verbose) {
      aTime = node.atime;
      mTime = node.mtime;
      cTime = node.ctime;
      printf(
         "File inode:\n"
         "\tuint16_t mode 0x%9x  (", node.mode);
      printPermissions(node.mode);

      printf(
         ")\n\tuint16_t links %10hu\n"
         "\tuint16_t uid %12hu\n"
         "\tuint16_t gid %12hu\n"
         "\tuint32_t size %11u\n"
         "\tuint32_t atime %8u %s"
         "\tuint32_t mtime %8u %s"
         "\tuint32_t ctime %8u %s"
         , node.links, node.uid, node.gid, node.size
         , node.atime, ctime(&aTime), node.mtime, ctime(&mTime)
         , node.ctime, ctime(&cTime)
         );

      printf(
         "\tDirect zones:\n"
         "\t\t  zone[0]  = %8u\n"
         "\t\t  zone[1]  = %8u\n"
         "\t\t  zone[2]  = %8u\n"
         "\t\t  zone[3]  = %8u\n"
         "\t\t  zone[4]  = %8u\n"
         "\t\t  zone[5]  = %8u\n"
         "\t\t  zone[6]  = %8u\n"
         "\tuint32_t    indirect %8u\n"
         "\tuint32_t    double   %8u\n"
         , node.zone[0], node.zone[1], node.zone[2]
         , node.zone[3], node.zone[4], node.zone[5]
         , node.zone[6], node.indirect, node.two_indirect
         );
   }

   fseek(fileSys.imageFile, node.zone[0] * zonesize, SEEK_SET);
   fread(&file, sizeof(fileent), 1, fileSys.imageFile);
   printf("/:\n");
   printFile(fileSys.super, node.zone[0], node.size);
}

void printFile(superblock super, uint32_t zone, uint32_t inode_size) {
   uint32_t num_files;
   fileent file;
   inode node; 
   int i = 0;

   num_files = inode_size / FILEENT_SIZE;

   for(i = 0; i < num_files; i++) {
      fseek(fileSys.imageFile, fileSys.bootblock + zone * zonesize + FILEENT_SIZE * i, SEEK_SET);
      fread(&file, sizeof(fileent), 1, fileSys.imageFile);

      if(file.ino == 0) 
         continue;

      fseek(fileSys.imageFile, fileSys.bootblock + blocksize + blocksize + blocksize * super.i_blocks + blocksize * super.z_blocks + sizeof(inode) * (file.ino - 1), SEEK_SET);
      fread(&node, sizeof(inode), 1, fileSys.imageFile);
      

      printPermissions(node.mode);
      printf("%10u ", node.size);
      printFileName(file.name);
      printf("\n");
   }

}

void printFileName(char* fileName) {
   int i = DIRSIZ; 

   while(i-- && *fileName != '\0') {
      putchar(*fileName++);
   }
}

void printPermissions(uint16_t mode) {
   int i = PERMISSION_MASK;
   int j = 3;

   if(IS_DIRECTORY(mode))
      putchar('d');
   else 
      putchar('-');

   while(j--) {
      if(mode & i)
         putchar('r');
      else 
         putchar('-');

      i >>= 1;

      if(mode & i)
         putchar('w');
      else 
         putchar('-');

      i >>= 1;

      if(mode & i)
         putchar('x');
      else 
         putchar('-');      

      i >>= 1;
   }


}

/*Using the path, figure out how to get the innode
 check if the innode is the directory.*/
