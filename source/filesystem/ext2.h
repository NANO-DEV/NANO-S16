/*
 * EXT2 filesystem
 */

#ifndef _EXT2_H
#define _EXT2_H

/*
 *  Superblock structure
 */
struct EXT2_SUPERBLOCK {
    int32_t     inodes;
    int32_t     blocks;
    int32_t     superuser_blocks;
    int32_t     unallocated_blocks;
    int32_t     unallocated_inodes;
    int32_t     superblock_block;
    int32_t     block_size;
    int32_t     fragment_size;
    int32_t     blocks_per_group;
    int32_t     fragments_per_group;
    int32_t     inodes_per_group;
    int32_t     mount_time;
    int32_t     written_time;
    int16_t     mounts_since_check;
    int16_t     mounts_until_check;
    int16_t     signature;
    int16_t     state;
    int16_t     error;
    int16_t     minor_version;
    int32_t     check_time;
    int32_t     check_interval;
    int32_t     os_id;
    int32_t     major_version;
    int16_t     reserved_uid;
    int16_t     reserved_gid;
/* extended */
    int32_t     first_nonreserved_inode;
    int16_t     inode_size;
    int16_t     superblock_group;
    int32_t     optional_features;
    int32_t     required_features;
    int32_t     readonly_features;
    int8_t      file_system_id[16];
    int8_t      volume_name[16];
    int8_t      mount_path[64];
    int32_t     compression;
    int8_t      file_preallocate_blocks;
    int8_t      dir_preallocate_blocks;
    int16_t     unused;
    int8_t      journal_id[16];
    int32_t     journal_inode;
    int32_t     journal_device;
    int32_t     orphan_inode_head;
};


/*
 *  Block group descriptor structure
 */
struct EXT2_BGDESC {
    int32_t     usage_bitmap_block;
    int32_t     inode_usage_block;
    int32_t     inode_table_block;
    int16_t     unallocated_blocks;
    int16_t     unallocated_inodes;
    int16_t     directories;
};

/*
 *  Inode structure
 */
struct EXT2_INODE {
    int16_t     type_perm;
    int16_t     uid;
    int32_t     low_size;
    int32_t     accessed_time;
    int32_t     creation_time;
    int32_t     modified_time;
    int32_t     deletion_time;
    int16_t     gid;
    int16_t     hard_links;
    int32_t     sectors;
    int32_t     flags;
    int32_t     os;
    int32_t     direct_block[12];
    int32_t     single_block;
    int32_t     double_block;
    int32_t     triple_block;
    int32_t     generation;
    int32_t     extended_attribute;
    int32_t     upper_size_dir_acl;
    int32_t     fragment;
    int8_t      os2[12];
};

/*
 *  Directory entry structure
 */
struct EXT2_DIRENT {
    int32_t     inode;
    int16_t     size;
    int8_t      name_length;
    int8_t      type;
    char_t      name_start;
};

#define EXT2_DIRENT_TYPE_UNKNOWN 0
#define EXT2_DIRENT_TYPE_FILE    1
#define EXT2_DIRENT_TYPE_DIR     2
#define EXT2_DIRENT_TYPE_CHAR    3
#define EXT2_DIRENT_TYPE_BLOCK   4
#define EXT2_DIRENT_TYPE_FIFO    5
#define EXT2_DIRENT_TYPE_SOCKET  6
#define EXT2_DIRENT_TYPE_LINK    7


#endif   /* _EXT2_H */
