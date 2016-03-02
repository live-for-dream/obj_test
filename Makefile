obj_t = test

obj_a = pass_file.a
obj_a_src = pass_file.c

all:$(obj_t) $(obj_a)
$(obj_t):$(obj_a) test.c
	gcc $^ -g -o $@
$(obj_a):$(obj_a_src)
	gcc -c $^ -g
	ar rcs obj_a $(patsubst %.c,%.o,$(obj_a_src) )