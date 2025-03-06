STACK_SIZE=256
NR_TASKLETS=16

all: clean main

main: main.c utility.c test2
	gcc --std=c99 main.c utility.c `dpu-pkg-config --cflags --libs dpu` -o main

test2: test2.c
	dpu-upmem-dpurte-clang -DNR_TASKLETS=$(NR_TASKLETS) -DSTACK_SIZE_DEFAULT=$(STACK_SIZE) -o test2 test2.c

clean:
	rm -f main test2
