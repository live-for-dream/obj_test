#include "pass_file.h"

//"cd <path>"  change dir
//open <path/file/record>
//ch   <record>
//del   <path/file/record>
//add  <path/file/record>


#define cd          0xc7      // 'c' + 'd'
#define open        0xdf      // 'o' + 'p'
#define del         0xc9      // 'd' + 'e'
#define add         0xc5      // 'a' + 'd'
#define ch          0xcb      // 'c' + 'h'
#define root_path       "/home/centos-dev/test"

void exec_cd(char *cmd) {
    char *tmp;
    
    if (strncmp(cmd, "cd", strlen("cd"))) {
        printf("error options\n");
    }

    tmp = cmd + 2;
    if (*tmp != ' ') {
        printf("error option format\n");
        return;
    }

    while(*tmp == ' ') {
        tmp++;
    }

    if (*tmp == '.' && *(tmp + 1) == '.') {
        if (current->parent) {
            current = current->parent;
        } else {
            printf("this is root, cannot cd ..\n");
        }
        return;
    }

    
                
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
                
                break;
            case open:
                if (strncmp(line, "open", strlen("open"))) {
                    printf("error options\n");
                }
                break;
            case del:
                if (strncmp(line, "del", strlen("del"))) {
                    printf("error options\n");
                }
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
