#include "pass_file.h"
#include <linux/limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#define max_other_len 2048
#define max_cipher_len 2048
#define passwd          "testabcd"

#define dir_mode        0755
#define file_mode       0755

/*
void decryption(record_t * record, char * passwd);
void decryption(record_t *record, char *passwd) {
    printf("record passwd:\n%s\n", record->passwd_ciper.str);
}
*/
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
    .create = class_create,
    .show_self = class_show_self,
    .show_childs = class_show_child,
    .delete = class_del,
    .write = class_write,
    .lookup = class_lookup,
    .build = class_build
};

static obj_attr_t user_attr = {
    .type = obj_type_usr,
    .create = user_create,
    .show_self = user_show_self,
    .show_childs = user_show_childs,
    .delete = user_del,
    .write = user_write,
    .lookup = user_lookup,
    .build = user_build
};

static obj_attr_t record_attr = {
    .type = obj_type_rcd,
	.delete = record_del,
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
    char               *str;
    uchar              *path_str;
    DIR                *dir;
    int                 len;
	int					ret;
	class_t			   *root_p = &root;

    init_class(root_p);
 
    if (!path) {
        getcwd(ab_path, sizeof(ab_path));
        if (errno) {
            return ERR;
        }

        len = strlen(ab_path);
        sprintf(ab_path + len, "/root");
        len += strlen("/root");  
    } else {
        
        sprintf(ab_path, "%s", path->str);
        len = path->len;
    }

    path_str = (uchar *)malloc(sizeof(char) * (len + 1));
    if (!path_str) {
        return ERR;
    }
        
    sprintf(path_str, "%s", ab_path); 

    root.path.str = root.name.str = path_str;
    root.path.len = root.name.len = len;

    dir = opendir(ab_path);
    if (!dir) {
        if (errno == ENOENT) {
            ret = mkdir(ab_path, dir_mode);
            if (ret) {
                return ERR;
            }

            dir = opendir(ab_path);
            if (!dir) {
                return ERR;
            }
        }
    }

	close(dir);
    root.dir = NULL;
    root.obj.options = &cla_attr;    
}

object_t *get_root_obj() {
    return &root.obj;
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
    
    
    path.str = (uchar *)ab_path;
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
            if (!strcmp(p_dirent->d_name, ".") || !strcmp(p_dirent->d_name, "..")) {
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
			new_cla->path.len = strlen(p_dirent->d_name);
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
    int       ret;
    list_for_each_entry(next_obj, &obj->childs, sibling) {
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
    

    path.str = (uchar *)ab_path;
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

    path.str = (uchar *)ab_path;
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

    usr->file_name.str = file_name.str;
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

int class_write(object_t *obj) {
    int             ret;
    user_t         *usr;
    if (list_empty(&dirty_queue)) {
        return 0;
    }
    
    (void *)obj;
    list_for_each_entry(usr, &dirty_queue, dirty_queue) {
        ret = usr->obj.options->write(&usr->obj);
        if (ret < 0) {
            printf("user write err\n");
        }
    }

    return ret;
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

int class_del(object_t *obj) {
    class_t         *cla;
    object_t        *child_obj;
    string_t         path;
    char             ab_path[PATH_MAX + 1];
	int				 ret;

    list_for_each_entry(child_obj, &obj->childs, sibling) {
        if (child_obj->options->type == obj_type_cla) {
            ret = class_del(child_obj);
            if (ret != OK) {
                return ERR;
            }
        } else if (child_obj->options->type == obj_type_usr) {
            ret = user_del(child_obj);
            if (ret != OK) {
                return ERR;
            }
        } else {
            return ERR;
        }
    }

    cla = obj_entry(obj, class_t, obj);
    path.len = 0;
    path.str = (uchar *)ab_path;
    get_class_path(obj, &path);

    list_del(&obj->sibling);
    ret = closedir(cla->dir);
    if (!ret) {
        printf("close_dir fail\n");
    }

    ret = rmdir(ab_path);
    if (!ret) {
        return ERR;
    }

    if (cla->path.len) {
        free(cla->path.str);
    }

    if (cla->name.len) {
        free(cla->name.str);
    }

    return OK;
}

object_t *class_lookup(object_t *obj, string_t *name) {
    class_t         *des_cla;
	user_t			*des_usr;
	object_t        *des_obj;
	
    list_for_each_entry(des_obj, &obj->childs, sibling) {
		if (des_obj->options->type == obj_type_cla) { 
        	des_cla = obj_entry(des_obj, class_t, obj);
        	if (!strcmp(name->str, des_cla->path.str)) {
            	return des_obj;
        	}
		} else if (des_obj->options->type == obj_type_usr) {
			des_usr = obj_entry(des_obj, user_t, obj);
			if (!strcmp(name->str, des_usr->user_name.str)) {
				return des_obj;
			}
		} else {
			return NULL;
		}
		
    }

    return NULL;
}

/*
int class_move(object_t *obj, object_t *parent) {
    int             ret;
    class_t        *cla;
    class_t        *parent_cla;
    object_t       *old_parent;
    char            ab_path[PATH_MAX + 1];
    string_t        old_path;
    string_t        new_path;

    old_parent = obj->parent;
    if (!old_parent) {
        goto FAIL;
    }
    cla = obj_entry(obj, class_t, obj);
    parent_cla = obj_entry(parent, class_t, obj);
    if () {
        
    }

FAIL:
    return ERR;
}
*/

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
    ret = read_file(fd, buf, size, off);
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
    ret = read_file(fd, buf, size, off);
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


static int record_write(int fd, record_t *record, off_t off) {
    char         len_buf[9];
    char        *str;
    ssize_t      size;
    int          ret;
    int          write_n = 0;

    sprintf(len_buf, "%08d", record->passwd_ciper.len);
    ret = write_file(fd, len_buf, 8, off);
    if (ret != 8) {
        goto FAIL;
    }

    write_n += ret;
    off += 8;

    size = record->passwd_ciper.len;
    str = record->passwd_ciper.str;
    ret = write_file(fd, str, size, off);
    if (ret != size) {
        goto FAIL;
    }

    write_n += size;
    off += size;

    sprintf(len_buf, "%08d", record->other.len);
    ret = write_file(fd, len_buf, 8, off);
    if (ret != 8) {
        goto FAIL;
    }

    write_n += ret;
    off += 8;

    if (!record->other.len) {
        goto OUT;
    }

    size = record->other.len;
    str = record->other.str;
    ret = write_file(fd, str, size, off);
    if (ret != size) {
        goto FAIL;
    }

    write_n += size;
    off += size;

OUT:
    return write_n;

FAIL:
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
    printf("user name:\n");
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


int user_write(object_t *obj) {
    int             ret;
    int             fd;
    user_t         *usr;
    record_t       *record;
	object_t	   *rcd_obj;
    char            ab_path[PATH_MAX + 1];
    char            len_buff[9];
    string_t        path;
    off_t           off;
    int             write_n;

    usr = obj_entry(obj, user_t, obj);
    path.len = 0;
    path.str = ab_path;
    get_user_path(obj, &path);

    fd = open(path.str, O_RDWR | O_TRUNC);
    if (fd < 0) {
        goto FAIL;
    }

    off = 0;
    sprintf(len_buff, "%08d", usr->record_num);
    write_n = write_file(fd, len_buff, 8, off);
    if (write_n != 8) {
        goto FD_FAIL;
    }

    off += 8;
    list_for_each_entry(rcd_obj, &obj->childs, sibling) {
		record = obj_entry(rcd_obj, record_t, obj);
        ret = record_write(fd, record, off);
        if (ret < 0) {
            goto FD_FAIL;
        }
        off += ret;
    }

    list_del(&usr->dirty_queue);

    close(fd);
    return OK;

FD_FAIL:
    close(fd);
FAIL:
    return ERR;
}


int user_del(object_t *obj){
    int         ret;
    object_t   *child_obj;
    user_t     *usr;
    char        ab_path[PATH_MAX + 1];
    string_t    path;
    
    list_for_each_entry(child_obj, &obj->childs, sibling) {
        ret = record_del(child_obj);
        if (ret != OK) {
            return ERR;
        }
    }

    usr = obj_entry(obj, user_t, obj);
    path.str = ab_path;
    path.len = 0;

    list_del(&obj->sibling);
    list_del(&usr->dirty_queue);
    get_user_path(obj, &path);
    
    ret = unlink(ab_path);
    if (!ret) {
        return ERR;
    }

    if (usr->file_name.len) {
        free(usr->file_name.str);
    }

    if (usr->user_name.len) {
        free(usr->user_name.str);
    }

    free(usr);

    return OK;
}

int user_move(object_t *obj, void *new_parent) {
    user_t          *new_usr;
    user_t          *old_usr;
    int              ret;
	object_t		*new_p;
    char             ab_path[PATH_MAX + 1];
    string_t         path;

	new_p = (object_t *)new_parent;
    if (new_p->options->type != obj_type_cla) {
        goto FAIL;
    }

    old_usr = obj_entry(obj, user_t, obj);
    new_usr = malloc(sizeof(user_t));
    if (!new_usr) {
        goto FAIL;
    }

    init_user(new_usr);
    if (old_usr->file_name.len) {
        new_usr->file_name.str = malloc(sizeof(char) * (old_usr->file_name.len + 1));
        if (!new_usr->file_name.str) {
            goto USR_FAIL;
        }

        sprintf(new_usr->file_name.str, "%s", old_usr->file_name.str);
        new_usr->file_name.len = old_usr->file_name.len;
    }

    if (old_usr->user_name.len) {
        new_usr->user_name.str = malloc(sizeof(char) * (old_usr->user_name.len + 1));
        if (!new_usr->user_name.str) {
            goto FILE_FAIL;
        }

        sprintf(new_usr->user_name.str, "%s", old_usr->user_name.str);
        new_usr->user_name.len = old_usr->user_name.len;
    }

    
    init_string(&path);
    path.str = ab_path;
    get_user_path(obj, &path);
    ret = unlink(ab_path);
    if (!ret) {
        goto UNAME_FAIL;
    }
    
    new_usr->record_num = old_usr->record_num;
    new_usr->obj.options = old_usr->obj.options;
    new_usr->obj.parent = new_p;
    list_replace(&new_usr->obj.childs, &old_usr->obj.childs);
    list_add(&new_usr->obj.sibling, &new_p->childs);

    if (old_usr->dirty) {
        clean_user_dirty(old_usr);
    }

    set_user_dirty(new_usr);
    return OK;
    //list_replace(&new_usr->obj.sl,list_head_t * old)

UNAME_FAIL:
    free(new_usr->user_name.str);
FILE_FAIL:
    free(new_usr->file_name.str);
USR_FAIL:
    free(new_usr);
FAIL:
    return ERR;
}

object_t *user_lookup(object_t *obj, string_t *name) {
	record_t			*des_rcd;
	object_t			*des_obj;

	list_for_each_entry(des_obj, &obj->childs, sibling) {
		if (des_obj->options->type != obj_type_rcd) {
			return NULL;
		}
		des_rcd = obj_entry(des_obj, record_t, obj);
		if (!strcmp(name, des_rcd->other.str)) {
			return des_obj;
		}
	}

	return NULL;
}

int record_show_self(object_t *obj) {
    record_t            *record;

    record = obj_entry(obj, record_t, obj);
    printf("record other info:\n%s\n", record->other.str);

    //decryption(record, passwd);
    return OK;
}

int record_del(object_t *obj) {
    user_t          *usr;
    record_t        *record;
    usr = obj_entry(obj->parent, user_t, obj);
    list_del(&obj->sibling);
    if (!usr->dirty) {
        set_user_dirty(usr);
    }

    record = obj_entry(obj->parent, record_t, obj);
    if (record->other.len) {
        free(record->other.str);
    }
    if (record->passwd_ciper.len) {
        free(record->passwd_ciper.str);
    }

    free(record);
    
    return OK;
}

int record_change(object_t *obj, void *data) {
    change_asgs_t           *arg;
    record_t                *record;
    uchar                   *str;
    string_t                 oth_str;
    string_t                 cip_str;
    user_t                  *usr;

    arg = (change_asgs_t *)data;
    if (arg->type != obj_type_rcd) {
        goto FAIL;
    }

    record = obj_entry(obj, record_t, obj);
    if (arg->other.len) {
        str = malloc(sizeof(char) * (arg->other.len + 1));
        if (!str) {
            goto FAIL;
        }
        sprintf(str, "%s", arg->other.str);
        //free(record->other.str);
        oth_str.str = record->other.str;
        oth_str.len = record->other.len;
        record->other.str = str;
        record->other.len = arg->other.len;
    }

    init_string(&oth_str);
    init_string(&cip_str);

    if (arg->cipher.len) {
        str = malloc(sizeof(char) * (arg->cipher.len + 1));
        if (!str) {
            goto OTH_FAIL;
        }
        sprintf(str, "%s", arg->cipher.str);
        //free(record->passwd_ciper.str);
        cip_str.str = record->passwd_ciper.str;
        cip_str.len = record->passwd_ciper.len;
        record->passwd_ciper.str= str;
        record->passwd_ciper.len = arg->cipher.len;
    }

    usr = obj_entry(obj->parent, user_t, obj);
    if (!usr->dirty) {
        set_user_dirty(usr);
    }

    if (oth_str.len) {
        free(oth_str.str);
    }

    if (cip_str.len) {
        free(cip_str.str);
    }

    return OK;

OTH_FAIL:
    free(record->other.str);
    record->other.str = oth_str.str;
    record->other.len = oth_str.len;
FAIL:
    return ERR;
}

int init_objs_tree(object_t *obj) {
	object_t		*tmp_obj;
	int				 ret;

	if (obj->options->type == obj_type_rcd) {
		return OK;
	}

	ret = obj->options->build(obj);
	if (ret != OK) {
		return ret;
	}
	
	list_for_each_entry(tmp_obj, &obj->childs, sibling) {
		ret = init_objs_tree(tmp_obj);
		if (ret != OK) {
			return ret;
		}
	}

	return OK;
}

