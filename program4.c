#include <stdio.h>

#include "ext2.h"

int main(int argc, char **argv) {
   char *fileName = NULL;
   char *path = NULL;
   uint8_t listContents = 0;
   FILE *fp;

   if (argc == 2) {
      fileName = argv[1];
   } else if (argc == 3) {
      fileName = argv[1];
      path = argv[2];
   } else if (argc == 4) {
      listContents = 1;
      fileName = argv[2];
      path = argv[3];
   } else {
      fprintf(stderr, "ERROR: incorrect syntax\n");
      return 0;
   }

   if (! fileName) {
      fprintf(stderr, "ERROR: missing file name\n");
      return 0;
   }

   fp = fopen(fileName, "r");

   ext2_init(fp);
   printInode(path, listContents);

   fclose(fp);

   return 0;
}