#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include "minls.h"


filesystem fileSys;
int verbose = 0;

int main(int argc, char *argv[]) {
   extern char *optarg;
   extern int optind;
   char* imagefile = 0;
   int c = getopt(argc, argv, "vp:s:h");
   int sflag = 0, pflag = 0;
   int isPart = 0;
   fileSys.part = -1;
   fileSys.subPart = -1;
   fileSys.path = 0;
   while (c != -1) {
      switch (c) {
         case 'v':
            verbose = 1;
            break;
         case 'p':
            isPart = 1;
            pflag = 1;
            fileSys.part = atoi(optarg);
            break;
         case 's':
            sflag = 1;
            fileSys.subPart = atoi(optarg);
            break;
         case 'h':
            fprintf(stderr,
             "usage: minls  [ -v ] [ -p num [-s num ] ] imagefile [ path ]\n"
             "Options:\n"
             "-p part    --- select partition for filesystem (default: none)\n"
          "-s sub     --- select subpartition for filesystem (default: none)\n"
             "-h help    --- print usage information and exit\n"
             "-v verbose --- increase verbosity level\n"
                   );
            exit(EXIT_FAILURE);
         default:
            fprintf(stderr,
             "usage: minls  [ -v ] [ -p num [-s num ] ] imagefile [ path ]\n"
             "Options:\n"
             "-p part    --- select partition for filesystem (default: none)\n"
          "-s sub     --- select subpartition for filesystem (default: none)\n"
             "-h help    --- print usage information and exit\n"
             "-v verbose --- increase verbosity level\n"
                    );
            exit(EXIT_FAILURE);
            
      }
      if (pflag == 1 && ((optind+1) > argc)) {
         fprintf(stderr, "Not enough arguments for -p flag\n");
         exit(EXIT_FAILURE);
      }
      if (sflag == 1 && isPart == 0) {
         fprintf(stderr, "Cannot have subpartition without partition.\n");
         exit(EXIT_FAILURE);
      }
      else if (sflag == 1 && ((optind+1) > argc)) {
         fprintf(stderr, "Not enough arguments for -s flag\n");
         exit(EXIT_FAILURE);
      }
      pflag = sflag = 0;
      c = getopt(argc, argv, "vp:s:h");
   }
   if (optind >= argc) {
      fprintf(stderr, "Need an imagefile.\n");
      exit(EXIT_FAILURE);
   }
   else {
      fileSys.imageFile = fopen(argv[optind++], "rb");
      if (fileSys.imageFile == NULL) {
         fprintf(stderr, "Cannot open the image file %s\n", imagefile);
         exit(EXIT_FAILURE);
      }
   }
   if (optind < argc) {
      fileSys.path = argv[optind];
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
   int i;
   uint8_t block[BLOCK_SIZE];
   partition table[4];
   partition *partitionTable = NULL;
   partition *partitionTablePrint = NULL;
   fseek(fileSys.imageFile, fileSys.bootblock, SEEK_SET);
   fread((void*)block, BLOCK_SIZE, 1, fileSys.imageFile);
   if (block[510] != PMAGIC510 || block[511] != PMAGIC511) {
      printf("Invalid partition table.\n");
      exit(EXIT_FAILURE);
   }
   fseek(fileSys.imageFile, (fileSys.bootblock) + PTABLE_OFFSET, SEEK_SET);
   fread((void*)table, sizeof(partition), 4, fileSys.imageFile);
   if (table->type != MINIXPART) {
      fprintf(stderr, "Not a MINIX partition table\n");
      exit(EXIT_FAILURE);
   }
   /*There has to be a better way to do this, but it will work for now*/
   partitionTable = (partition*)table;
   partitionTablePrint = partitionTable;
   if(verbose) {
      /*Print Partition Table*/
      printf("Partition table:\n");
      printf("----Start----      ------End-----\n");
      printf("Boot head  sec  cyl Type head  sec  cyl      First       Size\n");
      /*Loop through each of the possible 4 partitions*/
      for(i = 0; i < 4; i++) {
         printf("0x%2x", partitionTablePrint->bootind);
         printf("%5d", partitionTablePrint->start_head);
         printf("%5d", partitionTablePrint->start_sec);
         printf("%5d", partitionTablePrint->start_cyl);
         printf(" ");
         printf("0x%2x", partitionTablePrint->type);
         printf("%5d", partitionTablePrint->end_head);
         printf("%5d", partitionTablePrint->end_sec);
         printf("%5d", partitionTablePrint->end_cyl);
         printf("%11d", partitionTablePrint->lFirst);
         printf("%11d\n", partitionTablePrint->size);
         partitionTablePrint++;
      }
   }
   partitionTable = partitionTable + partitionNum;
   /*Set the bootblock to the first sector => lfirst sector in the case
    that there is subpart or for when you want to find the superblock*/
   fileSys.bootblock = partitionTable->lFirst * SECTOR_SIZE;
   
}

void findSuperBlock() {
  superblock super;

   /*Check the magic number to make sure it is a minix filesystem
    Set the fileSys.zonesize = 
    superblock->fileSys.blocksize <<superblock->log_zone_size*/
   fseek(fileSys.imageFile, BLOCK_SIZE + fileSys.bootblock, SEEK_SET);
   fread(&super, sizeof(superblock), 1, fileSys.imageFile);
   fileSys.blocksize = super.blocksize;
   fileSys.zonesize = fileSys.blocksize << super.log_zone_size;

   fileSys.super = super;
   
   if (super.magic != MIN_MAGIC) {
      fprintf(stderr, "Not a MINIX filesystem. Incorrect magic number\n");
      exit(EXIT_FAILURE);
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
         "\tfileSys.blocksize%11u\n"
         "\tsubversion%10u\n"
         , super.ninodes, super.i_blocks, super.z_blocks
         , super.firstdata, super.log_zone_size
         , fileSys.zonesize , super.max_file
         , super.magic, super.zones, fileSys.blocksize, super.subversion);
   }


}

void findPath() {
   inode node;
   printNode newNode;
   time_t aTime;
   time_t mTime;
   time_t cTime;
   char* keepPath = fileSys.path;

   fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize +
         fileSys.blocksize + fileSys.blocksize * fileSys.super.i_blocks +
         fileSys.blocksize * fileSys.super.z_blocks, SEEK_SET);
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

   newNode = findPathToInode(node);

   //Check if it's a file. This is special and gets printed differently
   if(!IS_DIRECTORY(newNode.mode)) {
      printPermissions(newNode.mode);
      printf("%10u ", newNode.size);
      printf("%s\n", keepPath);
      return;
   }
   else {
      //If keepPath has a value, aka not null, print it
      if(keepPath)
         printf("%s:\n", keepPath);
      else//Else just print the home
         printf("/:\n");
   }

   printFile(newNode);
}

//Recursively finds path to correct file and returns a printing Node
printNode findPathToInode(inode node) {
   uint32_t num_files;
   fileent file;
   printNode newNode; 
   uint32_t zoneNum = node.zone[0];
   // int i = 0;
   int j = 0;

   num_files = node.size / FILEENT_SIZE;

   fseek(fileSys.imageFile, zoneNum * fileSys.zonesize, SEEK_SET);
   fread(&file, sizeof(fileent), 1, fileSys.imageFile);

   if(fileSys.path != 0) {
      for(j = 0; j < num_files; j++) {
         fseek(fileSys.imageFile, fileSys.bootblock + zoneNum *
               fileSys.zonesize + FILEENT_SIZE * j, SEEK_SET);
         fread(&file, sizeof(fileent), 1, fileSys.imageFile);

         if(file.ino == 0) 
            continue;

         if(fileCmp(file.name)) {

            fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize +
                  fileSys.blocksize + fileSys.blocksize *
                  fileSys.super.i_blocks + fileSys.blocksize *
                  fileSys.super.z_blocks + sizeof(inode) * (file.ino - 1),
                  SEEK_SET);
            fread(&node, sizeof(inode), 1, fileSys.imageFile);
            if(*fileSys.path != '\0') {
               newNode = findPathToInode(node);

               if(*newNode.name == '\0') {
                  memcpy(newNode.name, file.name, DIRSIZ);
               }

               return newNode;
            }
         }  
      }
   }
   newNode.mode = node.mode;
   newNode.size = node.size;
   /*This could be an issue...*/
   memcpy(newNode.zone, node.zone,sizeof(uint32_t) * DIRECT_ZONES);
   newNode.indirect = node.indirect;
   newNode.two_indirect = node.two_indirect;
   *newNode.name = '\0';

   return newNode;
}

void printFile(printNode node) {
   uint32_t num_files;
   fileent file;
   inode newNode; 
   int i = 0;

   num_files = node.size / FILEENT_SIZE;

   for(i = 0; i < num_files; i++) {
      fseek(fileSys.imageFile, fileSys.bootblock + node.zone[0] *
            fileSys.zonesize + FILEENT_SIZE * i, SEEK_SET);
      fread(&file, sizeof(fileent), 1, fileSys.imageFile);

      if(file.ino == 0) 
         continue;

      fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize +
            fileSys.blocksize + fileSys.blocksize * fileSys.super.i_blocks +
            fileSys.blocksize * fileSys.super.z_blocks + sizeof(inode) *
            (file.ino - 1), SEEK_SET);
      fread(&newNode, sizeof(inode), 1, fileSys.imageFile);
      

      printPermissions(newNode.mode);
      printf("%10u ", newNode.size);
      printFileName(file.name);
      printf("\n");
   }

}

int fileCmp(char* cmp) {
   int i = DIRSIZ; 
   char* keepPath = fileSys.path;

   //Chew up the \/
   if(*fileSys.path == '/')
      fileSys.path++;

   //If reach i = 0, they were max size and the same. 
   //Else if they both equal null, they are the same
   //ELse if one path is '/' and the other is null
   while(i-- && (*fileSys.path != '\0' && *cmp != '\0')) {
      if(*fileSys.path++ != *cmp++) {
         fileSys.path = keepPath;
         return 0;
      }

      if(*fileSys.path == '/' && *cmp == '\0') {
         return 1;
      }
      else if(*fileSys.path == '/'){
         //This means that the file was a subfile of the path
         fileSys.path = keepPath;
         return 0;
      }
   } 
   return 1;
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