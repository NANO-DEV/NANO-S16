/*
 * EXT2 filesystem
 */

#include "../types.h"
#include "../kernel.h"
#include "../utils.h"
#include "ext2.h"
#include "fs.h"
#include "../hw86.h"
#include "../clib/clib.h"

#define EXT2_ROOT_INODE   2
#define SECTOR_SIZE     512

static struct EXT2_SUPERBLOCK ext2_superblock[4];
static int                    ext2_block_size[4]     = 0;
static int                    ext2_fragment_size[4]  = 0;
static int                    ext2_n_block_groups[4] = 0;
static int                    cdri = 0;
static int                    cdrc = 0;

static struct EXT2_CWD {
    char      path[FS_MAX_PATH];
    int       inode;
} fs_cwd;

struct PATH_ELEMENT {
    struct PATH_ELEMENT*    next;
    struct PATH_ELEMENT*    prev;
    char                    name[FS_NAME_LEN];
};

/*
 * Read disk, specific sector, offset and size
 */
static int read_disk_sector(int sector, int offset, int buff_size, char* buff)
{
    char aux_buff[SECTOR_SIZE];
    int n_sectors = 0;
    int i = 0;
    int result = 0;

    sector += offset / SECTOR_SIZE;
    offset = mod(offset, SECTOR_SIZE);

    if(buff == 0) {
        sprintf("Read Disk: Bad buffer\n\r");
        return 1;
    }

    if(offset) {
        result = read_disk(cdrc, sector, 1, aux_buff);
        i = min(SECTOR_SIZE-offset, buff_size);
        memcpy(buff, &aux_buff[offset], i);
        sector++;
        buff_size -= i;
    }

    n_sectors = buff_size / SECTOR_SIZE;

    if(n_sectors && result == 0) {
        result = read_disk(cdrc, sector, n_sectors, &buff[i]);
        /* Handle DMA access 64kb boundary */
        if(result == 0x900) {
            sprintf("Read Disk: DMA access accross 64Kb boundary. Using block by block mode.\n\r");
            for(; n_sectors > 0; n_sectors--) {
                result = read_disk(cdrc, sector, 1, aux_buff);
                if(result != 0) {
                    break;
                }
                memcpy(&buff[i], aux_buff, SECTOR_SIZE);
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

    if(buff_size && result == 0) {
        result = read_disk(cdrc, sector, 1, aux_buff);
        memcpy(&buff[i], aux_buff, buff_size);
    }

    if(result != 0) {
        sprintf("Read Disk Sector Result %x\n\r", result);
    }

    return result;
}

/*
 * Write disk, specific sector, offset and size
 */
static int write_disk_sector(int sector, int offset, int buff_size, char* buff)
{
    char aux_buff[SECTOR_SIZE];
    int n_sectors = 0;
    int i = 0;
    int result = 0;

    sector += offset / SECTOR_SIZE;
    offset = mod(offset, SECTOR_SIZE);

    if(buff == 0) {
        sprintf("Disk write: Bad buffer\n\r");
        return 1;
    }

    if(offset) {
        result += read_disk(cdrc, sector, 1, aux_buff);
        i = min(SECTOR_SIZE-offset, buff_size);
        memcpy(&aux_buff[offset], buff, i);
        result += write_disk(cdrc, sector, 1, aux_buff);
        sector++;
        buff_size -= i;
    }

    n_sectors = buff_size / SECTOR_SIZE;

    if(n_sectors && result == 0) {
        result += write_disk(cdrc, sector, n_sectors, &buff[i]);
        /* Handle DMA access 64kb boundary */
        if(result == 0x900) {
            sprintf("Write Disk: DMA access accross 64Kb boundary. Using block by block mode.\n\r");
            for(; n_sectors > 0; n_sectors--) {
                memcpy(aux_buff, &buff[i], SECTOR_SIZE);
                result = write_disk(cdrc, sector, 1, aux_buff);
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

    if(buff_size && result == 0) {
        result += read_disk(cdrc, sector, 1, aux_buff);
        memcpy(aux_buff, &buff[i], buff_size);
        result += write_disk(cdrc, sector, 1, aux_buff);
    }

    if(result != 0) {
        sprintf("Write Disk Sector Result %x\n\r", result);
    }

    return result;
}

/*
 * Read the superblock
 */
static int ext2_read_superblock(struct EXT2_SUPERBLOCK* buff)
{
    int rd = read_disk_sector(2, 0, sizeof(struct EXT2_SUPERBLOCK), buff);

    /* Check signature */
    if(rd == 0 && buff->signature == 0xEF53) {
        /* Calculate and store the block size */
        ext2_block_size[cdri] = 1024 << buff->block_size;
        /* Calculate and store the fragment size */
        ext2_fragment_size[cdri] = 1024 << buff->fragment_size;
        /* Calculate number of block groups */
        ext2_n_block_groups[cdri] = max(1, (int)ext2_superblock[cdri].blocks /
                            (int)ext2_superblock[cdri].blocks_per_group );

        return 0;
    }

    return 1;
}

/*
 * Write the superblock
 */
static int ext2_write_superblock(struct EXT2_SUPERBLOCK* buff)
{
    return write_disk_sector(2, 0, sizeof(struct EXT2_SUPERBLOCK), buff);
}

/*
 * Read block (translate to sector)
 */
static int ext2_read_block(int block, int offset, int buff_size, char* buff)
{
    int s = block * (ext2_block_size[cdri] / SECTOR_SIZE);
    return read_disk_sector(s, offset, buff_size, buff);
}

/*
 * Write block (translate to sector)
 */
static int ext2_write_block(int block, int offset, int buff_size, char* buff)
{
    int s = block * (ext2_block_size[cdri] / SECTOR_SIZE);
    return write_disk_sector(s, offset, buff_size, buff);
}

/*
 * Read a block group descriptor
 */
static int ext2_read_bgdesc(int block_group, struct EXT2_BGDESC* buff)
{
    int block = (int)ext2_superblock[cdri].superblock_block + 1 +
                block_group * sizeof(struct EXT2_BGDESC) / ext2_block_size[cdri];
    int offset = mod(block_group * sizeof(struct EXT2_BGDESC), ext2_block_size[cdri]);
    return ext2_read_block(block, offset, sizeof(struct EXT2_BGDESC), buff);
}

/*
 * Write a block group descriptor
 */
static int ext2_write_bgdesc(int block_group, struct EXT2_BGDESC* buff)
{
    int block = (int)ext2_superblock[cdri].superblock_block + 1 +
                block_group * sizeof(struct EXT2_BGDESC) / ext2_block_size[cdri];
    int offset = mod(block_group * sizeof(struct EXT2_BGDESC), ext2_block_size[cdri]);
    return ext2_write_block(block, offset, sizeof(struct EXT2_BGDESC), buff);
}

/*
 * Read inode
 */
static int ext2_read_inode(int inode, struct EXT2_INODE* buff)
{
    struct EXT2_BGDESC ext2_bgdesc;

    int block_group = (inode - 1) / (int)ext2_superblock[cdri].inodes_per_group;
    int result = ext2_read_bgdesc(block_group, &ext2_bgdesc);

    if(result == 0) {
        int table_index = mod(inode - 1, (int)ext2_superblock[cdri].inodes_per_group);
        int inode_block = (int)ext2_bgdesc.inode_table_block +
            (table_index * (int)ext2_superblock[cdri].inode_size) / ext2_block_size[cdri];
        int offset = mod(table_index * (int)ext2_superblock[cdri].inode_size, ext2_block_size[cdri]);

        result = ext2_read_block(inode_block, offset, sizeof(struct EXT2_INODE), buff);
    }

    return result;
}

/*
 * Write inode
 */
static int ext2_write_inode(int inode, struct EXT2_INODE* buff)
{
    struct EXT2_BGDESC ext2_bgdesc;

    int block_group = (inode - 1) / (int)ext2_superblock[cdri].inodes_per_group;
    int result = ext2_read_bgdesc(block_group, &ext2_bgdesc);

    if(result == 0) {
        int table_index = mod(inode - 1, (int)ext2_superblock[cdri].inodes_per_group);
        int inode_block =  (int)ext2_bgdesc.inode_table_block +
            (table_index * (int)ext2_superblock[cdri].inode_size) / ext2_block_size[cdri];
        int offset = mod(table_index * (int)ext2_superblock[cdri].inode_size, ext2_block_size[cdri]);

        result += ext2_write_block(inode_block, offset, sizeof(struct EXT2_INODE), buff);
    }

    return result;
}

/*
 * Find and allocate first free block
 */
static int ext2_allocate_first_free_block(int* block)
{
    struct EXT2_BGDESC ext2_bgdesc;
    int found = 0;
    int result = 0;
    int offset_aux = 0;
    char* aux_buff = malloc(ext2_block_size[cdri]);
    int block_group = 0;
    *block = 0;
    for(block_group=0; block_group<ext2_n_block_groups[cdri] && !(*block) && result==0; block_group++) {
        result += ext2_read_bgdesc(block_group, &ext2_bgdesc);
        result += ext2_read_block((int)ext2_bgdesc.usage_bitmap_block, 0, ext2_block_size[cdri], aux_buff);
        for(offset_aux=0; offset_aux<(int)ext2_superblock[cdri].blocks_per_group && result==0; offset_aux++) {
            if(block_group == 0 && offset_aux < 8) {
                offset_aux = 8;
                continue;
            }
            if((aux_buff[offset_aux/8] & (1 << mod(offset_aux, 8))) == 0) {
                aux_buff[offset_aux/8] |= (1 << mod(offset_aux, 8));
                result += ext2_write_block((int)ext2_bgdesc.usage_bitmap_block, 0, ext2_block_size[cdri], aux_buff);

                ext2_bgdesc.unallocated_blocks--;
                result += ext2_write_bgdesc(block_group, &ext2_bgdesc);

                ext2_superblock[cdri].unallocated_blocks = (int)ext2_superblock[cdri].unallocated_blocks - 1;
                result += ext2_write_superblock(&ext2_superblock[cdri]);

                *block = block_group * (int)ext2_superblock[cdri].blocks_per_group + offset_aux + 2;
                found = 1;
                break;
            }
        }
    }

    free(aux_buff);

    if(found == 0) {
        result = 1;
    }
    return result;
}

/*
 * Free block
 */
static int ext2_free_block(int block)
{
    struct EXT2_BGDESC ext2_bgdesc;

    int block_group = block / (int)ext2_superblock[cdri].blocks_per_group;
    int result = ext2_read_bgdesc(block_group, &ext2_bgdesc);

    if(result == 0) {
        char* aux_buff = malloc(ext2_block_size[cdri]);
        result += ext2_read_block((int)ext2_bgdesc.usage_bitmap_block, 0, ext2_block_size[cdri], aux_buff);
        if(result == 0) {
            int offset_aux = mod(block, (int)ext2_superblock[cdri].blocks_per_group);
            aux_buff[offset_aux/8] &= (0xFF - (1 << mod(offset_aux, 8)));
            result += ext2_write_block((int)ext2_bgdesc.usage_bitmap_block, 0, ext2_block_size[cdri], aux_buff);
            free(aux_buff);

            ext2_bgdesc.unallocated_blocks++;
            result += ext2_write_bgdesc(block_group, &ext2_bgdesc);

            ext2_superblock[cdri].unallocated_blocks = (int)ext2_superblock[cdri].unallocated_blocks + 1;
            result += ext2_write_superblock(&ext2_superblock[cdri]);
        }
    }

    return result;
}

/*
 * Set inode indirect data size, recursive with level
 */
static int ext2_set_inode_indirect_data_size(int* block, int n_blocks, int* n_done, int level)
{
    int result = 0;
    int n_done_aux = *n_done;

    /* Allocate block if needed */
    if(n_blocks - *n_done > 0 && *block == 0) {
        char* aux_buff = malloc(ext2_block_size[cdri]);
        int block_group = 0;
        result += ext2_allocate_first_free_block(&block_group);
        *block = block_group;

        if(result == 0) {
            memset(aux_buff, 0, ext2_block_size[cdri]);
            result += ext2_write_block(*block, 0, ext2_block_size[cdri], aux_buff);
        }
        free(aux_buff);
    }

    /* Fill block references if needed */
    if(*block != 0 && result == 0) {
        int i = 0;
        long int* indirect_buffer = malloc(ext2_block_size[cdri]);
        result += ext2_read_block(*block, 0, ext2_block_size[cdri], indirect_buffer);
        for(i=0; i<ext2_block_size[cdri]/4 && result==0; i++) {
            if(level <= 1) {
                if(*n_done < n_blocks) {
                    *n_done++;
                    if((int)indirect_buffer[i] == 0) {
                        int block_group = 0;
                        result += ext2_allocate_first_free_block(&block_group);
                        indirect_buffer[i] = block_group;
                    }
                } else if((int)indirect_buffer[i] > 0) {
                    result += ext2_free_block((int)indirect_buffer[i]);
                    indirect_buffer[i] = 0;
                }
            } else {
                int t = (int)indirect_buffer[i];
                result += ext2_set_inode_indirect_data_size(&t, n_blocks, n_done, level - 1);
                indirect_buffer[i] = t;
            }
        }
        result += ext2_write_block(*block, 0, ext2_block_size[cdri], indirect_buffer);
        free(indirect_buffer);
    }

    /* Free block if needed */
    if(n_blocks - n_done_aux <= 0 && *block != 0 && result == 0) {
        result += ext2_free_block(*block);
        *block = 0;
    }

    return result;
}

/*
 * Free inode
 */
static int ext2_free_inode(int inode, int block_group, struct EXT2_BGDESC* ext2_bgdesc)
{
    char* buff = malloc(ext2_block_size[cdri]);

    int result = ext2_read_block((int)ext2_bgdesc->inode_usage_block, 0, ext2_block_size[cdri], buff);
    if(result == 0) {
        int table_index = mod((int)inode - 1, (int)ext2_superblock[cdri].inodes_per_group);
        buff[table_index/8] &= (0xFF - (1 << mod(table_index, 8)));
        result += ext2_write_block((int)ext2_bgdesc->inode_usage_block, 0, ext2_block_size[cdri], buff);
    }

    free(buff);

    ext2_bgdesc->unallocated_inodes++;
    result += ext2_write_bgdesc(block_group, ext2_bgdesc);

    ext2_superblock[cdri].unallocated_inodes = (int)ext2_superblock[cdri].unallocated_inodes + 1;
    result += ext2_write_superblock(&ext2_superblock[cdri]);

    return result;
}

/*
 * Set inode size
 */
static int ext2_set_inode_data_size(int inode, int size, int free_inode)
{
    struct EXT2_BGDESC ext2_bgdesc;
    struct EXT2_INODE  ext2_inode;
    int i = 0;
    int n_blocks = 0;
    int n_done = 0;

    int block_group = (inode - 1) / (int)ext2_superblock[cdri].inodes_per_group;
    int result = ext2_read_bgdesc(block_group, &ext2_bgdesc);

    int table_index = mod(inode - 1, (int)ext2_superblock[cdri].inodes_per_group);
    int inode_block = (int)ext2_bgdesc.inode_table_block +
            (table_index * (int)ext2_superblock[cdri].inode_size) / ext2_block_size[cdri];
    int offset = mod(table_index * (int)ext2_superblock[cdri].inode_size, ext2_block_size[cdri]);
    result += ext2_read_block(inode_block, offset, sizeof(struct EXT2_INODE), &ext2_inode);

    /* Free this inode if requested */
    if(free_inode != 0) {
        ext2_inode.hard_links--;
        if(ext2_inode.hard_links <= 0) {
            result += ext2_free_inode(inode, block_group, &ext2_bgdesc);
            size = 0;
        }
    }

    /* Direct blocks */
    n_blocks = size / ext2_block_size[cdri];
    n_blocks += mod(size, ext2_block_size[cdri]) ? 1 : 0;
    for(i=0; i<12 && result == 0; i++) {
        if(i < n_blocks) {
            if((int)ext2_inode.direct_block[i] == 0) {
                block_group = 0;
                result += ext2_allocate_first_free_block(&block_group);
                ext2_inode.direct_block[i] = block_group;
            }
        } else if((int)ext2_inode.direct_block[i] > 0) {
            result += ext2_free_block((int)ext2_inode.direct_block[i]);
            ext2_inode.direct_block[i] = 0;
        }
    }

    /* Indirect blocks */
    if(result == 0) {
        n_done = min(n_blocks, 12);
        i = (int)ext2_inode.single_block;
        result += ext2_set_inode_indirect_data_size(&i, n_blocks, &n_done, 1);
        ext2_inode.single_block = i;
    }
    if(result == 0) {
        i = (int)ext2_inode.double_block;
        result += ext2_set_inode_indirect_data_size(&i, n_blocks, &n_done, 2);
        ext2_inode.double_block = i;
    }
    if(result == 0) {
        i = (int)ext2_inode.triple_block;
        result += ext2_set_inode_indirect_data_size(&i, n_blocks, &n_done, 3);
        ext2_inode.triple_block = i;
    }
    if(result == 0) {
        ext2_inode.low_size = size;
        result += ext2_write_block(inode_block, offset, sizeof(struct EXT2_INODE), &ext2_inode);
    }
    return result;
}

/*
 * Load data from inode indirect block, level recursive
 */
static int ext2_read_indirect_data(int block, int* bytes_read, int* read_limit, char* buffer, int level)
{
    int result = 0;
    int i;

    if(*bytes_read < *read_limit && block != 0) {
        long int* indirect_buffer = malloc(ext2_block_size[cdri]);
        result += ext2_read_block(block, 0, ext2_block_size[cdri], indirect_buffer);

        for(i=0; *bytes_read<*read_limit && i<ext2_block_size[cdri]/4 && result==0; i++) {
            if((int)indirect_buffer[i] != 0) {
                if(level == 1) {
                    int read_size = min(*read_limit - *bytes_read, ext2_block_size[cdri]);
                    result += ext2_read_block((int)indirect_buffer[i],
                        0, read_size, &(buffer[*bytes_read]));
                    *bytes_read += read_size;
                } else {
                    result += ext2_read_indirect_data((int)indirect_buffer[i],
                        bytes_read, read_limit, buffer, level - 1);
                }
            }
        }
        free(indirect_buffer);
    }
    return result;
}

/*
 * Load data from inode
 */
static int ext2_read_inode_data(struct EXT2_INODE* ext2_inode, int offset, int buff_size, char* buffer)
{
    int result = 0;
    int bytes_read = 0;
    int read_limit = min(buff_size, (int)ext2_inode->low_size - offset);
    int read_size = 0;
    int i = 0;
    for(; bytes_read<read_limit && i<12 && result==0; i++) {
        if((int)ext2_inode->direct_block[i] > 0) {
            if(offset > ext2_block_size[cdri]) {
                offset -= ext2_block_size[cdri];
            } else {
                read_size = min(read_limit - bytes_read, ext2_block_size[cdri] - offset);
                result += ext2_read_block((int)(ext2_inode->direct_block[i]),
                    offset, read_size, &(buffer[bytes_read]));

                bytes_read += read_size;
                offset = 0;
            }
        }
    }

    if(bytes_read < read_limit && (int)ext2_inode->single_block != 0 && result == 0) {
        result += ext2_read_indirect_data((int)ext2_inode->single_block,
                &bytes_read, &read_limit, buffer, 1);
    }

    if(bytes_read < read_limit && (int)ext2_inode->double_block != 0 && result == 0) {
        result += ext2_read_indirect_data((int)ext2_inode->double_block,
                &bytes_read, &read_limit, buffer, 2);
    }

    if(bytes_read < read_limit && (int)ext2_inode->triple_block != 0 && result == 0) {
        result += ext2_read_indirect_data((int)ext2_inode->triple_block,
                &bytes_read, &read_limit, buffer, 3);
    }

    return result;
}

/*
 * Write data to inode indirect block
 */
static int ext2_write_indirect_data(int block, int* bytes_wr, int* wr_limit, char* buffer, int level)
{
    int result = 0;
    int i = 0;

    if(*bytes_wr < *wr_limit && block != 0) {
        long int* indirect_buffer = malloc(ext2_block_size[cdri]);
        result += ext2_read_block(block, 0, ext2_block_size[cdri], (char*)indirect_buffer);

        for(i=0; *bytes_wr<*wr_limit && i<ext2_block_size[cdri]/4 && result==0; i++) {
            if((int)indirect_buffer[i] != 0) {
                if(level == 1) {
                    int wr_size = min(*wr_limit - *bytes_wr, ext2_block_size[cdri]);
                    result += ext2_write_block((int)indirect_buffer[i],
                        0, wr_size, &(buffer[*bytes_wr]));
                    *bytes_wr += wr_size;
                } else {
                    result += ext2_write_indirect_data((int)indirect_buffer[i],
                        bytes_wr, wr_limit, buffer, level - 1);
                }
            }
        }
        free(indirect_buffer);
    }

    return result;
}

/*
 * Write data to inode
 */
static int ext2_write_inode_data(struct EXT2_INODE* ext2_inode, int offset, int buff_size, char* buffer)
{
    int result = 0;
    int bytes_wr = 0;
    int wr_limit = min(buff_size, (int)ext2_inode->low_size - offset);
    int wr_size = 0;
    int i = 0;
    for(i=0; bytes_wr<wr_limit && i<12 && result==0; i++) {
        if((int)ext2_inode->direct_block[i] > 0) {
            if(offset > ext2_block_size[cdri]) {
                offset -= ext2_block_size[cdri];
            } else {
                wr_size = min(wr_limit - bytes_wr, ext2_block_size[cdri] - offset);
                result += ext2_write_block((int)(ext2_inode->direct_block[i]),
                    offset, wr_size, &(buffer[bytes_wr]));

                bytes_wr += wr_size;
                offset = 0;
            }
        }
    }

    if(bytes_wr < wr_limit && (int)ext2_inode->single_block != 0 && result == 0) {
        result += ext2_write_indirect_data((int)ext2_inode->single_block,
                &bytes_wr, &wr_limit, buffer, 1);
    }

    if(bytes_wr < wr_limit && (int)ext2_inode->double_block != 0 && result == 0) {
        result += ext2_write_indirect_data((int)ext2_inode->double_block,
                &bytes_wr, &wr_limit, buffer, 2);
    }

    if(bytes_wr < wr_limit && (int)ext2_inode->triple_block != 0 && result == 0) {
        result += ext2_write_indirect_data((int)ext2_inode->triple_block,
                &bytes_wr, &wr_limit, buffer, 3);
    }

    return result;
}

/*
 * Is entry
 */
static int ext2_is_entry(char* name, struct EXT2_DIRENT* ext2_dirent)
{
    return strncmp(&(ext2_dirent->name_start), name,
        max(strlen(name), ext2_dirent->name_length)) ? 0 : 1;
}

/*
 * Find entry
 */
static int ext2_find_entry(int inode, struct PATH_ELEMENT* path, struct EXT2_DIRENT** direntry)
{
    int bytes_read = 0;

    struct EXT2_INODE ext2_inode;
    struct EXT2_DIRENT* ext2_dirent;
    int result = ext2_read_inode(inode, &ext2_inode);

    char* buffer = malloc((int)ext2_inode.low_size);
    if(buffer == 0) {
        return 1;
    }

    result += ext2_read_inode_data(&ext2_inode, 0, (int)ext2_inode.low_size, buffer);
    *direntry = 0;

    for(;bytes_read<(int)ext2_inode.low_size && result==0; bytes_read+=(int)ext2_dirent->size) {
        ext2_dirent = &buffer[bytes_read];
        if((int)ext2_dirent->inode > 0) {
            if((path == 0 && strncmp(&(ext2_dirent->name_start), ".", (int)ext2_dirent->name_length) == 0) ||
               (path != 0 && ext2_is_entry(path->name, ext2_dirent))) {
                if(path != 0 && path->next != 0) {
                    inode = (int)ext2_dirent->inode;
                    free(buffer);
                    buffer = 0;
                    result += ext2_find_entry(inode, path->next, direntry);
                } else {
                    *direntry = malloc((int)ext2_dirent->size);
                    if(*direntry != 0) {
                        memcpy(*direntry, ext2_dirent, (int)ext2_dirent->size);
                    }
                    free(buffer);
                    buffer = 0;
                }
                break;
            }
        }
    }

    if(*direntry == 0) {
        result++;
    }

    if(buffer) {
        free(buffer);
    }

    return result;
}

/*
 * Release path
 */
static void path_free(struct PATH_ELEMENT* pe)
{
    struct PATH_ELEMENT* pt = pe;

    while(pt) {
        pe = pe->next;
        free(pt);
        pt = pe;
    }
}

/*
 * Get parent path
 */
static int get_parent_path(struct PATH_ELEMENT* path, struct PATH_ELEMENT** parent)
{
    struct PATH_ELEMENT* parent_path_aux = 0;
    struct PATH_ELEMENT* parent_path_aux_p = 0;
    struct PATH_ELEMENT* path_aux = path;
    *parent = 0;

    while(path_aux != 0 && path_aux->next != 0 ) {
        parent_path_aux_p = parent_path_aux;
        parent_path_aux = malloc(sizeof(struct PATH_ELEMENT));
        if(parent_path_aux == 0) {
            return 1;
        }
        parent_path_aux->next = 0;
        parent_path_aux->prev = parent_path_aux_p;
        if(parent_path_aux_p) {
            parent_path_aux_p->next = parent_path_aux;
        }
        strcpy(parent_path_aux->name, path_aux->name);
        if(*parent == 0) {
            *parent = parent_path_aux;
        }
        path_aux = path_aux->next;
    }

    return 0;
}

/*
 * Parse path
 */
static struct PATH_ELEMENT* path_parse(char* path, char* parsed)
{
    int i = 0;
    int j = 0;
    struct PATH_ELEMENT* pe = 0;
    struct PATH_ELEMENT* pt = 0;

    if(parsed != 0) {
        parsed[0] = 0;
    }
    if(path == 0 || parsed == 0) {
        return 0;
    }

    /* Remove initial spaces */
    while(path[i] == ' ') {
        i++;
    }

    /* Add CWD if needed */
    if(path[i] != '/') {
        j += strcpy(parsed, fs_cwd.path);
        if(parsed[j-1] != '/') {
            parsed[j] = '/';
            j++;
            parsed[j] = 0;
        }
    }

    /* Copy path */
    strcpy(&parsed[j], &path[i]);

    /* Remove final spaces */
    j = strlen(parsed) - 1;
    while(j >= 0 && parsed[j] == ' ') {
        parsed[j] = 0;
        j--;
    }

    /* Parse */
    pe = malloc(sizeof(struct PATH_ELEMENT));
    pe->next = 0;
    pe->prev = 0;
    pe->name[0] = 0;
    pt = pe;
    j=1;
    while(pe != 0 && pt != 0) {
        i=0;
        while(parsed[j] != 0 && parsed[j] != '/') {
            pt->name[i] = parsed[j];
            i++;
            j++;
        }

        pt->name[i] = 0;

        if(parsed[j] == '/') {
            pt->next = malloc(sizeof(struct PATH_ELEMENT));
            pt->next->prev = pt;
            pt = pt->next;
            pt->next = 0;
            pt->name[0] = 0;

            j++;
            continue;
        }

        if(parsed[j] == 0) {
            if(pt->name[0] == 0) {
                pt->prev->next = 0;
                free(pt);
                if(pt == pe) {
                    pe = 0;
                }
            }
            break;
        }
    }

    if(pe->name[0] == 0 && pe != 0) {
        free(pe);
        pe = 0;
    }

    return pe;
}

/*
 * Simplify path, export string
 */
static struct PATH_ELEMENT* path_simplify_tos(char* path, struct PATH_ELEMENT* pe)
{
    struct PATH_ELEMENT* pi;
    struct PATH_ELEMENT* pt = pe;

    /* Remove parent dir (..) and current dir (.) references */
    while(pt) {
        if(strcmp(pt->name, ".") == 0) {
            if(pt->prev != 0) {
                pt->prev->next = pt->next;
            } else {
                pe = pt->next;
            }

            if(pt->next) {
                pt->next->prev = pt->prev;
            }
            pi = pt->next;
            free(pt);
            pt = pi;
            continue;
        }
        else if(strcmp(pt->name, "..") == 0) {
            if(pt->prev != 0 && pt->prev->prev != 0) {
                pt->prev->prev->next = pt->next;
                if(pt->next) {
                    pt->next->prev = pt->prev->prev;
                }
            } else {
                pe = pt->next;
                if(pt->next) {
                    pt->next->prev = 0;
                }
            }

            if(pt->prev) {
                free(pt->prev);
            }
            pi = pt->next;
            free(pt);
            pt = pi;
            continue;
        }
        pt = pt->next;
    }

    /* To string */
    strcpy(path, "/");
    pt = pe;
    while(pt) {
        strcat(path, pt->name);
        if(pt->next) {
            strcat(path, "/");
        }
        pt = pt->next;
    }

    return pe;
}

/*
 * Find entry for parent of entry
 */
static int ext2_find_parent_entry(int inode, struct PATH_ELEMENT* path, struct EXT2_DIRENT** direntry)
{
    struct PATH_ELEMENT* parent_path = 0;
    int result = 0;
    *direntry = 0;

    result = get_parent_path(path, &parent_path);

    if(result == 0) {
        result = ext2_find_entry(inode, parent_path, direntry);
    }

    path_free(parent_path);

    return result;
}

/*
 * Get filesystem info
 */
int fs_get_info(int* n, struct FS_INFO** info)
{
    int i = 0;
    if(n == 0 || info == 0) {
        return 1;
    }

    *n = 0;
    for(i=0; i<K_MAX_DRIVE; i++) {
        if(kdinfo[i].sectors != 0 || kdinfo[i].sides != 0) {
            (*n)++;;
        }
    }

    *info = malloc((*n) * sizeof(struct FS_INFO));

    if(*info == 0) {
        return 1;
    }

    *n = 0;
    for(i=0; i<K_MAX_DRIVE; i++) {
        if(kdinfo[i].sectors == 0 || kdinfo[i].sides == 0) {
            continue;
        }

        (*info)[*n].id = index_to_drive(i);
        strcpy((*info)[*n].name, drive_to_string((*info)[*n].id));
        (*info)[*n].fs_type = kdinfo[i].fstype;
        (*info)[*n].fs_size = kdinfo[i].fssize;
        (*info)[*n].ds_size = kdinfo[i].sectors * kdinfo[i].sides * kdinfo[i].cylinders / 2;
        (*n)++;
    }

    return 0;
}

/*
 * Set current drive
 */
int fs_set_drive(int drive)
{
    int di = drive_to_index(drive);
    sprintf("Set drive: %d ", drive);

    if(kdinfo[di].sectors == 0 || kdinfo[di].sides == 0) {
        sprintf("failed\n\r");
        return 1;
    }

    cdrc = drive;
    cdri = di;
    sprintf("ok\n\r");

    return 0;
}

/*
 * Init filesystem
 */
int fs_init(void)
{
    int result = 0;
    strcpy(fs_cwd.path, "/");
    fs_cwd.inode = EXT2_ROOT_INODE;

    for(cdri=0; cdri<K_MAX_DRIVE; cdri++) {
        if(kdinfo[cdri].sectors == 0 || kdinfo[cdri].sides == 0) {
            kdinfo[cdri].fstype = FS_TYPE_UNKNOWN;
            continue;
        }
        cdrc = index_to_drive(cdri);
        sprintf("Check filesystem in %x: ", cdrc);
        result = ext2_read_superblock(&ext2_superblock[cdri]);
        if(result) {
            kdinfo[cdri].fstype = FS_TYPE_UNKNOWN;
            kdinfo[cdri].fssize = 0;
            sprintf("unknown\n\r");
        } else {
            kdinfo[cdri].fstype = FS_TYPE_EXT2;
            kdinfo[cdri].fssize = (int)ext2_superblock[cdri].blocks;
            switch((int)ext2_superblock[cdri].block_size) {
            case 512:
                kdinfo[cdri].fssize /= 2;
                break;
            case 2048:
                kdinfo[cdri].fssize *= 2;
                break;
            }

            sprintf("ext2 %d kb of %d kb\n\r", kdinfo[cdri].fssize,
                kdinfo[cdri].sectors * kdinfo[cdri].sides * kdinfo[cdri].cylinders / 2);
        }
    }

    fs_set_drive(device);

    sprintf("Filesystem init: %d\n\r", result);
    return result;
}

/*
 * Get CWD
 */
int fs_getCWD(struct FS_PATH* cwd)
{
    if(cwd == 0) {
        return 1;
    }
    strcpy(cwd->path, fs_cwd.path);
    cwd->drive = cdrc;
    return 0;
}

/*
 * Set CWD
 */
int fs_setCWD(struct FS_PATH* cwd)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    struct PATH_ELEMENT* pe = 0;
    struct EXT2_DIRENT* ext2_dirent = 0;

    sprintf("Set CWD: %d %s\n\r", cwd->drive, cwd->path);

    result = fs_set_drive(cwd->drive);
    if(result != 0) {
        fs_set_drive(device);
        strcpy(fs_cwd.path, "/");
        fs_cwd.inode = EXT2_ROOT_INODE;
        return 1;
    }
    pe = path_parse(cwd->path, parsed);

    if(pe != 0 && result == 0) {
        result = ext2_find_entry(EXT2_ROOT_INODE, pe, &ext2_dirent);

        if(result == 0 && ext2_dirent != 0) {
            if(ext2_dirent->type == EXT2_DIRENT_TYPE_DIR) {
                /* Simplify and update path */
                pe = path_simplify_tos(fs_cwd.path, pe);
                fs_cwd.inode = ext2_dirent->inode;
            } else {
                result = 1;
            }
        }

        if(ext2_dirent) {
            free(ext2_dirent);
        }
        path_free(pe);
    } else {
        strcpy(fs_cwd.path, "/");
        fs_cwd.inode = EXT2_ROOT_INODE;
    }

    sprintf("Set CWD result = %d\n\r", result);
    return result;
}

/*
 * Find last entry offset
 */
static int ext2_find_last_entry_offset(struct EXT2_INODE* ext2_inode, char* buffer)
{
    int offset = 0;
    int offset_prev = 0;
    struct EXT2_DIRENT* ext2_dirent_aux = buffer;

    while((int)ext2_dirent_aux->inode != 0 && offset < (int)ext2_inode->low_size) {
        offset_prev = offset;
        offset += ext2_dirent_aux->size;
        ext2_dirent_aux = &buffer[offset];
    }

    return offset_prev;
}

/*
 * Calculate needed entry size given a name
 */
static int ext2_calculate_entry_size(char* name)
{
    int size = sizeof(struct EXT2_DIRENT) + strlen(name);
    size += 4 - mod(size, 4);
    return size;
}

/*
 * Get parent directory entry list, given a path
 */
static int ext2_get_parent_entry_list(struct PATH_ELEMENT* path, struct EXT2_DIRENT** ext2_dirent_parent,
        struct EXT2_INODE* ext2_inode, struct EXT2_DIRENT** ext2_dirent_buffer)
{
    int result = 0;

    *ext2_dirent_parent = 0;
    *ext2_dirent_buffer = 0;

    if(path != 0) {
        /* Find parent entry inode */
        result = ext2_find_parent_entry(EXT2_ROOT_INODE, path, ext2_dirent_parent);
        if(result == 0 && *ext2_dirent_parent != 0) {
            /* Load parent inode, containing entry */
            result = ext2_read_inode((int)(*ext2_dirent_parent)->inode, ext2_inode);
            if(result == 0) {
                *ext2_dirent_buffer = malloc((int)ext2_inode->low_size);
                if(*ext2_dirent_buffer != 0) {
                    result = ext2_read_inode_data(ext2_inode, 0, (int)ext2_inode->low_size, *ext2_dirent_buffer);
                    if(result != 0) {
                        free(*ext2_dirent_buffer);
                        *ext2_dirent_buffer = 0;
                        free(*ext2_dirent_parent);
                        *ext2_dirent_parent = 0;
                    }
                }
            }
        }
    } else {
        result = 1;
    }

    return result;
}

/*
 * Allocate first free inode
 */
static int ext2_allocate_first_free_inode(int* inode)
{
    int result = 0;

    struct EXT2_BGDESC ext2_bgdesc;
    struct EXT2_INODE ext2_inode;
    char* buff = malloc(ext2_block_size[cdri]);
    int block_group = 0;
    int table_index = 0;
    *inode = 0;

    for(;block_group<ext2_n_block_groups[cdri] && *inode==0 && result==0; block_group++) {
        result += ext2_read_bgdesc(block_group, &ext2_bgdesc);
        result += ext2_read_block((int)ext2_bgdesc.inode_usage_block, 0, ext2_block_size[cdri], buff);

        for(table_index=0; table_index<(int)ext2_superblock[cdri].inodes_per_group && result==0; table_index++) {
            if(!(buff[table_index/8] & (1 << mod(table_index, 8)))) {

                buff[table_index/8] |= (1 << mod(table_index,8));
                result += ext2_write_block((int)ext2_bgdesc.inode_usage_block, 0, ext2_block_size[cdri], buff);

                ext2_bgdesc.unallocated_inodes--;
                result += ext2_write_bgdesc(block_group, &ext2_bgdesc);

                ext2_superblock[cdri].unallocated_inodes = (int)ext2_superblock[cdri].unallocated_inodes - 1;
                ext2_write_superblock(&ext2_superblock[cdri]);

                *inode = block_group * (int)ext2_superblock[cdri].inodes_per_group + table_index + 1;
                memset(&ext2_inode, 0, sizeof(struct EXT2_INODE));
                ext2_inode.low_size = 0;
                ext2_inode.hard_links = 1;
                result += ext2_write_inode(*inode, &ext2_inode);
                result += ext2_set_inode_data_size(*inode, 2, 0);
                break;
            }
        }
    }
    free(buff);

    return result;
}


/*
 * Create entry
 */
static int ext2_create_entry(char* name, int type)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    char* buffer;
    struct PATH_ELEMENT* pe = 0;
    struct EXT2_DIRENT* ext2_dirent = 0;
    struct EXT2_INODE ext2_inode;

    sprintf("Create entry: %s, %d\n\r", name, type);

    /* Parse path */
    pe = path_parse(name, parsed);

    /* Get direntry and address */
    if(pe != 0) {
        struct PATH_ELEMENT* pe_name = pe;
        while(pe_name->next) {
            pe_name = pe_name->next;
        }

        result = ext2_get_parent_entry_list(pe, &ext2_dirent, &ext2_inode, &buffer);

        if(result == 0) {
            struct EXT2_DIRENT* ext2_dirent_aux = buffer;
            int entry_size = ext2_calculate_entry_size(pe_name->name);
            struct EXT2_DIRENT* ext2_dirent_new = malloc(entry_size);

            if(ext2_dirent_new != 0) {
                ext2_dirent_new->inode = 0;

                if(strcmp(pe_name->name, ".") == 0){
                    ext2_dirent_new->inode = ext2_dirent->inode;
                }
                else if(strcmp(pe_name->name, "..") == 0) {
                    struct PATH_ELEMENT* parent = 0;
                    result = get_parent_path(pe, &parent);
                    if(result == 0) {
                        struct EXT2_DIRENT* ext2_dirent_pr = 0;
                        struct EXT2_INODE ext2_inode_pr;
                        char* buff_pr;
                        result = ext2_get_parent_entry_list(pe, &ext2_dirent_pr, &ext2_inode_pr, &buff_pr);
                        ext2_dirent_new->inode = ext2_dirent_pr->inode;
                        free(parent);
                        free(buff_pr);
                    }
                } else {
                    int i = 0;
                    result += ext2_allocate_first_free_inode(&i);
                    ext2_dirent_new->inode = i;
                }

                ext2_dirent_new->size = entry_size;
                ext2_dirent_new->name_length = strlen(pe_name->name);
                ext2_dirent_new->type = type;
                strcpy(&(ext2_dirent_new->name_start), pe_name->name);

                /* Calculate needed inode data size */
                if(result == 0)
                {
                    int offset_insert = ext2_find_last_entry_offset(&ext2_inode, buffer);

                    if(offset_insert > 0) {
                        ext2_dirent_aux = &buffer[offset_insert];
                        ext2_dirent_aux->size = ext2_calculate_entry_size(&ext2_dirent_aux->name_start);
                        result += ext2_write_inode_data(&ext2_inode, offset_insert, ext2_dirent_aux->size, ext2_dirent_aux);
                        offset_insert = offset_insert + ext2_dirent_aux->size;
                    }
                    if((int)ext2_dirent_new->size < (int)ext2_inode.low_size - offset_insert) {
                        ext2_dirent_new->size = (int)ext2_inode.low_size - offset_insert;
                    } else {
                        result += ext2_set_inode_data_size((int)ext2_dirent->inode, offset_insert + (int)ext2_dirent_new->size, 0);
                    }
                    result += ext2_write_inode_data(&ext2_inode, offset_insert, ext2_dirent_new->size, ext2_dirent_new);
                }
                if((int)ext2_dirent_aux->type == (int)EXT2_DIRENT_TYPE_DIR && result == 0) {
                    struct EXT2_BGDESC ext2_bgdesc;
                    int block_group = ((int)ext2_dirent->inode - 1) / (int)ext2_superblock[cdri].inodes_per_group;
                    result += ext2_read_bgdesc(block_group, &ext2_bgdesc);
                    ext2_bgdesc.directories++;
                    result += ext2_write_bgdesc(block_group, &ext2_bgdesc);
                }
                free(ext2_dirent_new);
            }
        }
        free(buffer);
        buffer = 0;

        if(ext2_dirent) {
            free(ext2_dirent);
        }
        path_free(pe);
    } else {
        result = 1;
    }

    sprintf("Create entry result = %d\n\r", result);
    return result;
}

/*
 * Get entry
 */
int fs_get_entry(char* file, struct FS_DIRENTRY* entry)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    struct EXT2_DIRENT* ext2_dirent;
    struct EXT2_INODE ext2_inode;

    /* Parse path */
    struct PATH_ELEMENT* pe = path_parse(file, parsed);

    /* Get direntry and address */
    if(pe != 0) {
        result = ext2_find_entry(EXT2_ROOT_INODE, pe, &ext2_dirent);
        if(result == 0) {
            /* Entry found, fill direntry */
            strncpy(entry->name, &(ext2_dirent->name_start), ext2_dirent->name_length);
            entry->flags = 0;
            if(ext2_dirent->type == EXT2_DIRENT_TYPE_DIR) {
                entry->flags |= FS_FLAG_DIR;
            }
            if(ext2_dirent->type == EXT2_DIRENT_TYPE_FILE) {
                entry->flags |= FS_FLAG_FILE;
            }

            entry->size = 0;
            result = ext2_read_inode((int)ext2_dirent->inode, &ext2_inode);
            if( result == 0){
                entry->size = (int)ext2_inode.low_size;
            }
            free(ext2_dirent);
        }
    }
    path_free(pe);

    return result;
}

/*
 * Get dir entry
 */
int fs_get_direntry(char* dir, int n, struct FS_DIRENTRY* direntry)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    char* buffer;
    struct PATH_ELEMENT* pe = 0;
    struct EXT2_DIRENT* ext2_dirent = 0;
    struct EXT2_DIRENT* ext2_dirent_aux = 0;
    struct EXT2_INODE ext2_inode;
    struct EXT2_INODE ext2_inode_aux;
    int bytes_read = 0;
    int i = 0;

    /* Parse path */
    pe = path_parse(dir, parsed);

    /* Find inode */
    result = ext2_find_entry(EXT2_ROOT_INODE, pe, &ext2_dirent);

    if(result == 0 && ext2_dirent != 0) {
        if(ext2_dirent->type == EXT2_DIRENT_TYPE_DIR) {
            /* Load */
            result = ext2_read_inode((int)ext2_dirent->inode, &ext2_inode);
            if(result == 0) {
                buffer = malloc((int)ext2_inode.low_size);
                if(buffer != 0) {
                    result = ext2_read_inode_data(&ext2_inode, 0, (int)ext2_inode.low_size, buffer);
                    if(result == 0) {
                        ext2_dirent_aux = buffer;
                        bytes_read = 0;
                        i = 0;
                        while(bytes_read < (int)ext2_inode.low_size) {
                            if((int)ext2_dirent_aux->inode > 0) {
                                /* If direntry is 0, return num entries */
                                if(direntry == 0) {
                                    result++;
                                } else if(i == n) {
                                    /* Entry found, fill direntry */
                                    strncpy(direntry->name, &(ext2_dirent_aux->name_start), ext2_dirent_aux->name_length);
                                    direntry->flags = 0;
                                    if(ext2_dirent_aux->type == EXT2_DIRENT_TYPE_DIR) {
                                        direntry->flags |= FS_FLAG_DIR;
                                    }
                                    if(ext2_dirent_aux->type == EXT2_DIRENT_TYPE_FILE) {
                                        direntry->flags |= FS_FLAG_FILE;
                                    }

                                    direntry->size = 0;
                                    result = ext2_read_inode((int)ext2_dirent_aux->inode, &ext2_inode_aux);
                                    if( result == 0){
                                        direntry->size = (int)ext2_inode_aux.low_size;
                                    }
                                    break;
                                }
                                i++;
                            }
                            bytes_read += (int)ext2_dirent_aux->size;
                            ext2_dirent_aux = &buffer[bytes_read];
                        }
                    }
                    free(buffer);
                    buffer = 0;
                } else {
                    result = 1;
                }
            }
        } else {
            result = 1;
        }
    } else {
        result = 1;
    }

    if(ext2_dirent) {
        free(ext2_dirent);
    }

    if(pe) {
        path_free(pe);
    }

    return result;
}

/*
 * Create Dir
 */
int fs_create_dir(char* dir)
{
    char path[FS_MAX_PATH];
    int result = 0;

    sprintf("Create dir: %s\n\r", dir);
    result = ext2_create_entry(dir, EXT2_DIRENT_TYPE_DIR);

    /* Create entry for . and .. */
    strcpy(path, dir);
    strcat(path, "/.");
    result += ext2_create_entry(path, EXT2_DIRENT_TYPE_DIR);
    strcat(path, ".");
    result += ext2_create_entry(path, EXT2_DIRENT_TYPE_DIR);

    sprintf("Create dir result = %d\n\r", result);

    return result;
}

/*
 * Set file size
 */
int fs_set_file_size(char* file, int size)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    struct PATH_ELEMENT* pe = 0;
    struct PATH_ELEMENT* pe_name;
    struct EXT2_INODE ext2_inode;
    struct EXT2_DIRENT* ext2_dirent = 0;
    struct EXT2_DIRENT* ext2_dirent_aux = 0;
    char* buffer = 0;
    int bytes_read = 0;

    sprintf("Set file size: %s\n\r", file, size);

    /* Parse path */
    pe = path_parse(file, parsed);

    /* Get direntry and address */
    if(pe != 0) {
        pe_name = pe;
        while(pe_name->next) {
            pe_name = pe_name->next;
        }

        result = ext2_get_parent_entry_list(pe, &ext2_dirent, &ext2_inode, &buffer);

        if(result == 0) {
            /* Locate entry */
            ext2_dirent_aux = buffer;
            bytes_read = 0;
            while(bytes_read < (int)ext2_inode.low_size) {
                if((int)(ext2_dirent_aux->inode) > 0) {
                    if(ext2_is_entry(pe_name->name, ext2_dirent_aux)) {
                        result += ext2_set_inode_data_size((int)ext2_dirent_aux->inode, size, 0);
                        break;
                    }
                }
                bytes_read += (int)ext2_dirent_aux->size;
                ext2_dirent_aux = &(buffer[bytes_read]);
            }
        }
        free(buffer);
        buffer = 0;

        if(ext2_dirent) {
            free(ext2_dirent);
        }
        path_free(pe);
    } else {
        result = 1;
    }

    sprintf("Set file size result = %d\n\r", result);
    return result;
}

/*
 * Delete
 */
int fs_delete(char* file)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    struct PATH_ELEMENT* pe = 0;
    struct PATH_ELEMENT* pe_name;
    struct EXT2_INODE ext2_inode;
    struct EXT2_DIRENT* ext2_dirent = 0;
    struct EXT2_DIRENT* ext2_dirent_aux = 0;
    char* buffer = 0;
    int bytes_read = 0;
    int size_aux = 0;

    sprintf("Delete file: %s\n\r", file);

    pe = path_parse(file, parsed);

    if(pe != 0) {
        pe_name = pe;
        while(pe_name->next) {
            pe_name = pe_name->next;
        }

        result = ext2_get_parent_entry_list(pe, &ext2_dirent, &ext2_inode, &buffer);

        if(result == 0) {
            ext2_dirent_aux = buffer;
            bytes_read = 0;
            while(bytes_read < (int)ext2_inode.low_size) {
                if((int)(ext2_dirent_aux->inode) > 0) {
                    if(ext2_is_entry(pe_name->name, ext2_dirent_aux)) {
                        if(ext2_dirent_aux->type == EXT2_DIRENT_TYPE_DIR) {
                            struct FS_DIRENTRY direntry;
                            int n = 0;

                            /* Decrease number of dirs in group */
                            struct EXT2_BGDESC bgdesc;
                            int block_group = ((int)ext2_dirent_aux->inode - 1) / (int)ext2_superblock[cdri].inodes_per_group;
                            result += ext2_read_bgdesc(block_group, &bgdesc);
                            bgdesc.directories--;
                            result += ext2_write_bgdesc(block_group, &bgdesc);

                            /* Remove all sub-files and dirs */
                            n = fs_get_direntry(file, 0, 0);
                            while(n > 2) {
                                char dir_entry_name[FS_MAX_PATH];
                                fs_get_direntry(file, --n, &direntry);
                                strcpy(dir_entry_name, file);
                                strcat(dir_entry_name,"/");
                                strcat(dir_entry_name, direntry.name);
                                fs_delete(dir_entry_name);
                            }
                        }

                        /* Remove entry */
                        result += ext2_set_inode_data_size((int)ext2_dirent_aux->inode, 0, 1);
                        size_aux = ext2_dirent_aux->size;
                        memcpy(&(buffer[bytes_read]), &(buffer[bytes_read + (int)ext2_dirent_aux->size]),
                            (int)ext2_inode.low_size - (int)ext2_dirent_aux->size - bytes_read);

                        result += ext2_write_inode_data(&ext2_inode, 0, (int)ext2_inode.low_size, buffer);

                        ext2_inode.low_size = (int)ext2_inode.low_size - size_aux;
                        result += ext2_write_inode((int)ext2_dirent->inode, &ext2_inode);

                        break;
                    }
                }

                bytes_read += (int)ext2_dirent_aux->size;
                ext2_dirent_aux = &(buffer[bytes_read]);
            }
        }
        free(buffer);
        buffer = 0;

        if(ext2_dirent) {
            free(ext2_dirent);
        }
        path_free(pe);
    } else {
        result = 1;
    }

    sprintf("Delete file result = %d\n\r", result);
    return result;
}

/*
 * Rename entry
 */
int fs_rename(char* file, char* new_name)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    struct PATH_ELEMENT* pe = 0;
    struct PATH_ELEMENT* pe_name;
    struct EXT2_INODE ext2_inode;
    struct EXT2_DIRENT* ext2_dirent = 0;
    struct EXT2_DIRENT* ext2_dirent_aux = 0;
    char* buffer = 0;
    int bytes_read = 0;
    int i;

    sprintf("Rename file: %s -> %s\n\r", file, new_name);

    /* Parse path */
    pe = path_parse(file, parsed);

    /* Get direntry and address */
    if(pe != 0) {
        pe_name = pe;
        while(pe_name->next) {
            pe_name = pe_name->next;
        }

        /* Don't want to rename these special paths */
        if(!strcmp(pe_name->name, ".") || !strcmp(pe_name->name, "..")) {
            path_free(pe);
            return 1;
        }

        result = ext2_get_parent_entry_list(pe, &ext2_dirent, &ext2_inode, &buffer);

        if(result == 0) {
            /* Locate entry */
            ext2_dirent_aux = buffer;
            bytes_read = 0;
            while(bytes_read < (int)ext2_inode.low_size) {
                if((int)(ext2_dirent_aux->inode) > 0) {
                    if(ext2_is_entry(pe_name->name, ext2_dirent_aux)) {
                        int entry_size = ext2_calculate_entry_size(new_name);
                        int mem_disp = entry_size - ext2_dirent_aux->size;
                        /* Check size for new name */
                        if(entry_size > ext2_dirent_aux->size) {
                            int last_entry_offset = ext2_find_last_entry_offset(&ext2_inode, buffer);
                            if(last_entry_offset == bytes_read) {
                                /* This is the last entry */
                                /* Resize it */
                                char* buffer_aux = 0;
                                ext2_set_inode_data_size((int)ext2_dirent->inode, (int)ext2_inode.low_size + mem_disp, 0);
                                ext2_inode.low_size = (int)ext2_inode.low_size + mem_disp;
                                buffer_aux = malloc((int)ext2_inode.low_size);
                                memcpy(buffer_aux, buffer, (int)ext2_inode.low_size - mem_disp);
                                free(buffer);
                                buffer = buffer_aux;
                            } else {
                                /* This is not the last entry */
                                /* Move all entrys to make room */
                                struct EXT2_DIRENT* ext2_dirent_last = &(buffer[last_entry_offset]);
                                int last_entry_required_size = ext2_calculate_entry_size(&ext2_dirent_last->name_start);
                                if(ext2_dirent_last->size - last_entry_required_size <= mem_disp) {
                                    /* Resize */
                                    char* buffer_aux = 0;
                                    ext2_set_inode_data_size((int)ext2_dirent->inode, (int)ext2_inode.low_size + mem_disp, 0);
                                    ext2_inode.low_size = (int)ext2_inode.low_size + mem_disp;
                                    buffer_aux = malloc((int)ext2_inode.low_size);
                                    memcpy(buffer_aux, buffer, (int)ext2_inode.low_size - mem_disp);
                                    free(buffer);
                                    buffer = buffer_aux;
                                } else {
                                    /* Just update size values to make room */
                                    ext2_dirent_last->size -= mem_disp;
                                }
                                /* Move memory */
                                for(i=(int)ext2_inode.low_size-1; i>=bytes_read+(int)ext2_dirent_aux->size; i--){
                                    buffer[i] = buffer[i-mem_disp];
                                }
                            }
                            /* Update size values */
                            ext2_dirent_aux->size = entry_size;
                            ext2_dirent_aux->name_length = strlen(new_name);
                        }
                        /* Modify name and write */
                        strcpy(&(ext2_dirent_aux->name_start), new_name);
                        result = ext2_write_inode_data(&ext2_inode, 0, (int)ext2_inode.low_size, buffer);
                        break;
                    }
                }

                bytes_read += (int)ext2_dirent_aux->size;
                ext2_dirent_aux = &(buffer[bytes_read]);
            }
        }
        if(buffer) {
            free(buffer);
        }

        if(ext2_dirent) {
            free(ext2_dirent);
        }
        path_free(pe);
    } else {
        result = 1;
    }

    sprintf("Rename file result = %d\n\r", result);
    return result;
}

/*
 * Read file
 */
int fs_read_file(char* file, int offset, int buff_size, char* buff)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    struct PATH_ELEMENT* pe = 0;
    struct EXT2_DIRENT* ext2_dirent = 0;
    struct EXT2_INODE ext2_inode;

    sprintf("Read file: %s, offset: %d, size: %d, buff: %x\n\r", file, offset, buff_size, (int)buff);

    pe = path_parse(file, parsed);

    if(pe != 0) {
        result = ext2_find_entry(EXT2_ROOT_INODE, pe, &ext2_dirent);

        if(result == 0 && ext2_dirent != 0) {
            sprintf("Read inode: %d\n\r", (int)ext2_dirent->inode);
            if(ext2_dirent->type == EXT2_DIRENT_TYPE_FILE) {
                result = ext2_read_inode((int)ext2_dirent->inode, &ext2_inode);
                if(result == 0) {
                    if(buff != 0) {
                        result = ext2_read_inode_data(&ext2_inode, offset, buff_size, buff);
                    } else {
                        result = 1;
                    }
                }
            } else {
                result = 1;
            }
        } else {
            result = 1;
        }

        if(ext2_dirent) {
            free(ext2_dirent);
        }
        path_free(pe);
    } else {
        result = 1;
    }

    sprintf("Read file result = %d\n\r", result);
    return result;
}

/*
 * Write file
 */
int fs_write_file(char* file, int offset, int buff_size, char* buff)
{
    int result = 0;
    char parsed[FS_MAX_PATH];
    struct PATH_ELEMENT* pe = 0;
    struct EXT2_DIRENT* ext2_dirent = 0;
    struct EXT2_INODE ext2_inode;

    sprintf("Write file: %s, offset: %d, size: %d, buff: %x\n\r", file, offset, buff_size, (int)buff);

    pe = path_parse(file, parsed);

    if(pe != 0) {
        result = ext2_find_entry(EXT2_ROOT_INODE, pe, &ext2_dirent);

        /* Create if it does not exist */
        if(result != 0) {
            result = ext2_create_entry(file, EXT2_DIRENT_TYPE_FILE);
            result += ext2_find_entry(EXT2_ROOT_INODE, pe, &ext2_dirent);
        }

        if(result == 0 && ext2_dirent != 0) {
            if(ext2_dirent->type == EXT2_DIRENT_TYPE_FILE) {
                /* Load */
                result = ext2_read_inode((int)ext2_dirent->inode, &ext2_inode);
                if(result == 0) {
                    if(buff != 0) {
                        if(offset + buff_size > (int)ext2_inode.low_size) {
                            result = ext2_set_inode_data_size((int)ext2_dirent->inode, offset + buff_size, 0);
                            result += ext2_read_inode((int)ext2_dirent->inode, &ext2_inode);
                        }
                        result += ext2_write_inode_data(&ext2_inode, offset, buff_size, buff);
                    } else {
                        result = 1;
                    }
                }
            } else {
                result = 1;
            }
        } else {
            result = 1;
        }

        if(ext2_dirent) {
            free(ext2_dirent);
        }
        path_free(pe);
    } else {
        result = 1;
    }

    sprintf("Write file result = %d\n\r", result);
    return result;
}

/*
 * Sys drive
 */
int fs_sys_drive(int dst, int src)
{
    int i = 0;
    int result = 0;
    char buff[SECTOR_SIZE];

    for(i=0; i<2880 && result == 0; i++) {
        result += read_disk(src, i, 1, buff);
        result += write_disk(dst, i, 1, buff);
    }
    return result;
}
