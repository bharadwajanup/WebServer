all:
	gcc TCP/server.c -o TCP/server
	gcc TCP/client.c -o TCP/client
	gcc -pthread Threads/thread_server.c -o Threads/server
	gcc -pthread Threads/thread_client.c -o Threads/client
	gcc UDP/udp_server.c -o UDP/server
	gcc UDP/udp_client.c -o UDP/client
