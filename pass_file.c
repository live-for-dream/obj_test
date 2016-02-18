#include "pass_file.h"
#include <linux/limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

#define max_other_len 2048
#define max_cipher_len 2048
#define passwd          "testabcd"

#define dir_mode        0755
#define file_mode       0755

void decryption(record_t *record, char *passwd) {
    printf("record passwd:\n%s\n", record->passwd_ciper.str);
}
/*
struct obj_attr_s {
    int     type;
    obj_op  create;
    obj_op  show_self;
    obj_op  show_childs;
    obj_op  del;
    obj_op  build;
};

*/

class_t  root;
list_head_t dirty_queue;
static obj_attr_t cla_attr = {
    .type = obj_type_cla,
    .create = NULL,
    .show_self = class_show_self,
    .show_childs = class_show_child,
    .build = class_build
};

static obj_attr_t user_attr = {
    .type = obj_type_usr,
    .
    .build = user_build
};

static obj_attr_t record_attr = {
    .type = obj_type_rcd,
    .build = NULL
};


#define set_user_dirty(usr)\
    (usr)->dirty = true;\
    list_add(&(usr)->dirty_queue, &dirty_queue)
#define clean_user_dirty(usr)\
        (usr)->dirty = false;\
        list_del(&(usr)->dirty_queue)

static int show_childs(object_t *obj);
static void get_class_path(object_t *obj, string_t *path) ;
static void get_user_path(object_t *obj, string_t *path);
static int create_class(object_t *parent, create_args_t *arg);
static int create_user(object_t *parent, create_args_t *arg);
static int write_file(int fd, char *buf, ssize_t size, off_t off);
static int read_file(int fd, char *buf, ssize_t size,off_t off);



int init_root(string_t *path) {
    char                ab_path[PATH_MAX + 1];
    if (!path) {
        getcwd(ab_path, sizeof(ab_path));
        if (errno) {
            return ERR;
        }
    }

    
}

int start_record_tree(string_t *path) {
    char                ab_path[PATH_MAX + 1];
    int                 p_len;
    DIR                *dir;
    struct dirent      *p_dirent;

    if (!path || !path->len) {
        getcwd(ab_path, sizeof(ab_path));
        if (errno) {
            return ERR;
        }
    } else {
        sprintf(ab_path, "%s", (char *)path->str);
        ab_path[path->len] = '\0';
    }

    p_len = strlen(ab_path);
    init_class(&root);
    root.path.len = p_len;
    root.path.str = malloc(sizeof(uchar) *p_len + 1);
    if (!root.path.str) {
        return ERR_NOMEM;
    }
    sprintf((char *)root.path.str, "%s", ab_path);
    root.path.str[p_len] = '\0';
    
    dir = opendir(ab_path);
    if (!dir) {
        return ERR;
    }

    root.dir = dir;
    while((p_dirent=readdir(dir))) {
        if (p_dirent->d_type & DT_REG) {
            
        } else if (p_dirent->d_type & DT_DIR) {
            
        }
        printf("%s\n",p_dirent->d_name);
    }
    
}

static void get_class_path(object_t *obj, string_t *path) {
    class_t *cla; 
    cla = obj_entry(obj, class_t, obj);
    if (!obj->parent) {
        sprintf((char *)path->str, "%s", (char *)cla->path.str);
        path->len += cla->path.len;
        return;
    }

    get_class_path(obj->parent, path);
    sprintf((char *)path->str + path->len, "/%s", (char *)cla->path.str);
    path->len += 1 + cla->path.len;
    return;
}

static void get_user_path(object_t *obj, string_t *path) {
    user_t *usr;
    usr = obj_entry(obj, user_t, obj);
    
    get_class_path(obj->parent , path);
    sprintf((char *)path->str + path->len, "/%s", (char *)usr->file_name.str);
    path->len += 1 + usr->file_name.len;
}

int class_build(object_t *obj) {
    class_t            *cla;
    class_t            *new_cla;
    user_t             *new_user;
    string_t            path;
    char                ab_path[PATH_MAX + 1];
    DIR                *dir;
    struct dirent      *p_dirent;
    string_t            hex_name;
    
    
    path.str = (uchar)ab_path;
    path.len = 0;
    cla = obj_entry(obj, class_t, obj);
    get_class_path(obj, &path);

    dir = opendir(ab_path);
    if (!dir) {
        return ERR;
    }

    cla->dir = dir;
    while((p_dirent=readdir(dir))) {
        if (p_dirent->d_type & DT_DIR) {
            if (!strcmp(p_dirent->d_name, ".") || !strcmp(p_dirent->d_name, ".")) {
                continue;
            }

            new_cla = malloc(sizeof(class_t));
            if (!new_cla) {
                return ERR_NOMEM;
            }

            new_cla->path.str = malloc(strlen(p_dirent->d_name) * sizeof(uchar) + 1);
            if (!new_cla->path.str) {
                return ERR_NOMEM;
            }

            sprintf(new_cla->path.str, "%s", p_dirent->d_name);
            init_obj(&new_cla->obj);
            add_obj(obj, &new_cla->obj);
            new_cla->obj.options = &cla_attr;
            new_cla->obj.options->build(&new_cla->obj);
        } else if (p_dirent->d_type & DT_REG) {
            new_user = malloc(sizeof(user_t));
            if (!new_user) {
                return ERR_NOMEM;
            }
            
            hex_name.len = strlen(p_dirent->d_name);
            hex_name.str = (uchar *)p_dirent->d_name;
            hex_to_string(&new_user->user_name, &hex_name);
            new_user->file_name.str = malloc(hex_name.len + 1);
            new_user->file_name.len = hex_name.len;
            sprintf(new_user->file_name.str, "%s", p_dirent->d_name);
            
            init_obj(&new_user->obj);
            add_obj(obj, &new_user->obj);
            new_user->obj.options = &user_attr;
            //new_user->obj.options->build(&new_user->obj);
        } else {
            continue;
        }
    }

    return OK;
}

int class_show_self(object_t *obj) {
    class_t         *cla;
    cla = obj_entry(obj, class_t, obj);
    printf("class name:\n%s\n", cla->path.str);
    return OK;
}

static int show_childs(object_t *obj) {
    object_t *next_obj;
    int ret
    list_for_each_entry(next_obj, &obj->childs, childs) {
        ret = next_obj->options->show_self(next_obj);
        if (ret != OK) {
            return ERR;
        }
    }

    return OK;
}

static int create_class(object_t *parent, create_args_t *arg) {
    string_t         path;
    char             ab_path[PATH_MAX + 1];
    class_t         *cla;
    DIR             *dir;
    int              ret;
    

    path.str = (uchar)ab_path;
    path.len = 0;
    get_class_path(parent, &path);

    cla = malloc(sizeof(class_t));
    if (!cla) {
        goto FAIL;
    }
    init_class(cla);
    cla->obj.options = &cla_attr;
    
    cla->path.str = malloc((arg->name.len + 1) * sizeof(char));
    if (!cla->path.str) {
        goto CLA_FAIL;
    }
    cla->path.len = arg->name.len;
    sprintf(cla->path.str, "%s", arg->name.str);
    sprintf(ab_path + path.len, "/%s", arg->name.str);
    path.len += 1 + arg->name.len;
    
    dir = opendir(ab_path);
    if (dir) {
        printf("dir %s exsists\n", ab_path);
        goto PATH_FAIL;
    }

    ret = mkdir(ab_path, dir_mode);
    if (ret) {
        goto PATH_FAIL;
    }

    cla->dir = dir;
    add_obj(parent, &cla->obj);
OUT:
    return OK;

PATH_FAIL:
    free(cla->path.str);
CLA_FAIL:
    free(cla);
FAIL:
    return ERR;
    
}

static int create_user(object_t *parent, create_args_t *arg) {
    string_t         path;
    char             ab_path[PATH_MAX + 1];
    char             len_buf[9];
    user_t          *usr;
    string_t         file_name;
    int              ret;
    int              fd;

    path.str = (uchar)ab_path;
    path.len = 0;
    get_class_path(parent, &path);

    usr = malloc(sizeof(user_t));
    if (!usr) {
        goto FAIL;
    }
    init_user(usr);
    usr->obj.options = &user_attr;

    ret = string_to_hex(&arg->name, &file_name);
    if (ret != OK) {
        goto USR_FAIL;
    }

    sprintf(ab_path + path.len, "/%s", file_name.str);
    path.len += 1 + file_name.len;

    ret =  access(ab_path, F_OK);
    if (!ret || errno != ENOENT) {
        goto HEX_FAIL;
    }

    fd = open(ab_path, O_CREAT | O_RDWR, file_mode);
    if (fd < 0) {
        goto HEX_FAIL;
    }

    sprintf(len_buf, "%08d", 0);
    len_buf[8] = '\0';
    ret = write_file(fd, len_buf, 8, 0);
    if (ret != 8) {
        goto FILE_FAIl;
    }

    usr->file_name.str = file_name.str
    usr->file_name.len = file_name.len;
    usr->record_num = 0;
    usr->user_name.str = malloc(sizeof(uchar) * (arg->name.len + 1));
    sprintf(usr->user_name.str, "%s", arg->name.str);
    usr->user_name.len = arg->name.len;

    add_obj(parent, &usr->obj);
    close(fd);
    
    return OK;

FILE_FAIl:
    close(fd);
HEX_FAIL:
    free(file_name.str);
USR_FAIL:
    free(usr);
FAIL:
    return ERR;
}


int class_create(object_t *parent, void *data) {
    create_args_t   *arg;
    if (!data) {
        return ERR;
    }

    arg = (create_args_t *)data;
    if (arg->type == obj_type_cla) {
        return create_class(parent, arg);
    } else if (arg->type == obj_type_usr) {
        return create_user(parent, arg);
    } else {
        return ERR;
    }
}
int class_show_child(object_t * obj) {
    return show_childs(obj);
}
static int read_file(int fd, char *buf, ssize_t size,off_t off) {
    int ret;
    ssize_t read_n, n_read;
    ret = lseek(fd, off, SEEK_SET);
    if (ret) {
        return ERR;
    }

    read_n = 0;
    
    while(read_n < size) {
        n_read = read(fd, buf + read_n, size - read_n);
        if (n_read < 0) {
            return ERR;
        }

        read_n += n_read;
    }
    return read_n;
}

static int write_file(int fd, char *buf, ssize_t size, off_t off) {
    int         ret;
    ssize_t     write_n, n_write;

    ret = lseek(fd, off, SEEK_SET);
    if (ret) {
        return ERR;
    }

    write_n = 0;
    while(write_n < size) {
        n_write = write(fd, buf + write_n, size - write_n);
        if (n_write < 0) {
            return ERR;
        }

        write_n += n_write;
    }

    return write_n;
}

int record_read(int fd, record_t **record_r, off_t off) {
    char         len_buf[9];
    char        *buf;
    ssize_t      size;
    int          ret;
    int          read_n = 0;
    record_t    *record;
    
    record = malloc(sizeof(record_t));
    if (!record) {
        goto FAIL;
    }
    init_record(record);
    record->obj.options = &record_attr;
 
    ret = read_file(fd, len_buf, 8, off);
    if (ret != 8) {
        goto FAIL;
    }
    read_n += 8;
    len_buf[8] = '\0';
    ret = atoi(len_buf);
    if (ret < 0 || ret > max_cipher_len) {
        return ERR;
    }
    record->passwd_ciper.len = (uint)ret;
    buf = malloc(sizeof(char) * (ret + 1));
    if (!buf) {
        goto FAIL;
    }

    off += 8;
    size = (ssize_t)ret;
    ret = read_file(fd, buf, size, off)
    if (ret != size) {
        goto CIPHER_FAIL;
    }
    read_n += ret;
    buf[size] = '\0';
    record->passwd_ciper.str = (uchar *)buf;

    off += (off_t)size;
    ret = read_file(fd, len_buf, 8, off);
    if (ret != 8) {
        goto CIPHER_FAIL;
    }

    read_n += 8;
    len_buf[8] = '\0';
    ret = atoi(len_buf);
    if (ret < 0 || ret > max_other_len) {
        goto CIPHER_FAIL;
    }

    record->other.len = (uint)ret;
    if (ret == 0) {
        goto OUT;
    }

    buf = malloc(sizeof(char) *(ret + 1));
    if (!buf) {
        goto CIPHER_FAIL;
    }

    off += 8;
    size = (ssize_t)ret;
    ret = read_file(fd, buf, size, off)
    if (ret != size) {
        goto OTHER_FAIL;
    }
    read_n += ret;
    buf[size] = '\0';
    record->other.str = (uchar *)buf;

OUT:
    *record_r = record;
    return read_n;

OTHER_FAIL:
    free(record->other.str);
CIPHER_FAIL:
    free(record->passwd_ciper.str);
FAIL:    
    free(record);
    return ERR;
}

int user_build(object_t *obj) {
    user_t             *usr;
    char                ab_path[PATH_MAX + 1];
    string_t            path;
    int                 fd;
    char                lenbuf[9];
    int                 len;
    off_t               off = 0;
    int                 read_n, i;
    record_t            *record;

    path.len = 0;
    path.str = ab_path;
    get_class_path(obj->parent, &path);
    usr = obj_entry(obj, user_t, obj);

    sprintf(path.str + path.len, "/%s", usr->file_name.str);
    path.len += usr->file_name.len + 1;

    fd = open(path.str, O_RDWR);
    if (fd < 0) {
        return ERR;
    }

    read_n = read_file(fd, lenbuf, 8, 0);
    if (read_n != 8) {
        return ERR;
    }
    off += 8;
    lenbuf[8] = '\0';
    usr->record_num = atoi(lenbuf);

    for (i = 0; i < usr->record_num; i++) {
        read_n = record_read(fd, &record, off);
        if (read_n < 0) {
            return ERR;
        }

        add_obj(obj, &record->obj);
    }

    close(fd);
    return OK;
    
}

int user_show_self(object_t *obj) {
    user_t             *usr;

    usr = obj_entry(obj, user_t, obj);
    printf("user name:\n")
}

int user_show_childs(object_t * obj) {
    return show_childs(obj);
}

int user_create(object_t *parent, void *data) {
    create_args_t           *arg;
    char                     ab_path[PATH_MAX + 1];
    int                      fd;
    string_t                 path;
    record_t                *record;
    user_t                  *usr;
    
    arg = (create_args_t *)data;
    if (arg->type != obj_type_rcd) {
        goto FAIL;
    }

/*
    path.len = 0;
    path.str = ab_path;
    get_user_path(obj, &path);

    fd = open(ab_path, O_RDWR);
*/
    record = malloc(sizeof(record_t));
    if (!record) {
        goto FAIL;
    }

    init_record(record);
    if (arg->other.len) {
        record->other.str = malloc(sizeof(uchar) * (arg->other.len + 1));
        if (!record->other.str) {
            goto REC_FAIL;
        }
        record->other.len = arg->other.len;
    }

    //should encryption, now we opmitted it
    if (arg->plain.len) {
        record->passwd_ciper.str = malloc(sizeof(uchar) * (arg->other.len + 1));
        if (!record->passwd_ciper.str) {
            goto OTH_FAIL;
        }
        sprintf(record->passwd_ciper.str, "%s", arg->plain.str);
        record->passwd_ciper.len = arg->plain.len;
    }

    init_obj(&record->obj);
    record->obj.options = &record_attr;
    add_obj(parent, &record->obj);

    usr = obj_entry(parent, user_t, obj);
    if (!usr->dirty) {
        set_user_dirty(usr);
    }

    return OK;
OTH_FAIL:
    if (record->other.len) {
        free(record->other.str);
    }
REC_FAIL:
    free(record);
FAIL:
    return ERR;
}

int record_show_self(object_t *obj) {
    record_t            *record;

    record = obj_entry(obj, record_t, obj);
    printf("record other info:\n%s\n", record->other.str);

    decryption(record, passwd);
    return OK;
}

