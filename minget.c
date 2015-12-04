#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include "lib.h"

void copy();
/*External variables used from the globals of lib.c*/
extern filesystem fileSys;
extern int verbose;
FILE *dstpath;

int main(int argc, char *argv[]) {
   /*getopt is used to easily parse the flags and arguments*/
   extern char *optarg;
   extern int optind;
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
            "usage: minget [ -v ] [ -p part [ -s subpart ] ]"
                    "imagefile srcpath [ dstpath ]\n"
            "Options:\n"
            "-p part    --- select partition for filesystem (default: none)\n"
             "-s sub     --- select subpartition for filesystem"
                    "(default: none)\n"
             "-h help    --- print usage information and exit\n"
             "-v verbose --- increase verbosity level\n"
                    );
            exit(EXIT_FAILURE);
         default:
            fprintf(stderr,
             "usage: minget [ -v ] [ -p part [ -s subpart ] ]"
                    "imagefile srcpath [ dstpath ]\n"
             "Options:\n"
            "-p part    --- select partition for filesystem (default: none)\n"
              "-s sub     --- select subpartition for filesystem"
                  "(default: none)\n"
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
   /*Partition => call findPartition() for part && subpart*/
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

/*Copies over the file from the srcpath to dstpath or stdout*/
void copy() {
   inode node;
   printNode newNode;
   int i;
   char *buffer;
   int32_t nodeSize;
   int32_t readSize;
   
   /*Buffer will store the file contents from zone to zone*/
   buffer = malloc(fileSys.zonesize);
   fseek(fileSys.imageFile, fileSys.bootblock + fileSys.blocksize +
         fileSys.blocksize + fileSys.blocksize * fileSys.super.i_blocks +
         fileSys.blocksize * fileSys.super.z_blocks, SEEK_SET);
   fread(&node, sizeof(inode), 1, fileSys.imageFile);
   /*newNode is the inode of the file*/
   newNode = searchZone(node);
   nodeSize = newNode.size;
   
   if(!newNode.found) {
      fprintf(stderr, "File not found.\n");
      exit(EXIT_FAILURE);
   }

   /*Check if it is not a regular file.*/
   if(!MIN_ISREG(newNode.mode)) {
      fprintf(stderr, "Not a regular file.\n");
      exit(EXIT_FAILURE);
   }
   /*Loop through the 7 direct zones to get the file contents*/
   for (i = 0; i < DIRECT_ZONES; i++) {
      /*As long as you havent read nodeSize keep looping
       though the zones to read zoneSize*/
      if(nodeSize > 0) {
         /*If nodeSize is less than the zonesize, just read nodeSize 
          amount of the zone*/
         if (nodeSize < fileSys.zonesize) {
            readSize = nodeSize;
         }
         /*ELse read the whole zone*/
         else {
            readSize = fileSys.zonesize;
         }
         /*If 0 appears a a zone of a file, it means
          that the entire zone referred to is to be treated as
          all zeros.*/
         if (newNode.zone[i] == 0) {
            memset(buffer, 0, readSize);
         }
         /*Go to the position of the zone's contents and read readSize*/
         else {
            fseek(fileSys.imageFile,
                  fileSys.bootblock +
                  (fileSys.zonesize * newNode.zone[i]), SEEK_SET);
            fread(buffer, readSize, 1, fileSys.imageFile);
         }
         /*Subtract the amount read from the current zone and
          write the buffer to the destination*/
         nodeSize -= readSize;
         fwrite((void*)buffer, readSize, 1, dstpath);
      }
   }
   
}









