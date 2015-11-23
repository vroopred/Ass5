#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "minls.h"

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
      printf("Partition: %d\n", partition);
      if(i < argc && argv[i][0] == '-' && argv[i][1] == 's') {
         printf("Getting here\n");
         if(i + 2 > argc) {
            fprintf(stderr, "Not enough arguments for %s flag\n", argv[i]);
            return -1;
         }
         i++; 
         subpartition = atoi(argv[i++]);
         printf("Subpartition: %d\n", subpartition);
      }
   }

   if(i < argc) {
      imagefile = argv[i++];
      fprintf(stderr, "Imagefile is %s\n", imagefile);
   }

   if(i < argc) {
      path = argv[i++];
      fprintf(stderr, "Path is %s\n", path);
   }

   return 0;
}