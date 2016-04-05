all:
	gcc -std=gnu99 -I./include ./test/test.c ./src/op_epoll.c ./src/threadpool.c -o server -lpthread
	cp server ./test/server
clean:
	rm server
	rm ./test/server
