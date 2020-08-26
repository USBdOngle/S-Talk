OUT=s-talk

all:
	gcc -std=gnu99 io.c common.c networking.c main.c instructorList.o -Wall -Werror -pthread -o $(OUT)

clean:
	rm $(OUT)