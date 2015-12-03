#define DIRECT_ZONES 7
#define INDIRECT_ZONE 4
/* constants */
#define PTABLE_OFFSET 0x1BE
#define PMAGIC510 0x55
#define PMAGIC511 0xAA
#define MINIXPART 0x81
#define MIN_MAGIC 0x4d5a /* the minix magic number */
#define MIN_MAGIC_REV 0x5a4d /* the minix magic number reversed*/
#define MIN_MAGIC_OLD 0x2468 /* the v2 minix magic number */
#define MIN_MAGIC_REV_OLD 0x6824 /* the v2 magic number reversed*/
                                 /*we have an endian problem */
#define MIN_ISREG(m) (((m)&0170000)==0100000)
#define MIN_ISDIR(m) (((m)&0170000)==0040000)
#define MIN_IRUSR 0400
#define MIN_IWUSR 0200
#define MIN_IXUSR 0100
#define MIN_IRGRP 0040
#define MIN_IWGRP 0020
#define MIN_IXGRP 0010
#define MIN_IROTH 0004
#define MIN_IWOTH 0002
#define MIN_IXOTH 0001
#define SECTOR_SIZE 512
#define BLOCK_SIZE 1024

#define FILEENT_SIZE    64
#define PERMISSION_MASK 256

#define IS_DIRECTORY(m) ((m)&0040000)
#define DIRECTORY 0040000

#ifndef DIRSIZ
#define DIRSIZ 60
#endif


typedef struct fileent {
uint32_t ino;
char name[DIRSIZ];
} fileent;

typedef struct partition {
/* see include/ibm/partition.h */
uint8_t bootind;
uint8_t start_head; /* start head */
uint8_t start_sec; /* start sector */
uint8_t start_cyl; /* start cylinder */
uint8_t type; /* partition type */
uint8_t end_head; /* end head */
uint8_t end_sec; /* end sector */
uint8_t end_cyl; /* end cylinder */
uint32_t lFirst; /* logical first sector */
uint32_t size; /* size of partition (in sectors )*/
} partition;

typedef struct superblock { /* Minix Version 3 Superblock
* this structure found in fs/super.h
* in minix 3.1.1
*/
/* on disk. These fields and orientation are non–negotiable */
uint32_t ninodes; /* number of inodes in this filesystem */
uint16_t pad1; /* make things line up properly */
int16_t i_blocks; /* # of blocks used by inode bit map */
int16_t z_blocks; /* # of blocks used by zone bit map */
uint16_t firstdata; /* number of first data zone */
int16_t log_zone_size; /* log2 of blocks per zone */
int16_t pad2; /* make things line up again */
uint32_t max_file; /* maximum file size */
uint32_t zones; /* number of zones on disk */
int16_t magic; /* magic number */
int16_t pad3; /* make things line up again */
uint16_t blocksize; /* block size in bytes */
uint8_t subversion; /* filesystem sub–version */
} superblock;/**/


typedef struct inode {
uint16_t mode; /* mode */
uint16_t links; /* number or links */
uint16_t uid;
uint16_t gid;
uint32_t size;
int32_t atime;
int32_t mtime;
int32_t ctime;
uint32_t zone[DIRECT_ZONES];
uint32_t indirect;
uint32_t two_indirect;
uint32_t unused;
} inode;

typedef struct printNode {
uint16_t mode; /* mode */
uint16_t links; /* number or links */
uint16_t uid;
uint16_t gid;
uint32_t size;
int32_t atime;
int32_t mtime;
int32_t ctime;
uint32_t zone[DIRECT_ZONES];
uint32_t indirect;
uint32_t two_indirect;
uint32_t unused;
char name[DIRSIZ];
int found;
} printNode;

typedef struct filesystem {
   FILE *imageFile;
   char *path;
   uint32_t bootblock;
   superblock super;
   int part;
   int subPart;
   uint32_t zonesize;
   uint32_t blocksize;
} filesystem;


void findPartition(int partitionNum);
void findSuperBlock();
void findPath();
void printFile(printNode node);
void printFileName(char* fileName);
void printPermissions(uint16_t mode);
int fileCmp(char* cmp);
printNode searchInode(inode node, uint32_t zoneNum);
printNode searchZone(inode node);
void printVerbosePath(printNode node);