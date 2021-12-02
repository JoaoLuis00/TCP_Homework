Once the server is open and the clients are connected to it it's possible to communicate with other clients via the 
file groupchat_client_send.c

One essential thing to consider in this group chat is that clients are connected in different devices and the network
is the same (since the TCP IP connection is local, since there's no port forwarding). This implicates that message
queues (used to communicate from the groupchat_client_send to the client) are unique for each device.

To know who is connected to the server, the user must type /clients. This request would transfered to the client
daemon, which would then parse the command and send it to the server. The server, upon receiving the command, would return
to the daemon the clients' name list which would be returned to the terminal of which the user sent the initial command.

To create or join a new channel, in order to send messages to specific clients, the user must type the command
/channel <name1> <name2>. This would create a channel specific to the user, client 1 and client 2. In order to send messages 
to everyone connected to the server, one could use the command /channel all.

To send a message the command /send <message> is used. This would redirect the <message> to the clients specified in the
previous /channel command. 