#include "pass_file.h"
#include <stdlib.h>

//"cd <path>"  change dir
//open <path/file/record>
//ch   <record>
//del   <path/file/record>
//add  <class|user|record> <name>


#define cd          0xc7      // 'c' + 'd'
#define open        0xdf      // 'o' + 'p'
#define del         0xc9      // 'd' + 'e'
#define add         0xc5      // 'a' + 'd'
#define ch          0xcb      // 'c' + 'h'
#define root_path       "/home/centos-dev/test"

object_t        *current;

void exec_cd(char *cmd) {
    char 			*tmp;
    string_t		 path;
	object_t		*tmp_obj;
	int				 ret;
	
    if (strncmp(cmd, "cd", strlen("cd"))) {
        printf("error options\n");
    }

    tmp = cmd + 2;
    if (*tmp != ' ') {
        printf("error option format\n");
        return;
    }

    while(tmp && *tmp == ' ') {
        tmp++;
    }

    if (*tmp == '.' && *(tmp + 1) == '.') {
        if (current->parent) {
            current = current->parent;
        } else {
            printf("this is root, cannot cd ..\n");
        }

		tmp += 2;
		if (!(*tmp)) {
			return;
		}

		if (*tmp != '/') {
			printf("error format, cd .. has been excuted\n");
		}

		tmp++;
    }

    path.len = strlen(tmp);
	path.str = tmp;
	tmp_obj = current->options->lookup(current, (void *)&path);
	if (!tmp_obj) {
		printf("no such path\n");
		return;
	}

	current = tmp_obj;

	ret = current->options->show_childs(current);
	return;
                
}

void exec_open(char *cmd) {
	char 			*tmp;
	string_t		 path;
	object_t		*tmp_obj;
	int				 ret;
	
	if (strncmp(cmd, "open", strlen("open"))) {
        printf("error options\n");
    }

	tmp = cmd + 4;
    if (tmp && *tmp != ' ') {
        printf("error option format\n");
        return;
    }

	while(*tmp == ' ') {
        tmp++;
    }

	path.str = tmp;
	path.len = strlen(tmp);
	tmp_obj = current->options->lookup(current, (void *)&path);
	if (!tmp_obj) {
		printf("no such obj\n");
	}

	ret = tmp_obj->options->show_self(tmp_obj);
	return;
}

void exec_del(char * cmd) {
	char 			*tmp;
	string_t		 path;
	object_t		*tmp_obj;
	int				 ret;
	
	if (strncmp(cmd, "del", strlen("del"))) {
        printf("error options\n");
    }

	tmp = cmd + 3;
    if (*tmp != ' ') {
        printf("error option format\n");
        return;
    }

	while(tmp && *tmp == ' ') {
        tmp++;
    }

	path.str = tmp;
	path.len = strlen(tmp);
	tmp_obj = current->options->lookup(current, (void *)&path);
	if (!tmp_obj) {
		printf("no such obj\n");
		return;
	}

	ret = tmp_obj->options->delete(tmp_obj);
	return;
}

void exec_add_class(char * name) {
	//to complete
	string_t		 path;
	object_t		*tmp_obj;
	int				 ret;
	create_args_t	 arg;

	path.str = name;
	path.len = strlen(name);

	if (current->options->type != obj_type_cla) {
		printf("cannot create class in user or record\n");
		return;
	}

	tmp_obj = current->options->lookup(current, (void *)&path);
	if (tmp_obj) {
		printf("the class exsists\n");
		return;
	}

	init_create_arg(&arg);
	arg.type = obj_type_cla;
	arg.name.len = path.len;
	arg.name.str = path.str;
	ret = current->options->create(current, &arg);
	if (ret != OK) {
		printf("add class failed\n");
	}

	return;
}

void exec_add_user(char * name) {
	//to complete
	string_t		 path;
	object_t		*tmp_obj;
	int				 ret;
	create_args_t	 arg;

	path.str = name;
	path.len = strlen(name);

	if (current->options->type != obj_type_cla) {
		printf("cannot create user in user or record\n");
		return;
	}

	tmp_obj = current->options->lookup(current, (void *)&path);
	if (tmp_obj) {
		printf("the user exsists\n");
		return;
	}

	init_create_arg(&arg);
	arg.type = obj_type_usr;
	arg.name.len = path.len;
	arg.name.str = path.str;
	ret = current->options->create(current, &arg);
	if (ret != OK) {
		printf("add user failed\n");
	}

	return;
}

void exec_add_record(char * name) {
	//to complete
	string_t		 path;
	object_t		*tmp_obj;
	char			*line;
	size_t			 len;
	int				 ret;
	create_args_t	 arg;

	path.str = name;
	path.len = strlen(name);

	if (current->options->type != obj_type_usr) {
		printf("cannot create record in class or record\n");
		return;
	}
	
	tmp_obj = current->options->lookup(current, (void *)&path);
	if (tmp_obj) {
		printf("the user exsists\n");
		return;
	}

	init_create_arg(&arg);
	arg.type = obj_type_rcd;
	arg.name.len = path.len;
	arg.name.str = path.str;

	printf("input cipher\n");
	ret = getline(&line, &len, stdin); 
	if (ret < 0) {
		printf("get input error");
		return;
	}

	arg.plain.len = len;
	arg.plain.str = malloc((len + 1) * sizeof(char));
	if (!arg.plain.str) {
		goto PLAIN_FAIL;
	}
	
	sprintf(arg.plain.str, "%s", line);
	free(line);
	line = NULL;

	printf("input other\n");
	ret = getline(&line, &len, stdin); 
	if (ret < 0) {
		printf("get input error");
		line = NULL;
		goto OTH_FAIL;
	}

	arg.other.len = len;
	arg.other.str = malloc((len + 1) * sizeof(char));
	if (!arg.other.str) {
		goto OTH_FAIL;
	}
	sprintf(arg.other.str, "%s", line);
	
	ret = current->options->create(current, &arg);
	if (ret != OK) {
		printf("add user failed\n");
	}

	if (arg.other.len) {
		free(arg.other.str);
	}
OTH_FAIL:
	if (arg.plain.len) {
		free(arg.plain.str);
	}
PLAIN_FAIL:
	if (line) {
		free(line);
	}
FAIL:
	return;

}



void exec_add(char * cmd) {
	char 			*tmp;
	string_t		 path;
	object_t		*tmp_obj;

	if (strncmp(cmd, "add", strlen("add"))) {
        printf("error options\n");
    }

	tmp = cmd + 3;
	if (*tmp != ' ') {
        printf("error option format\n");
        return;
    }

	while(*tmp == ' ') {
        tmp++;
    }

	if (*tmp == 'c') {
		if (!strncmp(tmp, "class", strlen("class"))) {
			tmp += 5;
			while(tmp && *tmp == ' ') {
     	    	tmp++;
    		}
			exec_add_class(tmp);
			return;
		} else {
			printf("error option format\n");
			return;
		}
	} 

	if (*tmp == 'u') {
		if (!strncmp(tmp, "user", strlen("user"))) {
			tmp += 4;
			while(tmp && *tmp == ' ') {
     	    	tmp++;
    		}
			exec_add_user(tmp);
			return;
		} else {
			printf("error option format\n");
			return;
		}
	}

	if (*tmp == 'r') {
		if (!strncmp(tmp, "record", strlen("record"))) {
			tmp += 6;
			while(tmp && *tmp == ' ') {
     	    	tmp++;
    		}
			exec_add_record(tmp);
			return;
		} else {
			printf("error option format\n");
			return;
		}
	}

	printf("error option format\n");
	return;
}


void parse_cmd() {
    char        *line;
    char        *tmp;
    size_t       len;
    int          op;
    
    while(1) {
        current->options->show_childs(current);
        printf("input options\n");
        getline(&line, &len, stdin);
        op = (int)line[0] + (int)line[1];
        switch (op) {
            case cd:
                exec_cd(line);
                break;
            case open:
                exec_open(line);
                break;
            case del:
                exec_del(line);
                break;
            case add:
                exec_add(line);
                break;
            case ch:
                if (strncmp(line, "ch", strlen("ch"))) {
                    printf("error options\n");
                }
                break;
            default:
                printf("error options\n");
        }
		if (line) {
			free(line);
			line = NULL;
		}
    }
}

int main(int argc, char *argv[]) {
    string_t        path;
	int				ret;
    
    init_string(&path);
    if (argc < 2) {
        init_root(NULL);
    } else {
        path.str = argv[1];
        path.len = strlen(argv[1]);
        init_root(&path);
    }
	
    current = get_root_obj();
	ret = init_objs_tree(current);
	if (ret != OK) {
		return ret;
	}
    parse_cmd();
}
