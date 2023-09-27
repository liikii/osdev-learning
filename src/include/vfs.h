#ifndef VFS_H
#define VFS_H
#include <generic_tree.h>
#include <list.h>
// Linux journaling是一种文件系统的技术，用于提供更强大的数据完整性和恢复能力。它主要用于防止文件系统崩溃或意外断电时数据的损坏。

// Journaling是通过在文件系统的日志（journal）中记录所有对文件系统所做的更改来实现的。这个日志记录了操作系统对文件系统进行的写入操作，包括创建、修改和删除文件以及更新目录结构等。每当执行一个写入操作时，相应的变化会首先被写入到日志中，然后再被应用到实际的文件系统结构中。

// 如果发生系统崩溃或其他故障，文件系统可以使用日志来恢复到最近一次一致状态。通过读取日志并重新应用之前未完成的操作，文件系统可以确保数据的一致性和完整性。

// 常见的Linux文件系统，如ext4、XFS和JFS，都支持journaling技术。这些文件系统利用日志记录机制来提供更可靠的数据保护，并减少了数据丢失的风险。无论在服务器环境还是个人计算机上，使用journaling文件系统都能提供更高的数据安全性和稳定性。
// https://read.seas.harvard.edu/cs161/2022/schedule/
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STRING "/"
#define PATH_UP  ".."
#define PATH_DOT "."
#define VFS_EXT2_MAGIC 0xeeee2222

#define O_RDONLY     0x0000
#define O_WRONLY     0x0001
#define O_RDWR       0x0002
#define O_APPEND     0x0008
#define O_CREAT      0x0200
#define O_TRUNC      0x0400
#define O_EXCL       0x0800
#define O_NOFOLLOW   0x1000
#define O_PATH       0x2000

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x04
#define FS_BLOCKDEVICE 0x08
#define FS_PIPE        0x10
#define FS_SYMLINK     0x20
#define FS_MOUNTPOINT  0x40

#define     _IFMT   0170000 /* type of file */
#define     _IFDIR  0040000 /* directory */
#define     _IFCHR  0020000 /* character special */
#define     _IFBLK  0060000 /* block special */
#define     _IFREG  0100000 /* regular */
#define     _IFLNK  0120000 /* symbolic link */
#define     _IFSOCK 0140000 /* socket */
#define     _IFIFO  0010000 /* fifo */

struct vfs_node;

typedef uint32_t (*get_file_size_callback)(struct vfs_node * node);
typedef uint32_t (*read_callback) (struct vfs_node *, uint32_t, uint32_t, char *);
typedef uint32_t (*write_callback) (struct vfs_node *, uint32_t, uint32_t, char *);
typedef void (*open_callback) (struct vfs_node*, uint32_t flags);
typedef void (*close_callback) (struct vfs_node *);
typedef struct dirent *(*readdir_callback) (struct vfs_node *, uint32_t);
typedef struct vfs_node *(*finddir_callback) (struct vfs_node *, char *name);
typedef void (*create_callback) (struct vfs_node *, char *name, uint16_t permission);
typedef void (*unlink_callback) (struct vfs_node *, char *name);
typedef void (*mkdir_callback) (struct vfs_node *, char *name, uint16_t permission);
typedef int (*ioctl_callback) (struct vfs_node *, int request, void * argp);
typedef int (*get_size_callback) (struct vfs_node *);
typedef void (*chmod_callback) (struct vfs_node *, uint32_t mode);
typedef char ** (*listdir_callback) (struct vfs_node *);

typedef struct vfs_node {
    // Baisc information about a file(note: in linux, everything is file, so the vfs_node could be used to describe a file, directory or even a device!)
    char name[256];
    void * device;
    uint32_t mask;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t inode_num;
    uint32_t size;
    uint32_t fs_type;
    uint32_t open_flags;
    // Time
    uint32_t create_time;
    uint32_t access_time;
    uint32_t modified_time;

    uint32_t offset;
    unsigned nlink;
    int refcount;

    // File operations
    read_callback read;
    write_callback write;
    open_callback open;
    close_callback close;
    readdir_callback readdir;
    finddir_callback finddir;
    create_callback create;
    unlink_callback unlink;
    mkdir_callback mkdir;
    ioctl_callback ioctl;
    get_size_callback get_size;
    chmod_callback chmod;
    get_file_size_callback get_file_size;

    listdir_callback listdir;
}vfs_node_t;

struct dirent {
    char name[256];
    uint32_t inode_num;
};

typedef struct vfs_entry {
    char * name;
    vfs_node_t * file;
}vfs_entry_t;


uint32_t vfs_get_file_size(vfs_node_t * node);

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, char *buffer);

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, char *buffer);

void vfs_open(struct vfs_node *node, uint32_t flags);

void vfs_close(vfs_node_t *node);

vfs_node_t *vfs_finddir(vfs_node_t *node, char *name);

void vfs_mkdir(char *name, uint16_t permission);

void vfs_mkfile(char *name, uint16_t permission);

int vfs_create_file(char *name, uint16_t permission);

vfs_node_t *file_open(const char *file_name, uint32_t flags);

char *expand_path(char *input);

int vfs_ioctl(vfs_node_t *node, int request, void * argp);

void vfs_chmod(vfs_node_t *node, uint32_t mode);

void vfs_unlink(char * name);

int vfs_symlink(char * value, char * name);

int vfs_readlink(vfs_node_t * node, char * buf, uint32_t size);

void vfs_init();

void vfs_mount(char * path, vfs_node_t * local_root);

typedef vfs_node_t * (*vfs_mount_callback)(char * arg, char * mountpoint);

void vfs_register(char * name, vfs_mount_callback callme);

void vfs_mount_dev(char * mountpoint, vfs_node_t * node);

void print_vfstree();

void vfs_db_listdir(char * name);

#endif
