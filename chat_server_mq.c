
/*****************************************************************************/
/*** tcpserver.c                                                           ***/
/***                                                                       ***/
/*** Demonstrate an TCP server.                                            ***/
/*****************************************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

void panic(char *msg);
#define panic(m)	{perror(m); abort();}

/*Device driver related*/
char ledOn = '1';
char ledOff = '0';
int fd0;
int led_status=0;

/*The set of possible commands*/

#define SEND_COMMAND			0
#define KILL_COMMAND			1
#define STATUS_COMMAND 			2
#define NULL_COMMAND 			3

#define MAX_CLIENT_NUM			3

typedef struct command command_t;

struct command
{
	unsigned int subtype;
	unsigned int value;
	char msg[128];
}; 

typedef struct client_socket_into client_socket_info_t;
struct client_socket_into
{
	int socket;
	int state;
	int index;
	char client_name[20];
}; 

typedef enum { AFK=0, ONLINE=1 }status_t;

status_t status;

client_socket_info_t socket_table[MAX_CLIENT_NUM];

int threads=0;

static void sendPeriodicUpdate(int signo)
{
	int i;
	command_t cmd;

	  if(signo==SIGALRM)
	  {
		cmd.subtype = STATUS_COMMAND;
		strcpy(cmd.msg, "ARE YOU OKAY?");
		cmd.value = 0;
 	     for(i=0; i<threads; i++)
 	     {
	  		if(socket_table[i].state)
	  			send(socket_table[i].socket,&cmd,sizeof(cmd),0);
	  	}

      /*  if (!led_status)
        {
            write(fd0, &ledOn, 1);
            led_status=1;
        }
        else
        {
            write(fd0, &ledOff, 1);
            led_status = 0;  
        }*/
        

	  }
}
/*****************************************************************************/
/*** This program creates a simple echo server: whatever you send it, it   ***/
/*** echoes the message back.                                              ***/
/*****************************************************************************/
void *threadfuntion(void *arg)                    
{	
	command_t cmd;
	client_socket_info_t *info = (client_socket_info_t *)arg;   /* get & convert the socket */
	
	while(1)
	{
		if(recv(info->socket,&cmd,sizeof(cmd),0) > 0);
		{
            switch(cmd.subtype)
            {
                case SEND_COMMAND:

					   if (!info->state)
					   {
						   info->state = ONLINE;
						   printf("Client %s is now ONLINE!\n", info->client_name);
					   }
					   
						printf("Client %s wants to share the message -> %s\n",info->client_name, cmd.msg);

                       //Build the message////////////
                       
                        char aux[128];
                        strcpy(aux,info->client_name);
                        strcat(aux, " said: ");
						strcat(aux,cmd.msg);
                        strcpy(cmd.msg,aux);

                       ///////////////////////////////

                       for (int i = 0; i < MAX_CLIENT_NUM; i++)
                        {
                            if (socket_table[i].state)// && socket_table[i].index != info->index)
                            {
                                send(socket_table[i].socket, &cmd, sizeof(cmd),0);
                            }
						}	
                break;

                case KILL_COMMAND:
                        
                        printf("%s left the chat\n", info->client_name);

                        int socket=info->socket;
                        //roll the struct one space back starting at the thread to be killed
						//note: not working very well
                        if(threads > 1)
                          {
                              printf("here");
                            for (int i = info->index; i < threads; i++)
                            {
                                if(i+1 < MAX_CLIENT_NUM)
                                    socket_table[i].index = (socket_table[i+1].index - 1);
                                    socket_table[i].socket = socket_table[i+1].socket;
                                    socket_table[i].state = socket_table[i+1].state;
                                    strcpy(socket_table[i].client_name, socket_table[i+1].client_name);
                            }
                          }

                        threads--;                  //free the thread space
                        return 0;                   //close thread
                break;

				case STATUS_COMMAND:
						if (cmd.value == AFK && socket_table[info->index].state)
						{
							printf("Client %s is now AFK!\n", info->client_name);
							socket_table[info->index].state = AFK;
							cmd.subtype = SEND_COMMAND;
							strcpy(cmd.msg, "You are now AFK!");
							send(info->socket, &cmd, sizeof(cmd),0);
						}
						else
							socket_table[info->index].state = cmd.value;

				case NULL_COMMAND:
				break;
            }
        } 
    }
	return 0;                           /* terminate the thread */

}

int main(int count, char *args[])
{	struct sockaddr_in addr;
	int listen_sd, port;

	if ( count != 2 )
	{
		printf("usage: %s <protocol or portnum>\n", args[0]);
		exit(0);
	}

	/*---Get server's IP and standard service connection--*/
	if ( !isdigit(args[1][0]) )
	{
		struct servent *srv = getservbyname(args[1], "tcp");
		if ( srv == NULL )
			panic(args[1]);
		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	}
	else
		port = htons(atoi(args[1]));

	/*--- create socket ---*/
	listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if ( listen_sd < 0 )
		panic("socket");

	/*--- bind port/address to socket ---*/
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = INADDR_ANY;                   /* any interface */
	if ( bind(listen_sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
		panic("bind");

    //Device drivers setup  prints for testing purposes only
/*
    //printf("\n\nInserting Device Driver...\n");
    system("insmod led.ko");

    fd0 = open("/dev/led0", O_WRONLY);
    system("echo none >/sys/class/leds/led0/trigger"); //enabling permissions to access the led
*/
    //
	/*--- make into listener with 10 slots ---*/
	if ( listen(listen_sd, MAX_CLIENT_NUM) != 0 )
		panic("listen")

	/*--- begin waiting for connections ---*/
	else
	{	
		struct itimerval itv;

		signal(SIGALRM,sendPeriodicUpdate);

		//ualarm(300,300);	
		itv.it_interval.tv_sec = 5;
		itv.it_interval.tv_usec = 0;
		itv.it_value.tv_sec = 5;
		itv.it_value.tv_usec = 0;
		setitimer (ITIMER_REAL, &itv, NULL);
	
		pthread_t servert[MAX_CLIENT_NUM];

		while (1)                         /* process all incoming clients */
		{
			//int i=0;
			int n = sizeof(addr);
			int sd = accept(listen_sd, (struct sockaddr*)&addr, &n);     /* accept connection */

			if(sd!=-1 && threads<MAX_CLIENT_NUM)
			{
				if(recv(sd,socket_table[threads].client_name,sizeof(socket_table[threads].client_name),0)>0)
				{
					socket_table[threads].socket=sd;
					socket_table[threads].state=1;		/*means connection opened*/
					socket_table[threads].index=threads;
					printf("Client %s has entered\n",socket_table[threads].client_name);
					
					pthread_create(&servert[threads], 0, threadfuntion, &socket_table[threads]);       /* start thread */
					pthread_detach(servert[threads]);                      /* don't track it */

					threads++;
				}
			}
		}
	}
}
