// Creates a system disk image
//
// This program runs only on development environment,
// so architecture can (will usually) differ from
// target architecture
//
// Expected parameters:
// output_file block_count kernel [other files]

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define MKFS // fs.h needs this

// Define these types for development machine arch
// fs.h also needs this
typedef char         uint8_t;
typedef unsigned int uint32_t;

#include "fs.h"

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

// Output file descriptor
int fsfd;

// Write and read blocks
void wblock(uint, void*);
void rblock(uint sec, void *buf);

// Entry point
int main(int argc, char *argv[])
{
  int i, f, e, b, cc, fd;
  char* name;
  char buf[BLOCK_SIZE];
  struct SFS_SUPERBLOCK sfs_sb;
  struct SFS_ENTRY* sfs_entry;

  // Check architecture and fs definition sizes
  assert(sizeof(uint8_t)  == 1);
  assert(sizeof(uint32_t) == 4);
  assert(BLOCK_SIZE % sizeof(struct SFS_ENTRY) == 0 ||
         sizeof(struct SFS_ENTRY) % BLOCK_SIZE == 0);

  // Check usage
  if(argc < 5) {
    fprintf(stderr,
      "Usage: %s output_file fs_size_blocks boot_sect kernel_file [other_files ...]\n",
      argv[0]);

    exit(1);
  }

  // Get fs parameters
  int fssize_blocks = atoi(argv[2]);  // Size of file system in blocks
  int numentries = min(((fssize_blocks * BLOCK_SIZE)/10)/sizeof(struct SFS_ENTRY), 4096);
  int entries_size = numentries * sizeof(struct SFS_ENTRY);

  // Open output file
  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0) {
    perror(argv[1]);
    exit(1);
  }

  // Create empty boot block
  memset(buf, 0, sizeof(buf));

  // Now add boot image to the block
  if((fd = open(argv[3], 0)) < 0) {
    perror(argv[3]);
    exit(1);
  }

  read(fd, buf, 512);
  close(fd);
  wblock(0, buf);

  // Write all image with 0s
  memset(buf, 0, sizeof(buf));
  for(i = 1; i < fssize_blocks; i++)
    wblock(i, buf);

  // Superblock
  sfs_sb.type = SFS_TYPE_ID;
  sfs_sb.size = fssize_blocks;
  sfs_sb.nentries = numentries;
  sfs_sb.bootstart = 2 + entries_size/BLOCK_SIZE;

  memmove(buf, &sfs_sb, sizeof(sfs_sb));
  wblock(1, buf);

  printf("%s: creating %s (size=%d nentries=%d bootstart=%d)\n",
    argv[0], argv[1], sfs_sb.size, sfs_sb.nentries, sfs_sb.bootstart);

  // Create empty entries table
  sfs_entry = malloc(entries_size);
  memset(sfs_entry, 0, entries_size);
  e = 0;
  b = sfs_sb.bootstart;

  // Create root dir
  strncpy(sfs_entry[e].name, ROOT_DIR_NAME, SFS_NAMESIZE-1);
  sfs_entry[e].flags = T_DIR;
  sfs_entry[e].time = 0;
  sfs_entry[e].size = argc - 4;
  sfs_entry[e].parent = 0;
  sfs_entry[e].next = 0;

  e++;

  // Copy input files to image
  // The first one is expected to be the kernel
  for(f = 4; f < argc; f++) {

    // Open file
    if((fd = open(argv[f], 0)) < 0) {
      perror(argv[f]);
      exit(1);
    }

    // Remove slashes from name
    name = strrchr(argv[f], '/');
    if(name == 0) {
      name = argv[f];
    } else {
      name++;
    }

    // Create first file entry
    sfs_entry[0].ref[f - 4] = e;

    strncpy(sfs_entry[e].name, name, SFS_NAMESIZE-1);
    sfs_entry[e].flags = T_FILE;
    sfs_entry[e].time = 0;
    sfs_entry[e].size = 0;
    sfs_entry[e].parent = 0;
    sfs_entry[e].next = 0;


    // Read file, write data block, and add more entries
    // when there is no room for more refs in current entry
    i = 0;

    while((cc = read(fd, buf, BLOCK_SIZE)) > 0) {
      int ec = e + (sfs_entry[e].size / (SFS_ENTRYREFS*BLOCK_SIZE));
      if(e != ec && sfs_entry[ec].name[0] == 0) {
        strncpy(sfs_entry[ec].name, sfs_entry[e].name, SFS_NAMESIZE-1);
        sfs_entry[ec-1].next = ec;
        sfs_entry[ec].flags = T_FILE;
        sfs_entry[ec].time = 0;
        sfs_entry[ec].size = 0;
        sfs_entry[ec].parent = ec-1;
        sfs_entry[ec].next = 0;
        i = 0;
      }

      sfs_entry[ec].ref[i] = b;
      wblock(sfs_entry[ec].ref[i], buf);
      while(ec >= e) {
        sfs_entry[ec].size += cc;
        ec--;
      }
      i++;
      b++;
    }

    close(fd);

    while(sfs_entry[e].name[0]) {
      e++;
    }
  }

  // Write entries table
  if(lseek(fsfd, 2*BLOCK_SIZE, 0) != 2*BLOCK_SIZE) {
    perror("lseek");
    exit(1);
  }

  if(write(fsfd, sfs_entry, entries_size) != entries_size) {
    perror("write");
    exit(1);
  }

  // Done! Free memory, close file, and exit
  free(sfs_entry);
  close(fsfd);

  exit(0);
}

// Write block
void wblock(uint bindex, void* buf)
{
  if(lseek(fsfd, bindex*BLOCK_SIZE, 0) != bindex*BLOCK_SIZE) {
    perror("lseek");
    exit(1);
  }
  if(write(fsfd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
    perror("write");
    exit(1);
  }
}

// Read block
void rblock(uint bindex, void* buf)
{
  if(lseek(fsfd, bindex*BLOCK_SIZE, 0) != bindex*BLOCK_SIZE) {
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
    perror("read");
    exit(1);
  }
}
