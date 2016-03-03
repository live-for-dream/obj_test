obj_t = test
obj_t_src = test.c
obj_a = pass_file.a
obj_a_src = pass_file.c types.c

all:$(obj_t) $(obj_a)
$(obj_t):$(obj_a) $(obj_t_src)
	gcc $(obj_t_src) -g -o $@ $(obj_a)
$(obj_a):$(obj_a_src)
	gcc -c $^ -g
	ar rcs $@ $(patsubst %.c,%.o,$(obj_a_src) )