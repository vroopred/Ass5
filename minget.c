#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include "lib.h"

void copy();

extern filesystem fileSys;
extern int verbose;
FILE *dstpath;

int main(int argc, char *argv[]) {
   /*minget [ -v ] [ -p part [ -s subpart ] ] imagefile srcpath [ dstpath ]
    Minget copies a regular file from the given source path to the given destination
    path. If the destination path is ommitted, it copies to stdout*/
   
   extern char *optarg;
   extern int optind;
   FILE* dstpath = NULL;
   char* imagefile = 0;
   int c = getopt(argc, argv, "vp:s:h");
   int sflag = 0, pflag = 0;
   int isPart = 0;
   verbose = 0;
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
                    "usage: minget [ -v ] [ -p part [ -s subpart ] ] imagefile srcpath [ dstpath ]\n"
                    "Options:\n"
                    "-p part    --- select partition for filesystem (default: none)\n"
                    "-s sub     --- select subpartition for filesystem (default: none)\n"
                    "-h help    --- print usage information and exit\n"
                    "-v verbose --- increase verbosity level\n"
                    );
            exit(EXIT_FAILURE);
         default:
            fprintf(stderr,
                    "usage: minget [ -v ] [ -p part [ -s subpart ] ] imagefile srcpath [ dstpath ]\n"
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
   if (optind >= argc) {
      fprintf(stderr, "Need an srcpath.\n");
      exit(EXIT_FAILURE);
   }
   else {
      fileSys.path = argv[optind++];
      printf("%s\n",fileSys.path);
   }
   if (optind < argc) {
      if ((dstpath = fopen(argv[optind++], "w+")) == NULL) {
         fprintf(stderr, "Cannot write to dstfile.\n");
         exit(EXIT_FAILURE);
      }
   }
   else {
      dstpath = stdout;
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
   copy();
   return 0;
}

   /*1. Using findPath, get the inode of the file from that path
     2. Check if its not a directory
    3. Get the contents of the file
    */

void copy() {
   inode node;
   printNode newNode;
   int i;
   void * buffer;
   int32_t nodeSize;
   int32_t readSize;
   
   buffer = malloc(fileSys.zonesize);
   fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize +
         fileSys.blocksize + fileSys.blocksize * fileSys.super.i_blocks +
         fileSys.blocksize * fileSys.super.z_blocks, SEEK_SET);
   fread(&node, sizeof(inode), 1, fileSys.imageFile);
   
   newNode = searchZone(node);
   nodeSize = newNode.size;
   
   if(!newNode.found) {
      fprintf(stderr, "File not found.\n");
      exit(EXIT_FAILURE);
   }

   //Check if it's a file. This is special and gets printed differently
   if(!IS_DIRECTORY(newNode.mode)) {
      fprintf(stderr, "Not a regular file.\n");
      exit(EXIT_FAILURE);
   }
   
   for (i = 0; i < DIRECT_ZONES; i++) {
      if(nodeSize < 0) {
         if (nodeSize < fileSys.zonesize) {
            readSize = nodeSize;
         }
         else {
            readSize = fileSys.zonesize;
         }
         if (!newNode.zone[i]) {
            memset(buffer, 0, readSize);
         }
         else {
            fseek(fileSys.imageFile,
               fileSys.bootblock + (fileSys.zonesize * newNode.zone[i]), SEEK_SET);
            fread(buffer, readSize, 1, fileSys.imageFile);
         }
         nodeSize -= readSize;
         fwrite(buffer, readSize, 1, dstpath);
      }
   }
   
}









