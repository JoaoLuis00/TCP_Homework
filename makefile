CC = gcc


all:
	$(CC) chat_server_mq.c -o chat_server_mq -pthread
	$(CC) chat_client_mq.c -o chat_client_mq -pthread -lrt
	$(CC) send.out.c -o send.out -pthread -lrt
 
# Compile Server
server:
	$(CC) chat_server_mq.c -o chat_server_mq -pthread -lrt
	
#Compile Client
client: 
	$(CC) chat_client_mq.c -o chat_client_mq -pthread -lrt 
		
#Compile send (will send via Posix Message Queue to client service)
send.out: 
	$(CC) send.c -o send.out-pthread

clean: 
	rm -f  *.o send.out chat_client_mq chat_server_mq
