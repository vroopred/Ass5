#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include "lib.h"

/*Globals for filesystem variable and verbose flag*/
filesystem fileSys;
int verbose;

/*Finds the partition table and indicated partition. Sets the values
 and moves the bootblock field which is used to indicate where the 
 disk starts to the correct location based on the partitions info*/
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
   /*check if byte 510 == PMAGIC510 && byte 511 == PMAGIC511*/
   fseek(fileSys.imageFile, fileSys.bootblock, SEEK_SET);
   fread((void*)block, BLOCK_SIZE, 1, fileSys.imageFile);
   if (block[510] != PMAGIC510 || block[511] != PMAGIC511) {
      printf("Invalid partition table.\n");
      exit(EXIT_FAILURE);
   }
   fseek(fileSys.imageFile, (fileSys.bootblock) + PTABLE_OFFSET, SEEK_SET);
   fread((void*)table, sizeof(partition), 4, fileSys.imageFile);
   /*Check the type of the partition table to make sure it is a minix
    partition table*/
   if (table->type != MINIXPART) {
      fprintf(stderr, "Not a MINIX partition table\n");
      exit(EXIT_FAILURE);
   }
   partitionTable = (partition*)table;
   partitionTablePrint = partitionTable;
   if(verbose) {
      /*Print Partition Table*/
      printf("Partition table:\n");
      printf("----Start----      ------End-----\n");
      printf("Boot head  sec  cyl Type head  sec  cyl      First      Size\n");
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
   /*Set partitionTable to the partition part*/
   partitionTable = partitionTable + partitionNum;
   /*Set the bootblock to the first sector => lfirst sector in the case
    that there is subpart or for when you want to find the superblock*/
   fileSys.bootblock = partitionTable->lFirst * SECTOR_SIZE;
   
}

/*Finds the superblock and sets the fields*/
void findSuperBlock() {
   superblock super;
   fseek(fileSys.imageFile, BLOCK_SIZE + fileSys.bootblock, SEEK_SET);
   fread(&super, sizeof(superblock), 1, fileSys.imageFile);
   fileSys.blocksize = super.blocksize;
   /*Set the zonesize*/
   fileSys.zonesize = fileSys.blocksize << super.log_zone_size;
   
   fileSys.super = super;
   /*Check the magic number to make sure it is a minix filesystem*/
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
             "\tblocksize%11u\n"
             "\tsubversion%10u\n"
             "Computed Fields:\n"
             "\tversion            3\n"
             "\tfirstImap          2\n"
             "\tfirstZmap          3\n"
             "\tfirstIblock        4\n"
             "\tzonesize        4096\n"
             "\tptrs_per_zone   1024\n"
             "\tino_per_block     64\n"
             "\twrongended         0\n"
             "\tfileent_size      64\n"
             "\tmax_filename      60\n"
             "\tent_per_zone      64\n"
             , super.ninodes, super.i_blocks, super.z_blocks
             , super.firstdata, super.log_zone_size
             , fileSys.zonesize , super.max_file
             , super.magic, super.zones, fileSys.blocksize, super.subversion);
   }
   
   
}

void findPath() {
   inode node;
   printNode newNode;
   char* keepPath = fileSys.path;
   
   fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize +
         fileSys.blocksize + fileSys.blocksize * fileSys.super.i_blocks +
         fileSys.blocksize * fileSys.super.z_blocks, SEEK_SET);
   fread(&node, sizeof(inode), 1, fileSys.imageFile);
   
   
   
   //Complicated attempt to print no path...
   //Due to time crunch
   if(!keepPath) {
      printf("/:\n");
      memcpy(&newNode, &node, sizeof(inode));
      *newNode.name = '\0';
      if(verbose)
         printVerbosePath(newNode);
      printFile(newNode);
      return;
   }
   
   newNode = searchZone(node);
   
   if(!newNode.found) {
      printf("File not found\n");
      exit(-1);
   }
   
   if(verbose)
      printVerbosePath(newNode);
   //Check if it's a file. This is special and gets printed differently
   if(!IS_DIRECTORY(newNode.mode)) {
      printPermissions(newNode.mode);
      printf("%10u ", newNode.size);
      printf("%s\n", keepPath);
      return;
   }
   else {
      printf("%s:\n", keepPath);
   }
   printFile(newNode);
}


void printVerbosePath(printNode node) {
   time_t aTime;
   time_t mTime;
   time_t cTime;
   aTime = node.atime;
   mTime = node.mtime;
   cTime = node.ctime;
   printf(
          "File inode:\n"
          "\tuint16_t mode      0x%4x  (", node.mode);
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


printNode searchZone(inode node) {
   int i, j;
   printNode retNode;
   uint32_t indirectZones[INDIRECT_ZONE];
   uint32_t dIndirectZones[INDIRECT_ZONE];
   // zone curZone;
   for(i = 0; i < DIRECT_ZONES; i++) {
      if(!node.zone[i])
         break;
      retNode = searchInode(node, node.zone[i]);
      if(retNode.found)
         return retNode;
   }
   
   if(node.indirect) {
      fseek(fileSys.imageFile, node.indirect * fileSys.zonesize, SEEK_SET);
      fread(&indirectZones,sizeof(uint32_t*),INDIRECT_ZONE,fileSys.imageFile);
      
      for (i = 0; i < INDIRECT_ZONE; i++){
         if(!indirectZones[i])
            break;
         retNode = searchInode(node, indirectZones[i]);
         if(retNode.found)
            return retNode;
      }
   }
   
   if(node.two_indirect) {
      fseek(fileSys.imageFile, node.two_indirect * fileSys.zonesize, SEEK_SET);
      fread(&indirectZones,sizeof(uint32_t*),INDIRECT_ZONE,fileSys.imageFile);
      
      for (i = 0; i < INDIRECT_ZONE; i++) {
         fseek(fileSys.imageFile,indirectZones[i] * fileSys.zonesize,
               SEEK_SET);
         fread(&dIndirectZones,sizeof(uint32_t*),INDIRECT_ZONE,
               fileSys.imageFile);
         for(j = 0; j < INDIRECT_ZONE; j++ ) {
            fseek(fileSys.imageFile, dIndirectZones[j] * fileSys.zonesize,
                  SEEK_SET);
            fread(&dIndirectZones,sizeof(uint32_t*),INDIRECT_ZONE,
                  fileSys.imageFile);
            if(!dIndirectZones[j])
               break;
            retNode = searchInode(node, indirectZones[j]);
            if(retNode.found)
               return retNode;
         }
      }
   }
   
   return retNode;
}

//Recursively finds path to correct file and returns a printing Node
printNode searchInode(inode node, uint32_t zoneNum) {
   uint32_t num_files;
   fileent file;
   printNode newNode;
   // uint32_t zoneNum = node.zone[0];
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
            fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize
                  + fileSys.blocksize + fileSys.blocksize
                  * fileSys.super.i_blocks + fileSys.blocksize
                  * fileSys.super.z_blocks + sizeof(inode) *
                  (file.ino - 1), SEEK_SET);
            fread(&node, sizeof(inode), 1, fileSys.imageFile);
            if(*fileSys.path != '\0') {
               newNode = searchZone(node);
               
               if(*newNode.name == '\0') {
                  memcpy(newNode.name, file.name, DIRSIZ);
               }
               
               return newNode;
            }
            else {
               memcpy(&newNode, &node, sizeof(inode));
               *newNode.name = '\0';
               newNode.found = 1;
               return newNode;
            }
         }
      }
   }
   newNode.found = 0;
   
   return newNode;
}


void printFile(printNode node) {
   uint32_t num_files;
   fileent file;
   inode newNode;
   int i = 0;
   int j;
   uint32_t indirectZones[INDIRECT_ZONE];
   int filesRead = 0;
   int filesPerZone = fileSys.zonesize/FILEENT_SIZE;
   num_files = node.size / FILEENT_SIZE;
   
   for(j = 0; j < DIRECT_ZONES; j++) {
      for(i = 0; i < filesPerZone && filesRead < num_files; i++, filesRead++) {
         fseek(fileSys.imageFile, fileSys.bootblock + node.zone[j] *
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
   
   // printf("%d %d\n", filesRead, num_files);
   
   if(node.indirect) {
      fseek(fileSys.imageFile, node.indirect * fileSys.zonesize, SEEK_SET);
      fread(&indirectZones,sizeof(uint32_t*),INDIRECT_ZONE,fileSys.imageFile);
      
      for (i = 0; i < INDIRECT_ZONE; i++){
         for(j = 0; j < filesPerZone && filesRead < num_files;j++,filesRead++){
            fseek(fileSys.imageFile, fileSys.bootblock + indirectZones[i] *
                  fileSys.zonesize + FILEENT_SIZE * j, SEEK_SET);
            fread(&file, sizeof(fileent), 1, fileSys.imageFile);
            
            if(file.ino == 0)
               continue;
            
            if(file.ino == 0)
               continue;
            
            fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize +
                  fileSys.blocksize + fileSys.blocksize*fileSys.super.i_blocks+
                  fileSys.blocksize * fileSys.super.z_blocks + sizeof(inode) *
                  (file.ino - 1), SEEK_SET);
            fread(&newNode, sizeof(inode), 1, fileSys.imageFile);
            
            
            printPermissions(newNode.mode);
            printf("%10u ", newNode.size);
            printFileName(file.name);
            printf("\n");
         }
      }
   }
   
   // printf("%d %d\n", filesRead, num_files);
   
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
   if(*cmp) {
      fileSys.path = keepPath;
      return 0;
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