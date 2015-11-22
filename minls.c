#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {

   if(argc < 2) {
      printf(
         "usage: minls  [ -v ] [ -p num] ] imagefile [ path ]\n"
         "Options:\n"     
         "-p part    --- select partition for filesystem (default: none)\n"
         "-s sub     --- select subpartition for filesystem (default: none)\n"
         "-h help    --- print usage information and exit\n"
         "-v verbose --- increase verbosity level\n"
         );
   }


   return 0;
}