/*
 * File system
 */

#include "types.h"
#include "kernel.h"
#include "fs.h"
#include "hw86.h"
#include "ulib/ulib.h"

/*
 * See fs.h for more detailed description
 * of file system and functions
 */

/*
 * Disk id to disk index
 */
uint disk_to_index(uint disk)
{
  uint index = 0;
  for(index=0; index<MAX_DISK; index++) {
    if(disk_info[index].id == disk) {
      return index;
    }
  }
  return ERROR_NOT_FOUND;
}

/*
 * Disk index to disk id
 */
uint index_to_disk(uint index)
{
  if(index < MAX_DISK) {
    return disk_info[index].id;
  }

  return ERROR_NOT_FOUND;
}

/*
 * Disk id to string
 */
uchar* disk_to_string(uint disk)
{
  uint index = 0;
  for(index=0; index<MAX_DISK; index++) {
    if(disk_info[index].id == disk) {
      return disk_info[index].name;
    }
  }

  return "unk";
}

/*
 * String to disk id
 */
uint string_to_disk(uchar* str)
{
  uint index = 0;
  for(index=0; index<MAX_DISK; index++) {
    if(strcmp(str, disk_info[index].name) == 0) {
      return disk_info[index].id;
    }
  }

  return ERROR_NOT_FOUND;
}

/*
 * Return 1 if string is a disk identifier, 0 otherwise
 */
uint string_is_disk(uchar* str)
{
  uint index = 0;

  for(index=0; index<MAX_DISK; index++) {
    uint disk = index_to_disk(index);
    if(strcmp(str, disk_to_string(disk)) == 0) {
      return 1;
    }
  }
  return 0;
}

/*
 * Convert number of blocks to size in MB
 */
uint32_t blocks_to_MB(uint32_t blocks)
{
  return ((uint32_t)blocks*(uint32_t)BLOCK_SIZE)/1048576L;
}

/*
 * Convert a user input entry name to a valid name.
 * Bad symbols are removed from input str and lenght
 * is clamped to SFS_NAMESIZE-1
 */
static uchar* string_to_name(uchar* str)
{
  uint i=0;
  for(i=0; i<strlen(str); i++) {
    if((str[i] < '0' || str[i] > '9') &&
       (str[i] < 'A' || str[i] > 'Z') &&
       (str[i] < 'a' || str[i] > 'z') &&
      str[i] != '.' &&
      str[i] != '-' && str[i] != '_' &&
      str[i] != '(' && str[i] != ')' &&
      str[i] != '[' && str[i] != ']') {
      str[i] = '_';
    }
    if(i == SFS_NAMESIZE-1) {
      str[i] = 0;
      break;
    }
  }
  return str;
}

/*
 * Read disk, specific block, offset and size
 * Returns 0 on success, another value otherwise
 */
static uint read_disk(uint disk, uint block, uint offset, uint buff_size, uchar* buff)
{
/*  uchar* aux_buff; [SECTOR_SIZE];*/
  uint n_sectors = 0;
  uint i = 0;
  uint result = 0;
  uint sector = 0;

  /* Check params */
  if(buff == 0) {
    debugstr("Read disk: bad buffer\n\r");
    return 1;
  }

  if(disk_info[disk_to_index(disk)].size == 0) {
    debugstr("Read disk: bad disk\n\r");
    return 1;
  }

  /* Convert blocks to sectors */
  if(BLOCK_SIZE >= SECTOR_SIZE) {
    sector = block * (BLOCK_SIZE / SECTOR_SIZE);
  } else {
    sector = (block * BLOCK_SIZE) / SECTOR_SIZE;
  }

  /* Compute initial sector and offset */
  sector += offset / SECTOR_SIZE;
  offset = offset % SECTOR_SIZE;

  /* read_disk_sector can only read entire and aligned sectors.
   * If requested offset is unaligned to sectors, read an entire
   * sector and copy only requested bytes in buff */
  if(offset) {
    result = read_disk_sector(disk, sector, 1, disk_buff);
    i = min(SECTOR_SIZE-offset, buff_size);
    memcpy(buff, &disk_buff[offset], i);
    sector++;
    buff_size -= i;
  }

  /* Now read aligned an entire sectors */
  n_sectors = buff_size / SECTOR_SIZE;

  if(n_sectors && result == 0) {
    /* Try to read all sectors at once */
    result = read_disk_sector(disk, sector, n_sectors, &buff[i]);

    /* Handle DMA access 64kb boundary. Read sector by sector */
    if(result == 0x900) {
      debugstr("Read disk: DMA access accross 64Kb boundary. Reading sector by sector\n\r");
      for(; n_sectors > 0; n_sectors--) {
        result = read_disk_sector(disk, sector, 1, disk_buff);
        if(result != 0) {
          break;
        }
        memcpy(&buff[i], disk_buff, SECTOR_SIZE);
        i += SECTOR_SIZE;
        sector++;
        buff_size -= SECTOR_SIZE;
      }
    } else {
      buff_size -= SECTOR_SIZE * n_sectors;
      sector += n_sectors;
      i += SECTOR_SIZE * n_sectors;
    }
  }

  /* read_disk_sector can only read entire and aligned sectors.
   * If requested size exceeds entire sectors, read
   * an entire sector and copy only requested bytes */
  if(buff_size && result == 0) {
    result = read_disk_sector(disk, sector, 1, disk_buff);
    memcpy(&buff[i], disk_buff, buff_size);
  }

  if(result != 0) {
    debugstr("Read disk error (%x)\n\r", result);
  }

  return result;
}

/*
 * Write disk, specific sector, offset and size
 * Returns 0 on success, another value otherwise
 */
static uint write_disk(uint disk, uint block, uint offset, uint buff_size, uchar* buff)
{
  uint n_sectors = 0;
  uint i = 0;
  uint result = 0;
  uint sector = 0;

  /* Check params */
  if(buff == 0) {
    debugstr("Write disk: bad buffer\n\r");
    return 1;
  }

  if(disk_info[disk_to_index(disk)].size == 0) {
    debugstr("Write disk: bad disk\n\r");
    return 1;
  }

  /* Convert blocks to sectors */
  if(BLOCK_SIZE >= SECTOR_SIZE) {
    sector = block * (BLOCK_SIZE / SECTOR_SIZE);
  } else {
    sector = (block * BLOCK_SIZE) / SECTOR_SIZE;
  }

  /* Compute initial sector and offset */
  sector += offset / SECTOR_SIZE;
  offset = offset % SECTOR_SIZE;

  /* write_disk_sector can only write entire and aligned sectors.
   * If requested offset is unaligned to sectors, read an entire
   * sector, overwrite requested bytes, and write it */
  if(offset) {
    result += read_disk_sector(disk, sector, 1, disk_buff);
    i = min(SECTOR_SIZE-offset, buff_size);
    memcpy(&disk_buff[offset], buff, i);
    result += write_disk_sector(disk, sector, 1, disk_buff);
    sector++;
    buff_size -= i;
  }

  /* Now write aligned an entire sectors */
  n_sectors = buff_size / SECTOR_SIZE;

  if(n_sectors && result == 0) {
    /* Try to write all sectors at once */
    result += write_disk_sector(disk, sector, n_sectors, &buff[i]);
    /* Handle DMA access 64kb boundary. Write sector by sector */
    if(result == 0x900) {
      debugstr("Write disk: DMA access accross 64Kb boundary. Writting sector by sector\n\r");
      for(; n_sectors > 0; n_sectors--) {
        memcpy(disk_buff, &buff[i], SECTOR_SIZE);
        result = write_disk_sector(disk, sector, 1, disk_buff);
        if(result != 0) {
          break;
        }
        i += SECTOR_SIZE;
        sector++;
        buff_size -= SECTOR_SIZE;
      }
    } else {
      buff_size -= SECTOR_SIZE * n_sectors;
      sector += n_sectors;
      i += SECTOR_SIZE * n_sectors;
    }
  }

  /* write_disk_sector can only write entire and aligned sectors.
   * If requested size exceeds entire sectors, read
   * an entire sector, overwrite requested bytes, and write */
  if(buff_size && result == 0) {
    result += read_disk_sector(disk, sector, 1, disk_buff);
    memcpy(disk_buff, &buff[i], buff_size);
    result += write_disk_sector(disk, sector, 1, disk_buff);
  }

  if(result != 0) {
    debugstr("Write disk error (%x)\n\r", result);
  }

  return result;
}

/*
 * Get filesystem info
 */
uint fs_get_info(uint disk_index, fs_info_t* info)
{
  uint i=0, j=0xFFFF, n=0;

  /* Check parameters */
  if(info == 0) {
    return 1;
  }

  /* Count avaliable disks (n) and return index of avaliable disks
   * number disk_index (j) */
  n = 0;
  for(i=0; i<MAX_DISK; i++) {
    if(disk_info[i].size != 0) {
      if(n == disk_index) {
        j = i;
      }
      n++;
    }
  }

  /* If found, fill info */
  if(j != 0xFFFF) {
    info->id = index_to_disk(j);
    strcpy_s(info->name, disk_to_string(info->id), sizeof(info->name));
    info->fs_type = disk_info[j].fstype;
    info->fs_size = blocks_to_MB(disk_info[j].fssize);
    info->disk_size = disk_info[j].size;
  }

  return n;
}

/*
 * Init file system info
 * Reads superblock and fills disk info
 */
void fs_init_info()
{
  uint result = 0;
  uint disk_index = 0;

  /* For each disk */
  for(disk_index=0; disk_index<MAX_DISK; disk_index++) {
    debugstr("Check filesystem in %x: ", index_to_disk(disk_index));

    /* If hardware related disk info is valid */
    if(disk_info[disk_index].size != 0) {
      /* Read superblock and check file system type and data */
      sfs_superblock_t sb;
      result = read_disk(index_to_disk(disk_index), 1, 0, sizeof(sb), &sb);
      if(result == 0 && sb.type == SFS_TYPE_ID) {
        disk_info[disk_index].fstype = FS_TYPE_NSFS;
        disk_info[disk_index].fssize = sb.size;
        debugstr("NSFS\n\r");
        continue;
      }
    }
    disk_info[disk_index].fstype = FS_TYPE_UNKNOWN;
    disk_info[disk_index].fssize = 0;
    debugstr("unknown\n\r");
  }
}

/*
 * Get disk id from path string
 */
static uint path_get_disk(uchar* path)
{
  uchar tokpath[64];
  uchar* tok = 0;
  uchar* nexttok = 0;

  /* Make a copy of path because tokenize
   * process will replaces some bytes */
  memset(tokpath, 0, sizeof(tokpath));
  strcpy_s(tokpath, path, sizeof(tokpath));
  tok = tokpath;
  nexttok = tok;

  /* First token should be disk identifier */
  tok = strtok(tok, &nexttok, PATH_SEPARATOR);
  if(*tok) {
    if(string_is_disk(tok)) {
      return string_to_disk(tok);
    }
  }

  /* Return system disk if disk is unspecified */
  return system_disk;
}

/*
 * Given an entry full path (path), this functions computes disk, parent entry index, and
 * a pointer to the start of the name of the entry in the input path.
 * The parent path must exist, but full path entry can not exist.
 * Return 0 on success, ERROR_NOT_FOUND if parent path does not exist, or another error code
 */
static uint path_parse_disk_parent_name(uchar** name, uint* parent, uint* disk, uchar* path)
{
  uchar tokpath[64];
  uchar* tok = 0;
  uchar* nexttok = 0;
  sfs_entry_t entry;
  uint n = 0;

  /* Make a copy of path because tokenize
   * process will replaces some bytes */
  memset(tokpath, 0, sizeof(tokpath));
  strcpy_s(tokpath, path, sizeof(tokpath));
  tok = tokpath;
  nexttok = tok;

  /* Start at root of system disk */
  *disk = system_disk;
  *parent = 0;
  *name = path;

  /* Check entries token by token */
  while(*tok && *nexttok) {
    tok = strtok(tok, &nexttok, PATH_SEPARATOR);
    *name = path + (uint)(tok - tokpath);

    /* First path token can be disk identifier */
    if(*tok && n == 0) {
      n++;
      if(string_is_disk(tok)) {
        *disk = string_to_disk(tok);
        tok = nexttok;
        continue;
      }
    }
    /* Check entry exists */
    if(*tok && *nexttok) {
      *parent = fs_get_entry(&entry, tok, *parent, *disk);
      if(*parent >= ERROR_ANY) {
        return *parent;
      }
      tok = nexttok;
    }
  }

  return 0;
}

/*
 * Get entry by disk and index
 * Returns the input index or ERROR_IO. Does check nothing
 */
static uint get_entry_n(sfs_entry_t* entry, uint disk, uint n)
{
  /* Compute block number and offset */
  uint32_t block = 2L +
    ((uint32_t)n*(uint32_t)sizeof(sfs_entry_t))/(uint32_t)BLOCK_SIZE;

  uint32_t offset = ((uint32_t)n *
    (uint32_t)sizeof(sfs_entry_t)) % (uint32_t)BLOCK_SIZE;

  /* Read and return */
  uint result = read_disk(disk, (uint)block, (uint)offset,
    sizeof(sfs_entry_t), entry);

  return result != 0 ? ERROR_IO : n;
}

/*
 * Get an entry given a path, parent and disk
 */
uint fs_get_entry(sfs_entry_t* entry, uchar* path, uint parent, uint disk)
{
  sfs_superblock_t sb;
  uint n = 0;
  uint result = 0;
  uchar* name = 0;

  /* If path is just a disk identifier and disk is unknown
   * then path is the root directory of this disk */
  if(disk == UNKNOWN_VALUE && string_is_disk(path)) {
    disk = string_to_disk(path);
    path = ROOT_DIR_NAME;
  }
  /* If parent is unknown, start at root directory */
  if(parent == UNKNOWN_VALUE) {
    parent = 0;
  }
  /* If disk is unknown, assume system disk */
  if(disk == UNKNOWN_VALUE) {
    disk = system_disk;
  }

  /* If path has components, get parent, disk and single entry name */
  if(parent == 0 && strchr(path, PATH_SEPARATOR)) {
    name = path;
    result = path_parse_disk_parent_name(&name, &parent, &disk, path);
    if(result >= ERROR_ANY) {
      return result;
    }
    path = name;
  }

  /* Read superblock */
  result = read_disk(disk, 1, 0, sizeof(sb), &sb);
  if(result != 0) {
    return ERROR_IO;
  }

  /* Check all entries */
  while(n < sb.nentries) {
    result = get_entry_n(entry, disk, n);
    if(result >= ERROR_ANY) {
      return result;
    }
    if((entry->flags & F_USED) && entry->parent == parent &&
      !strcmp(entry->name, path)) {
      return n;
    }
    n++;
  }

  return ERROR_NOT_FOUND;
}

/*
 * When the number of references in an entry is not enough to fit contents,
 * a chained entry for the same file or directory is created.
 *
 * Given the first entry of a file or directory (entry), and a reference
 * index (nref), this function finds the chained entry (outentry) which
 * contains its nref-th reference.
 * disk id and index of input entry must be also provided.
 * Returns the index of outentry in the entry table, or an error code
 */
static uint get_nref_entry_from_entry(sfs_entry_t* outentry,
  sfs_entry_t* entry, uint disk, uint nentry, uint nref)
{
  /* Initialize return entry information to the first entry */
  uint result = nentry;
  memcpy(outentry, entry, sizeof(sfs_entry_t));

  /* While reference index exceeds number of references in an entry */
  while((nref = nref / SFS_ENTRYREFS) > 0) {
    if(outentry->next) {
      /* Advance tot he next chained entry */
      result = get_entry_n(outentry, disk, (uint)outentry->next);
      if(result >= ERROR_ANY) {
        return result;
      }
    } else {
      return ERROR_NOT_FOUND;
    }
  }
  return result;
}

/*
 * Read file in buff, given path, offset and count
 */
uint fs_read_file(uchar* buff, uchar* path, uint offset, uint count)
{
  sfs_entry_t entry;
  uint nentry = 0;
  uint result = 0;
  uint read = 0;
  uint block = 0;

  /* Find entry */
  uint disk = path_get_disk(path);
  nentry = fs_get_entry(&entry, path, UNKNOWN_VALUE, UNKNOWN_VALUE);
  if(nentry < ERROR_ANY && (entry.flags & T_FILE)) {
    /* Compute initial block and offset */
    offset = min(offset, entry.size);
    count = min(count, entry.size - offset);
    block = offset / BLOCK_SIZE;
    offset = offset % BLOCK_SIZE;
    while(read < count) {
      /* Get chained entry for a given reference index number */
      nentry = get_nref_entry_from_entry(&entry, &entry, disk, nentry, block);
      if(nentry >= ERROR_ANY) {
        return nentry;
      }
      block = block % SFS_ENTRYREFS;

      /* Read in buffer */
      result = read_disk(disk, (uint)entry.ref[block % SFS_ENTRYREFS], offset,
        (uint)min(BLOCK_SIZE-offset, count-read), &(buff[read]));

      if(result != 0) {
        return ERROR_IO;
      }

      read += min(BLOCK_SIZE - offset, count-read);
      block++;
      offset = 0;
    }
    result = read;
  } else {
    result = nentry;
  }
  return result;
}

/*
 * Write entry by index at disk
 */
static uint write_entry(sfs_entry_t* entry, uint disk, uint n)
{
  /* Compute block number and offset */
  uint32_t block = 2L +
    ((uint32_t)n*(uint32_t)sizeof(sfs_entry_t))/(uint32_t)BLOCK_SIZE;

  uint32_t offset = ((uint32_t)n *
    (uint32_t)sizeof(sfs_entry_t)) % (uint32_t)BLOCK_SIZE;

  /* Write and return */
  uint result = write_disk(disk, (uint)block, (uint)offset,
    sizeof(sfs_entry_t), (uchar*)entry);

  return result != 0 ? ERROR_IO : 0;
}

/*
 * Set entry time to NOW
 */
static uint set_entry_time_to_current(uint disk, uint nentry)
{
  sfs_entry_t entry;
  time_t ctime;
  uint32_t fstime = 0;

  /* Get time and convert to fs time */
  time(&ctime);
  fstime = fs_systime_to_fstime(&ctime);

  /* Set in initial entry and all chained entries */
  do {
    uint result = get_entry_n(&entry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    entry.time = fstime;
    result = write_entry(&entry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    nentry = entry.next;
  } while(nentry != 0);

  return 0;
}

/*
 * Find first free entry in disk
 * Return its index or an error code
 */
static uint find_free_entry(uint disk)
{
  sfs_superblock_t sb;
  sfs_entry_t entry;
  uint n = 0;

  /* Read super block */
  uint result = read_disk(disk, 1, 0, sizeof(sb), &sb);
  if(result != 0) {
    return ERROR_IO;
  }

  /* Find an unused entry */
  while(n < sb.nentries) {
    result = get_entry_n(&entry, disk, n);
    if(result >= ERROR_ANY) {
      return result;
    }
    if(!(entry.flags & F_USED)) {
      return n;
    }
    n++;
  }

  return ERROR_NO_SPACE;
}

/*
 * Get number of needed blocks to contain a given size (bytes)
 */
static uint needed_blocks(uint size)
{
  uint nblocks = size / BLOCK_SIZE;
  if(size % BLOCK_SIZE) {
    nblocks++;
  }
  return nblocks;
}

/*
 * Find first free block at disk
 * Return its index or ERROR_NOT_FOUND
 */
static uint find_free_block(uint disk)
{
  sfs_superblock_t sb;
  sfs_entry_t entry;
  uint free_block = 0;
  uint max_blocks = 0;
  uint first_data_block = 0;
  uint n = 0;
  uint b = 0;
  uint found = 0;

  /* Read superblock */
  uint result = read_disk(disk, 1, 0, sizeof(sb), (uchar*)&sb);
  if(result != 0) {
    return ERROR_IO;
  }

  /* Compute first data block index */
  first_data_block = 2L +
    ((uint32_t)sb.nentries*(uint32_t)sizeof(sfs_entry_t))/(uint32_t)BLOCK_SIZE;

  /* For each possible block index */
  max_blocks = sb.size;
  for(free_block=first_data_block; free_block<max_blocks; free_block++) {

    /* Check if there is an entry referencing it */
    found = 0;
    n = 0;
    while(found == 0 && n < sb.nentries) {
      result = get_entry_n(&entry, disk, n);
      if(result >= ERROR_ANY) {
        return result;
      }
      if(entry.flags & T_FILE) {
        for(b=0; b<min(needed_blocks((uint)entry.size), SFS_ENTRYREFS); b++) {
          if(entry.ref[b] == free_block) {
            /* If there is, this block is not free */
            found = 1;
            break;
          }
        }
      }
      n++;
    }
    if(found == 0) {
      /* Otherwise, this is a free block */
      return free_block;
    }
  }

  return ERROR_NO_SPACE;
}

/*
 * Set references count in an entry
 *
 * Given an initial entry index, this function creates new chained entries
 * or deletes existing ones to fit a given number of total references.
 * Unused or newly created references are set to 0
 */
static uint set_entry_refcount(uint disk, uint nentry, uint refcount)
{
  sfs_entry_t entry;
  uint result = 0;
  uint i = 0;

  /* Compute number of needed entries */
  uint nentries = refcount / SFS_ENTRYREFS;
  if(refcount % SFS_ENTRYREFS) {
    nentries += 1;
  }

  /* Start from the first one */
  result = get_entry_n(&entry, disk, nentry);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Advance to the last chained entry, create if needed */
  while(nentries > 0) {
    if(entry.next) {
      nentry = get_entry_n(&entry, disk, (uint)entry.next);
      if(nentry >= ERROR_ANY) {
        return nentry;
      }
    } else {
      entry.next = find_free_entry(disk);
      if(entry.next >= ERROR_ANY) {
        return entry.next;
      }
      result = write_entry(&entry, disk, nentry);
      if(result >= ERROR_ANY) {
        return result;
      }
      entry.parent = nentry;
      nentry = entry.next;
      entry.next = 0;
      entry.size = 0;
      memset(entry.ref, 0, sizeof(entry.ref));
      result = write_entry(&entry, disk, nentry);
      if(result >= ERROR_ANY) {
        return result;
      }
    }
    nentries--;
  }

  /* Set to 0 unused references of last chained entry */
  for(i=refcount%SFS_ENTRYREFS; i<SFS_ENTRYREFS; i++) {
    entry.ref[i] = 0;
  }
  result = write_entry(&entry, disk, nentry);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Delete remaining chained entries */
  if(entry.next) {
    uint current = nentry;
    uint next = entry.next;
    entry.next = 0;
    result = write_entry(&entry, disk,current);
    if(result >= ERROR_ANY) {
      return result;
    }
    do {
      current = get_entry_n(&entry, disk, next);
      if(current >= ERROR_ANY) {
        return current;
      }
      next = entry.next;
      memset(&entry, 0, sizeof(entry));
      result = write_entry(&entry, disk, current);
      if(result >= ERROR_ANY) {
        return result;
      }
    } while(next);
  }

  return 0;
}

/*
 * Set entry size value, and update also chained entries
 */
static uint set_entry_size(uint disk, uint nentry, uint size)
{
  sfs_entry_t entry;
  uint result = 0;

  /* Update chain starting from nentry */
  do {
    result = get_entry_n(&entry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    entry.size = size;
    if(entry.flags & T_FILE) {
      size -= SFS_ENTRYREFS * BLOCK_SIZE;
    } else if(entry.flags & T_DIR) {
      size -= SFS_ENTRYREFS;
    }

    result = write_entry(&entry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    nentry = entry.next;
  } while(nentry != 0);

  return 0;
}

/*
 * Given an entry, return its refcount
 */
static uint get_entry_refcount(sfs_entry_t* entry)
{
  uint refcount = 0;
  if(entry->flags & T_FILE) {
    refcount = needed_blocks((uint)entry->size);
  } else if(entry->flags & T_DIR) {
    refcount = entry->size;
  }

  return refcount;
}

/*
 * Add a reference in an entry
 * Advances throguh the chain if needed
 */
static uint add_ref_in_entry(uint disk, uint nentry, uint ref)
{
  sfs_entry_t entry;
  sfs_entry_t refentry;
  uint nrefentry = nentry;
  uint refcount = 0;
  uint result = 0;

  /* Get initial entry */
  result = get_entry_n(&entry, disk, nentry);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Get current number of references */
  refcount = get_entry_refcount(&entry);

  /* Get chained entry to add a new one */
  nrefentry = get_nref_entry_from_entry(&refentry, &entry, disk, nrefentry, refcount);
  if(nrefentry>=ERROR_ANY && nrefentry!=ERROR_NOT_FOUND) {
    return nrefentry;
  }

  /* Create if does not exist */
  if(nrefentry == ERROR_NOT_FOUND) {
    nrefentry = set_entry_refcount(disk, nentry, refcount + 1);
    if(nrefentry >= ERROR_ANY) {
      return nrefentry;
    }
    result = get_entry_n(&refentry, disk, nrefentry);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  /* Set the reference */
  refentry.ref[refcount % SFS_ENTRYREFS] = ref;
  result = write_entry(&refentry, disk, nrefentry);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Update size if it's a directory */
  if(entry.flags & T_DIR) {
    result = set_entry_size(disk, nentry, (uint)entry.size + 1);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  return 0;
}

/*
 * Remove a reference in an entry
 * Updates size and chain length if needed
 */
static uint remove_ref_in_entry(uint disk, uint nentry, uint ref)
{
  sfs_entry_t entry;
  sfs_entry_t currentry;
  sfs_entry_t nextentry;
  uint ncurrentry = nentry;
  uint refcount = 0;
  uint result = 0;
  uint r = 0;
  uchar found = 0;

  /* Get initial entry */
  result = get_entry_n(&entry, disk, nentry);
  if(result >= ERROR_ANY) {
    return result;
  }
  memcpy(&currentry, &entry, sizeof(entry));
  memcpy(&nextentry, &entry, sizeof(entry));

  /* Get total number of references */
  refcount = get_entry_refcount(&entry);

  /* Find value in reference list */
  for(r=0; r<refcount; r++) {
    if(r == SFS_ENTRYREFS) {
      result = write_entry(&currentry, disk, ncurrentry);
      if(result >= ERROR_ANY) {
        return result;
      }
      ncurrentry = get_entry_n(&currentry, disk, (uint)currentry.next);
      if(ncurrentry >= ERROR_ANY) {
        return ncurrentry;
      }
      r = 0;
      refcount = get_entry_refcount(&currentry);
    }
    else if(r + 1 == SFS_ENTRYREFS) {
      if(nextentry.next) {
        result = get_entry_n(&nextentry, disk, (uint)nextentry.next);
        if(result >= ERROR_ANY) {
          return result;
        }
      } else {
        memset(&nextentry, 0, sizeof(nextentry));
      }
    }

    if(!found && currentry.ref[r] == ref) {
      found = 1;
    }
    if(found) {
      currentry.ref[r] = nextentry.ref[(r + 1) % SFS_ENTRYREFS];
    }
  }

  /* If found, update */
  if(found) {
    result = write_entry(&currentry, disk, ncurrentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    result = set_entry_refcount(disk, nentry, refcount - 1);
    if(result >= ERROR_ANY) {
      return result;
    }
    if(entry.flags & T_DIR) {
      result = set_entry_size(disk, nentry, refcount - 1);
      if(result >= ERROR_ANY) {
        return result;
      }
    }
  }

  return 0;
}

/*
 * Write buff to file given path, offset, count and flags
 */
uint fs_write_file(uchar* buff, uchar* path, uint offset, uint count, uint flags)
{
  uint disk = 0;
  uint nentry = 0;
  sfs_entry_t entry;
  sfs_entry_t tentry;
  uint written = 0;
  uint result = 0;

  /* Find file */
  disk = path_get_disk(path);
  nentry = fs_get_entry(&entry, path, UNKNOWN_VALUE, UNKNOWN_VALUE);

  /* Does not exist and should not create or it's a directory: return */
  if((nentry == ERROR_NOT_FOUND && !(flags & WF_CREATE)) ||
    (nentry >= ERROR_ANY && nentry != ERROR_NOT_FOUND)) {
    return nentry;
  }
  if(nentry < ERROR_ANY && (entry.flags & T_DIR)) {
    return ERROR_NOT_FOUND;
  }

  /* Create file if needed */
  if(nentry == ERROR_NOT_FOUND && (flags & WF_CREATE)) {
    uint parent = 0;
    memset(&entry, 0, sizeof(entry));

    /* Parse parent, disk and name */
    result = path_parse_disk_parent_name(&path, &parent, &disk, path);
    if(result >= ERROR_ANY) {
      return result;
    }

    /* Make name a valid name */
    path = string_to_name(path);

    /* Fill entry data */
    nentry = find_free_entry(disk);
    if(nentry >= ERROR_ANY) {
      return nentry;
    }
    entry.size = 0;
    entry.next = 0;
    entry.parent = parent;
    entry.flags = T_FILE;
    strcpy_s(entry.name, path, SFS_NAMESIZE);
    result = write_entry(&entry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }

    /* Add reference in parent */
    result = add_ref_in_entry(disk, (uint)entry.parent, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  /* Resize: grow if needed */
  if(entry.size < offset + count) {
    uint current_block = needed_blocks((uint)entry.size);
    uint final_block = needed_blocks(offset + count);
    uint ntentry = 0;

    /* Set reference count and size in the chain */
    result = set_entry_refcount(disk, nentry, final_block);
    if(result >= ERROR_ANY) {
      return result;
    }
    result = set_entry_size(disk, nentry, offset + count);
    if(result >= ERROR_ANY) {
      return result;
    }

    /* Allocate blocks */
    result = get_entry_n(&tentry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    ntentry = nentry;

    for(; current_block < final_block; current_block++) {
      while(current_block > SFS_ENTRYREFS) {
        result = write_entry(&tentry, disk, ntentry);
        if(result >= ERROR_ANY) {
          return result;
        }
        ntentry = get_entry_n(&tentry, disk, (uint)tentry.next);
        if(ntentry >= ERROR_ANY) {
          return ntentry;
        }
        current_block -= SFS_ENTRYREFS;
        final_block -= SFS_ENTRYREFS;
      }
      tentry.ref[current_block] = find_free_block(disk);
      if(tentry.ref[current_block] >= ERROR_ANY) {
        return tentry.ref[current_block];
      }
    }
    result = write_entry(&tentry, disk, ntentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    result = get_entry_n(&entry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  /* Resize: shrink if needed */
  if(entry.size > offset + count && (flags & WF_TRUNCATE)) {
    uint nblocks = needed_blocks(offset + count);
    result = set_entry_refcount(disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
    result = set_entry_size(disk, nentry, offset + count);
    if(result >= ERROR_ANY) {
      return result;
    }
    result = get_entry_n(&entry, disk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  /* Now file has the right size: write data */
  written = 0;
  while(count > 0) {
    uint to_copy = min(count, BLOCK_SIZE - (offset % BLOCK_SIZE));
    uint current_block = needed_blocks(offset + to_copy) - 1;
    while(current_block > SFS_ENTRYREFS) {
      result = get_entry_n(&entry, disk, (uint)entry.next);
      if(result >= ERROR_ANY) {
        return result;
      }
      offset -= SFS_ENTRYREFS*BLOCK_SIZE;
      current_block -= SFS_ENTRYREFS;
    }

    result = write_disk(disk, (uint)entry.ref[current_block],
      offset%BLOCK_SIZE, to_copy, &buff[written]);
    if(result != 0) {
      return result;
    }
    count -= to_copy;
    offset += to_copy;
    written += to_copy;
  }

  /* Update file entry time */
  result = set_entry_time_to_current(disk, nentry);
  if(result >= ERROR_ANY) {
    return result;
  }

  return written;
}

/*
 * Delete entry by index
 * Deletes the full chain
 */
static uint delete_n(uint disk, uint n)
{
  sfs_entry_t entry;

  uint result = get_entry_n(&entry, disk, n);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Delete reference in parent */
  result = remove_ref_in_entry(disk, (uint)entry.parent, n);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Recursively delete all files if it's a directory */
  if(entry.flags & T_DIR) {
    uint b = 0;
    while(b < min(entry.size, SFS_ENTRYREFS)) {
      result = delete_n(disk, (uint)entry.ref[b]);
      if(result >= ERROR_ANY) {
        return result;
      }
      b++;
    }
  }

  /* Delete full chain */
  if(entry.next) {
    result = delete_n(disk, (uint)entry.next);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  memset(&entry, 0, sizeof(entry));
  result = write_entry(&entry, disk, n);
  if(result >= ERROR_ANY) {
    return result;
  }

  return 0;
}

/*
 * Delete entry by path
 */
uint fs_delete(uchar* path)
{
  uint disk = 0;
  uint nentry = 0;
  sfs_entry_t entry;

  /* Find entry, and delete by index */
  disk = path_get_disk(path);
  nentry = fs_get_entry(&entry, path, UNKNOWN_VALUE, UNKNOWN_VALUE);
  if(nentry < ERROR_ANY) {
    nentry = delete_n(disk, nentry);
  }

  return nentry;
}

/*
 * Create a dirrectory
 */
uint fs_create_directory(uchar* path)
{
  sfs_entry_t entry;
  uint disk = UNKNOWN_VALUE;
  uint parent = UNKNOWN_VALUE;
  uint nentry = 0;

  /* Get disk, parent entry index, and directory name from path */
  uint result = path_parse_disk_parent_name(&path, &parent, &disk, path);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Make name a valid name */
  path = string_to_name(path);

  /* Check target does not exist */
  result = fs_get_entry(&entry, path, parent, disk);
  if(result != ERROR_NOT_FOUND) {
    if(result >= ERROR_ANY) {
      return result;
    }
    return ERROR_EXISTS;
  }

  /* Find a free entry index */
  nentry = find_free_entry(disk);
  if(nentry >= ERROR_ANY) {
    return nentry;
  }

  /* Create entry */
  memset(&entry, 0, sizeof(entry));
  strcpy_s(entry.name, path, SFS_NAMESIZE);
  entry.size = 0;
  entry.flags = T_DIR;
  entry.parent = parent;
  entry.next = 0;
  result = write_entry(&entry, disk, nentry);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Update modified time */
  result = set_entry_time_to_current(disk, nentry);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Add reference in parent */
  if(nentry != entry.parent) {
    result = add_ref_in_entry(disk, (uint)entry.parent, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  return nentry;
}

/*
 * Move entry
 */
uint fs_move(uchar* srcpath, uchar* dstpath)
{
  uint result = 0;
  uint dst_parent = 0;
  uint srcdisk = 0;
  uint dstdisk = 0;
  uint nentry = 0;
  sfs_entry_t entry;
  uchar* dstname = 0;

  /* Get disk, parent entry index, and name from destination path */
  result = path_parse_disk_parent_name(&dstname, &dst_parent, &dstdisk, dstpath);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Make name a valid name */
  dstname = string_to_name(dstname);

  /* Check destination does not exist */
  result = fs_get_entry(&entry, dstname, dst_parent, dstdisk);
  if(result != ERROR_NOT_FOUND) {
    if(result >= ERROR_ANY) {
      return result;
    }
    return ERROR_EXISTS;
  }

  /* Check source exists */
  srcdisk = path_get_disk(srcpath);
  nentry = fs_get_entry(&entry, srcpath, UNKNOWN_VALUE, UNKNOWN_VALUE);
  if(nentry >= ERROR_ANY) {
    return nentry;
  }

  /* If moving between disks, physically move data: copy and delete */
  if(srcdisk != dstdisk) {
    result = fs_copy(srcpath, dstpath);
    if(result != 0) {
      return result;
    }
    result = delete_n(srcdisk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
  } else {
    /* Else, remove reference in parent */
    result = remove_ref_in_entry(srcdisk, (uint)entry.parent, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }

    /* Update entry parent and name */
    memset(entry.name, 0, sizeof(entry.name));
    strcpy_s(entry.name, dstname, SFS_NAMESIZE);
    entry.parent = dst_parent;
    result = write_entry(&entry, dstdisk, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }

    /* Add reference in parent */
    result = add_ref_in_entry(dstdisk, (uint)entry.parent, nentry);
    if(result >= ERROR_ANY) {
      return result;
    }
  }

  return nentry;
}

/*
 * Copy entry
 */
uint fs_copy(uchar* srcpath, uchar* dstpath)
{
  uint result = 0;
  uint dst_parent = 0;
  uint src_disk = 0;
  uint dst_disk = 0;
  uint nentry = 0;
  sfs_entry_t entry;
  uchar* dstname = 0;

  /* Get disk, parent entry index, and name from destination path */
  result = path_parse_disk_parent_name(&dstname, &dst_parent, &dst_disk, dstpath);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Make name a valid name */
  dstname = string_to_name(dstname);

  /* Check destination does not exist */
  result = fs_get_entry(&entry, dstname, dst_parent, dst_disk);
  if(result != ERROR_NOT_FOUND) {
    if(result >= ERROR_ANY) {
      return result;
    }
    return ERROR_EXISTS;
  }

  /* Check source exists */
  src_disk = path_get_disk(srcpath);
  nentry = fs_get_entry(&entry, srcpath, UNKNOWN_VALUE, UNKNOWN_VALUE);
  if(nentry >= ERROR_ANY) {
    return nentry;
  }

  /* If source is a file, just read and write the full file */
  if(entry.flags & T_FILE) {
    uint offset = 0;
    uint copied = 0;
    uchar buff[BLOCK_SIZE];

    while(copied = fs_read_file(buff, srcpath, offset, sizeof(buff))) {
      if(copied >= ERROR_ANY) {
        return copied;
      }
      result = fs_write_file(buff, dstpath, offset, copied, WF_CREATE);
      if(result >= ERROR_ANY) {
        return result;
      }
      offset += copied;
    }
    return 0;
  }
  /* If source is a directory */
  else if(entry.flags & T_DIR) {
    uint r = 0;

    /* Create the directory */
    result = fs_create_directory(dstpath);
    if(result >= ERROR_ANY) {
      return result;
    }

    /* Iterate and recursively copy entries */
    for(r=0; r<entry.size; r++) {
      sfs_entry_t tentry;
      uchar src_path[64];
      uchar dst_path[64];
      result = get_entry_n(&tentry, src_disk, (uint)entry.ref[r]);
      if(result >= ERROR_ANY) {
        return result;
      }

      /* Compute src and dest paths, and move */
      strcpy_s(src_path, srcpath, sizeof(src_path));
      strcat_s(src_path, PATH_SEPARATOR_S, sizeof(src_path));
      strcat_s(src_path, tentry.name, sizeof(src_path));

      strcpy_s(dst_path, dstpath, sizeof(src_path));
      strcat_s(dst_path, PATH_SEPARATOR_S, sizeof(dst_path));
      strcat_s(dst_path, tentry.name, sizeof(dst_path));

      result = fs_copy(src_path, dst_path);
      if(result >= ERROR_ANY) {
        return result;
      }
    }
    return 0;
  }

  return ERROR_NOT_FOUND;
}

/*
 * List entries in a directory
 */
uint fs_list(sfs_entry_t* entry, uchar* path, uint n)
{
  uint nentry = 0;
  uint disk = 0;
  sfs_entry_t direntry;

  /* Find entry */
  disk = path_get_disk(path);
  memset(entry, 0, sizeof(sfs_entry_t));
  nentry = fs_get_entry(&direntry, path, UNKNOWN_VALUE, UNKNOWN_VALUE);
  if(nentry >= ERROR_ANY) {
    return nentry;
  }

  /* If found, and it's a directory */
  if(direntry.flags & T_DIR) {
    if(n < (uint)direntry.size) {
       /* Advance to the chained entry contaning the nth reference */
      uint res = get_nref_entry_from_entry(&direntry, &direntry, disk, nentry, n);
      if(res >= ERROR_ANY) {
        return res;
      }
      /* Get the entry */
      res = get_entry_n(entry, disk, (uint)direntry.ref[n]);
      if(res >= ERROR_ANY) {
        return res;
      }
    }
    return (uint)direntry.size;
  }
  return ERROR_NOT_FOUND;
}

/*
 * Format a disk
 */
uint fs_format(uint disk)
{
  uchar k_src_path[32];
  uchar k_dst_path[32];
  uchar buff[BLOCK_SIZE];
  sfs_entry_t* entry;
  sfs_superblock_t* sb;
  uint nentries = 0;
  uint result = 0;
  uint offset = 0;
  uint e = 0;
  uint32_t disk_size = 0;
  uint disk_index = disk_to_index(disk);

  debugstr("format disk: %x (system_disk=%x)\n\r", disk, system_disk);

  /* Copy boot block from system disk to target disk */
  result = read_disk(system_disk, 0, 0, BLOCK_SIZE, buff);
  if(result != 0) {
    return ERROR_IO;
  }

  result = write_disk(disk, 0, 0, BLOCK_SIZE, buff);
  if(result != 0) {
    return ERROR_IO;
  }

  /* Create superblock */
  disk_size = (uint32_t)disk_info[disk_index].sectors *
    (uint32_t)disk_info[disk_index].sides *
    (uint32_t)disk_info[disk_index].cylinders;

  if(SECTOR_SIZE > BLOCK_SIZE) {
    disk_size *= (uint32_t)(SECTOR_SIZE/BLOCK_SIZE);
  } else {
    disk_size /= (uint32_t)(BLOCK_SIZE/SECTOR_SIZE);
  }

  memset(buff, 0, sizeof(buff));
  sb = (sfs_superblock_t*)buff;
  sb->type = SFS_TYPE_ID;
  sb->size = disk_size;
  sb->nentries = min(
    (uint32_t)(((sb->size * (uint32_t)BLOCK_SIZE)/10L)/(uint32_t)sizeof(sfs_entry_t)),
    1024L);
  sb->bootstart = 2L + (sb->nentries * (uint32_t)sizeof(sfs_entry_t)) / (uint32_t)BLOCK_SIZE;
  result = write_disk(disk, 1, 0, BLOCK_SIZE, sb);
  if(result != 0) {
    return ERROR_IO;
  }
  debugstr("format: %x blocks=%U entries=%U boot=%U\n\r", disk, sb->size, sb->nentries, sb->bootstart);

  nentries = (uint)sb->nentries;

  /* Create root dir */
  memset(buff, 0, sizeof(buff));
  entry = (sfs_entry_t*)buff;
  strcpy_s(entry->name, ROOT_DIR_NAME, SFS_NAMESIZE);
  entry->flags = T_DIR;
  entry->size = 0;
  entry->parent = 0;
  entry->next = 0;
  entry->time = 0;

  result = write_entry(entry, disk, 0);
  if(result >= ERROR_ANY) {
    return result;
  }

  result = set_entry_time_to_current(disk, 0);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Write empty entries table */
  memset(buff, 0, sizeof(buff));
  for(e=1; e<nentries; e++) {
    write_entry(entry, disk, e);
  }

  /* Copy boot program */
  result = get_entry_n(entry, system_disk, 1);
  if(result >= ERROR_ANY) {
    return result;
  }

  strcpy_s(k_src_path, disk_to_string(system_disk), sizeof(k_src_path));
  strcat_s(k_src_path, PATH_SEPARATOR_S, sizeof(k_src_path));
  strcat_s(k_src_path, entry->name, sizeof(k_src_path));
  strcpy_s(k_dst_path, disk_to_string(disk), sizeof(k_dst_path));
  strcat_s(k_dst_path, PATH_SEPARATOR_S, sizeof(k_dst_path));
  strcat_s(k_dst_path, entry->name, sizeof(k_dst_path));

  debugstr("format: copy %s %s\n\r", k_src_path, k_dst_path);

  result = fs_copy(k_src_path, k_dst_path);
  if(result >= ERROR_ANY) {
    return result;
  }

  /* Update disks file system information */
  fs_init_info();

  return result;
}

/*
 * Convert fs-time to system TIME
 */
uint fs_fstime_to_systime(uint32_t fst, time_t* syst)
{
  syst->year   = ((fst >> 22 ) & 0x3FF) / 12 + 2017;
  syst->month  = ((fst >> 22 ) & 0x3FF) % 12 + 1;
  syst->day    = ((fst & 0x3FFFFFL) / 86400L) + 1;  /* 86400=24*60*60 */
  syst->hour   = ((fst & 0x3FFFFFL) / 3600L) % 24;  /* 3600=60*60 */
  syst->minute = ((fst & 0x3FFFFFL) / 60) % 60;
  syst->second = (fst & 0x3FFFFFL) % 60;
  return 0;
}

/*
 * Convert system TIME to fs-time
 */
uint32_t fs_systime_to_fstime(time_t* syst)
{
  uint32_t fst = syst->second;
  fst += ((uint32_t)syst->minute)*60;
  fst += ((uint32_t)syst->hour)*60*60;
  fst += ((uint32_t)(syst->day-1))*24*60*60;
  fst &= 0x3FFFFFL;
  fst |= ((uint32_t)(syst->year-2017)*12 + (syst->month-1)) << 22;
  return fst;
}
