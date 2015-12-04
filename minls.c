#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include "lib.h"

/*External variables used from the globals of lib.c*/
extern filesystem fileSys;
extern int verbose;

int main(int argc, char *argv[]) {
   /*getopt is used to easily parse the flags and arguments*/
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
   /*Partition => call findPartition() for part && subpart*/
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
