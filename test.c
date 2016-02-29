#include "pass_file.h"

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
	}

	current = tmp_obj;

	ret = current->options->show_childs(current);
	return;
                
}

void exec_open(char *cmd) {
	char 			*tmp;
	string_t		 path;
	object_t		*tmp_obj;
	
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
	}

	ret = tmp_obj->options->del(tmp_obj);
	return;
}

void exec_add_class(char * name) {
	//to complete
}

void exec_add_user(char * name) {
	//to complete
}

void exec_add_record(char * name) {
	//to complete
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
                if (strncmp(line, "add", strlen("add"))) {
                    printf("error options\n");
                }
                break;
            case ch:
                if (strncmp(line, "ch", strlen("ch"))) {
                    printf("error options\n");
                }
                break;
            default:
                printf("error options\n");
        }
    }
}

object_t        *current;
int main(int argc, char *argv[]) {
    string_t        path;
    
    init_string(path);
    if (argc < 2) {
        init_root(NULL);
    } else {
        path.str = argv[1];
        path.len = strlen(argv[1]);
        init_root(&path);
    }

    current = get_root_obj();
    parse_cmd();
}
