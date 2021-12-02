# Groupchat

This program includes 3 files, a server, a client and a file that enables to send a message.


Steps to run the different files:

Initialize the server first
./chat_server_mq \<port number\>

initialize the client services that will run in the background
./chat_client_mq <ip> <server port number>

To send a message run the send.out file and pass the message as a argument
./send.out <message to send> 
