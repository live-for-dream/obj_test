#ifndef PASS_FILE_INCLUDED
#define PASS_FILE_INCLUDED
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "objects.h"
#include <sys/types.h>
#include <dirent.h>

/*file layout nextoff is for the same username;
record_num 
passwd_len passworld other_len other 
...
*/
#define md5_len         33;

typedef struct record_s record_t;
#define true            1
#define false           0

typedef struct create_args_s {
    int         type;
    string_t    name;
    string_t    other;
    string_t    plain;
} create_args_t;

typedef struct change_asgs_s {
    int         type;
    string_t    name;
    string_t    other;
    string_t    cipher;
    void       *info;
} change_asgs_t;

struct record_s {
    object_t    obj;
    uint        total_len;
    uint        next_off;
    string_t    passwd_ciper;
    string_t    other;
};

typedef struct user_s user_t;
struct user_s {
    object_t    obj;
    string_t    user_name;
    string_t    file_name;
    list_head_t dirty_queue;
    int         record_num;
    int         dirty;
};

typedef struct class_s class_t;

struct class_s {
    object_t    obj;
    string_t    name;
    string_t    path;
    DIR        *dir;
};


#define init_record(record) \
    init_string((record)->passwd_ciper);\
    init_string((record)->other)\
    (record)->total_len = (record)->next_off = 0;\
    init_obj(&record->obj)

#define init_user(usr) \
    init_obj(&(usr)->obj);\
    init_string(&(usr)->user_name);\
    init_string(&(usr)->file_name);\
    INIT_LIST_HEAD(&(usr)->dirty_queue);\
    (usr)->record_num = 0;\
    (usr)->dirty = 0
/*    
#define init_class(cla) \
	do { \
    init_obj(&(cla)->obj); \
    init_string(&(cla)->name); \
    init_string(&(cla)->path); \
    (cla)->dir = NULL; \
	} while(0)
*/
static inline void init_class(class_t *cla) {
	init_obj(&(cla->obj));
    init_string(&cla->name);
    init_string(&cla->path);
    cla->dir = NULL; 
} 

#define init_create_arg(arg) \
	init_string(&(arg)->name);\
	init_string(&(arg)->other);\
	init_string(&(arg)->cipher);\
	(arg)->type = 0;\
	(arg)->info = NULL

int init_root(string_t *path);
object_t *get_root_obj();

int insert_record();
int record_read(int fd, record_t **record_r, off_t off);
int start_record_tree(string_t path);

int class_build(object_t *obj);
int class_show_self(object_t *obj);
int class_show_child(object_t *obj);
int class_create(object_t *parent, void *data);
int class_write(object_t *obj);
int class_del(object_t *obj);
int class_move(object_t *obj, object_t *parent);
object_t *class_lookup(object_t *obj, string_t *name);

int user_build(object_t *obj);
int user_show_self(object_t *obj);
int user_show_childs(object_t * obj);
int user_create(object_t *parent, void *data);
int user_write(object_t *obj);
int user_del(object_t *obj);
int user_move(object_t *obj, void *new_parent);
object_t *user_lookup(object_t *obj, string_t *name);


int record_show_self(object_t *obj);
int record_del(object_t *obj);
int record_change(object_t *obj, void *data);

#endif
