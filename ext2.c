#include "ext2.h"

static FILE *fp = NULL;

static struct ext2_inode currentInode;
static uint32_t currentInodeNum = 0;

#ifndef DEBUG
#define DEBUG 0
#endif

#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define min(a, b) \
        ((a) < (b) ? (a) : (b))

//the block argument is in terms of SD card 512 byte sectors
void read_data(uint32_t block, uint16_t offset, uint8_t* data, uint16_t size) {
   if (offset > 511) {
      printf ("Offset greater than 511.\n");
      exit(0);
   }

   debug_print("read_data\tpos: %d\tsize: %d\n", block*512 + offset, size);

   fseek(fp,block*512 + offset,SEEK_SET);
   fread(data,size,1,fp);
}

uint32_t getIndirect(uint32_t address, uint32_t index) {
   address *= 1024;
   address += index * 4;
   read_data(address / 512, address % 512, (void *) &address, 4);
   return address;
}

uint32_t getDIndirect(uint32_t address, uint32_t index) {
   address *= 1024;
   address += (index / 256) * 4;
   read_data(address / 512, address % 512, (void *) &address, 4);
   return getIndirect(address, index % 256);
}

uint32_t getTIndirect(uint32_t address, uint32_t index) {
   address *= 1024;
   address += (index / (256 * 256)) * 4;
   read_data(address / 512, address % 512, (void *) &address, 4);
   return getDIndirect(address, index % (256 * 256));
}

void getBlockData(uint32_t offset, void *data, uint8_t size) {
   if ((offset % 1024) + size > 1024) {
      uint8_t pre = 1024 - (offset % 1024);
      getBlockData(offset, data, pre);
      getBlockData(offset + pre, (void *) (((char *) data) + pre), size - pre);
      return;
   }

   uint32_t index = offset / 1024;
   uint32_t blockAddr;

   if (index < EXT2_NDIR_BLOCKS) {
      blockAddr = currentInode.i_block[index];
   } else {
      index -= EXT2_NDIR_BLOCKS;
      if (index < 256) {
         blockAddr = getIndirect(currentInode.i_block[EXT2_IND_BLOCK], index);
      } else {
         index -= 256;
         if (index < 256 * 256) {
            blockAddr = getDIndirect(currentInode.i_block[EXT2_DIND_BLOCK], index);
         } else {
            index -= 256 * 256;
            if (index < 256 * 256 * 256) {
               blockAddr = getTIndirect(currentInode.i_block[EXT2_TIND_BLOCK], index);
            } else {
               fprintf(stderr, "ERROR: too large of offset\n");
               exit(0);
            }
         }
      }
   }

   blockAddr *= 1024;
   blockAddr += offset % 1024;
   if ((blockAddr % 512) + size > 512) {
      uint8_t pre = 512 - (blockAddr % 512);
      read_data(blockAddr / 512, blockAddr % 512, data, pre);
      read_data((blockAddr / 512) + 1, 0, (void *)(((char *) data) + pre), size - pre);
   } else {
      read_data(blockAddr / 512, blockAddr % 512, data, size);
   }
}

void getInode(uint32_t inode) {
   if (inode == currentInodeNum)
      return;

   uint32_t inodesPerGroup;
   read_data(2, 40, (void *) &inodesPerGroup, 4);

   debug_print("inodesPerGroup: %d\n", inodesPerGroup);

   uint32_t group = (inode - 1) / inodesPerGroup;
   uint32_t address = 1024 * (8192 * group + 5) + 128 * ((inode - 1) % inodesPerGroup);

   debug_print("group: %d\taddress: %d\n", group, address);

   read_data(address / 512, address % 512, (void *) &currentInode, 128);

   currentInodeNum = inode;
}

uint32_t findInode(uint32_t base, char *name) {
   getInode(base);

   uint32_t inode, pos;
   uint16_t recLen;
   uint8_t nameLen;

   pos = 0;
   while (pos < currentInode.i_size) {
      getBlockData(pos, &inode, 4);
      getBlockData(pos + 4, &recLen, 2);
      getBlockData(pos + 6, &nameLen, 2);

      if (inode == 0) {
         recLen = 1024 - (pos % 1024);
      } else if (nameLen == strlen(name)) {
         char *tmp = malloc(nameLen);
         getBlockData(pos + 8, tmp, nameLen);
         uint8_t cmp = strncmp(name, tmp, nameLen);
         free(tmp);
         if (cmp == 0)
            return inode;
      }

      pos += recLen;
   }

   fprintf(stderr, "ERROR: file not found\n");
   exit(0);
}

uint32_t findFileInode(char *path) {
   uint32_t inode = EXT2_ROOT_INO;
   char *pch;

   debug_print("inode: %d\n", inode);
   pch = strtok(path, "/");
   while (pch != NULL) {
      inode = findInode(inode, pch);
      pch = strtok(NULL, "/");
      debug_print("inode: %d\n", inode);
   }

   return inode;
}

uint8_t inodeIsFile(uint32_t inode) {
   getInode(inode);
   debug_print("i_mode: %d\n", currentInode.i_mode);
   return currentInode.i_mode >> 12 == 8;
}

void printInfo(uint32_t inode, char *name) {
   getInode(inode);

   float size;
   char *suffix = " B";
   if (currentInode.i_size >= 1024 * 1024) {
      size = currentInode.i_size / (1024.0 * 1024.0);
      suffix = "MB";
   } else if (currentInode.i_size >= 1024) {
      size = currentInode.i_size / (1024.0);
      suffix = "KB";
   } else {
      size = currentInode.i_size;
   }

   printf("%5.1f %s\t%s%s\n", size, suffix, name, inodeIsFile(inode) ? "" : "/");
}

uint32_t getNextListing(uint32_t inode, char **last) {
   getInode(inode);

   char *next = NULL;
   uint32_t nextInode = 0, offset = 0;
   uint16_t nameLen, recLen;

   while (offset < currentInode.i_size) {
      debug_print("%d / %d\n", offset, currentInode.i_size);
      getBlockData(offset + 4, &recLen, 2);
      getBlockData(offset + 6, &nameLen, 2);

      debug_print("recLen: %d, nameLen: %d\n", recLen, nameLen);

      char *name = calloc(nameLen + 1, 1);
      getBlockData(offset + 8, name, nameLen);

      int8_t comp = *last ? strncmp(*last, name, min(strlen(*last), nameLen)) : -1;
      if (comp < 0 || (comp == 0 && nameLen > strlen(*last))) {
         comp = next ? strncmp(next, name, min(strlen(next), nameLen)) : 1;

         if (comp > 0 || (comp == 0 && nameLen < strlen(next))) {
            free(next);
            next = name;
            getBlockData(offset, &nextInode, 4);
         } else {
            free(name);
         }
      }
      
      offset += recLen;
   }

   if (next)
      debug_print("%s\n", next);

   free(*last);
   *last = next;

   return nextInode;
}

void printFolderListing(uint32_t inode) {
   uint32_t nextInode;
   char *last = NULL;

   while (nextInode = getNextListing(inode, &last)) {
      debug_print("nextInode: %d\n", nextInode);
      printInfo(nextInode, last);
   }

   free(last);
}

void printInode(char *path, uint8_t contents) {
   char *cpyPath = NULL;
   if (path) {
      cpyPath = malloc(strlen(path) + 1);
      strcpy(cpyPath, path);
   }

   uint32_t inode = findFileInode(cpyPath);
   free(cpyPath);

   uint8_t isFile = inodeIsFile(inode);

   if (contents) {
      if (!isFile) {
         fprintf(stderr, "ERROR: %s is not a file\n", path);
         exit(0);
      }
      char tmp[129];
      uint32_t pos;
      uint8_t read = 128;

      for (pos = 0; pos < currentInode.i_size; pos += read) {
         if (pos + read > currentInode.i_size)
            read = currentInode.i_size - pos;

         getBlockData(pos, tmp, read);
         tmp[read] = 0;
         printf("%s", tmp);
         debug_print("\n%d / %d\n", pos, currentInode.i_size);
      }
      fflush(stdout);
   } else {
      if (isFile)
         printInfo(inode, path);
      else
         printFolderListing(inode);
   }
}

void ext2_init(FILE *file) {
   fp = file;
}
