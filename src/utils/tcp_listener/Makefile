libtcp_listener.so: tcp_listener.o
	gcc -Wall -Wextra -shared tcp_listener.o -o libtcp_listener.so -lpthread
tcp_listener.o: tcp_listener.c tcp_listener.h
	gcc -Wall -Wextra -fPIC -c tcp_listener.c -o tcp_listener.o
clean:
	rm -f *.o
install:
	cp *.so /usr/local/lib