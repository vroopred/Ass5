#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "minls.h"

filesystem stuff;
filesystem *fileSys;
int verbose = 0;

int main(int argc, char *argv[]) {
   /*minget [ -v ] [ -p part [ -s subpart ] ] imagefile srcpath [ dstpath ]
    Minget copies a regular file from the given source path to the given destination
    path. If the destination path is ommitted, it copies to stdout*/
   
   int i = 1;
   int partition = 0;
   int subpartition = 0;
   char* imagefile = 0;
   char* path = 0;
   char* srcpath;
   char* dstpath;
   
   fileSys = &stuff;
   
   if(argc == 1 || (argv[i][0] == '-' && argv[i][1] == 'h')) {
      printf(
             "usage: minget [ -v ] [ -p part [ -s subpart ] ] imagefile srcpath [ dstpath ]\n"
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
         fileSys->subPart = -1;
      }
   }
   else {
      fileSys->part = -1;
   }
   
   if(i < argc) {
      imagefile = argv[i++];
      /*open file image and set the FILE pointer to the field*/
      fileSys->imageFile = fopen(imagefile, "rb");
      if (fileSys->imageFile == NULL) {
         fprintf(stderr, "Cannot open the image file %s\n", imagefile);
      }
      srcpath = argv[++];
   }
   
   if(i < argc) {
      dstpath = argv[i++];
      //fileSys->path = path;
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
   if (fileSys->part > -1) {
      findPartition(fileSys->part);
      if (fileSys->subPart > -1) {
         findPartition(fileSys->subPart);
      }
   }
   findSuperBlock();
   return 0;
}

void copy() {
   //open file
   //regular file check
}