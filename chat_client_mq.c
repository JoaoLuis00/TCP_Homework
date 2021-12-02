
/*****************************************************************************/
/*** tcpclient.c                                                           ***/
/***                                                                       ***/
/*** Demonstrate an TCP client.                                            ***/
/*****************************************************************************/

#include <stdio.h>
#include <mqueue.h>   /* mq_* functions */
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

/* name of the POSIX object referencing the queue */
#define MSGQOBJ_NAME    "/client_queue"
/* max length of a message (just for this process) */
#define MAX_MSG_LEN     100000

mqd_t msgq_id;
unsigned int sender;

/*The set of possible commands*/
#define SEND_COMMAND			0
#define KILL_COMMAND			1
#define STATUS_COMMAND			2
#define NULL_COMMAND 			3

pthread_t sendt,receivet;

typedef struct command command_t;
struct command
{
	unsigned int subtype;
	unsigned int value;
	char msg[128];
}; 
int old_sid;
pthread_mutex_t sent_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef enum { AFK=0, ONLINE=1 } status_t;

status_t status;

const char command_send[]="/send";
const char command_kill[]="/kill";

int sent=0;

void panic(char *msg);
#define panic(m)	{perror(m); abort();}
 

static void checkstatus(int signo)
{
	if (signo == SIGALRM)
	{
		if(sent > 0)
			{
				status = ONLINE;
				sent = 0;
			}
		else
			status = AFK;	
	}
	
}


void *sendfunction(void* arg)
{
    int sd = *(int*)arg;            
	command_t cmd;
	struct mq_attr msgq_attr;
    int msgsz;
    char msgcontent[MAX_MSG_LEN];
    
	while(1)
	{
		mq_getattr(msgq_id, &msgq_attr);
		if (msgq_attr.mq_curmsgs > 0)
		{

			msgsz = mq_receive(msgq_id, msgcontent, MAX_MSG_LEN, &sender);
				if(msgsz == -1)
				{
					perror("In mq_receive()");
					exit(1);
				}
				else if (msgsz >=0 )
				{
					if(!status) //if AFK
					status = ONLINE;
					printf("You: %s\n",msgcontent);
					sent++;
					cmd.subtype = SEND_COMMAND;
					strcpy(cmd.msg, msgcontent);
					send(sd,&cmd,sizeof(cmd),0);
				}
        
		}
	}
}

void *receivefunction(void* arg)
{
    int sd = *(int*)arg;            /* get & convert the socket */
	command_t cmd;
	char aux[128];

	while(1)
	{
        if(recv(sd,&cmd,sizeof(cmd),0)>0)
		{
            switch (cmd.subtype)
			{
			case SEND_COMMAND:
					printf("%s\n",cmd.msg);
				break;

			case STATUS_COMMAND:
					cmd.value = status;
					send(sd, &cmd, sizeof(cmd),0);
			default:
				break;
			}
        }
	}
}

/****************************************************************************/
/*** This program opens a connection to a server using either a port or a ***/
/*** service.  Once open, it sends the message from the command line.     ***/
/*** some protocols (like HTTP) require a couple newlines at the end of   ***/
/*** the message.                                                         ***/
/*** Compile and try 'tcpclient lwn.net http "GET / HTTP/1.0" '.          ***/
/****************************************************************************/
int main(int count, char *args[])
{	struct hostent* host;
	struct sockaddr_in addr;
	int sd, port;

    unsigned int msgprio = 1;
    	pid_t my_pid = getpid();
    	struct mq_attr msgq_attr;

	if ( count != 3 )
	{
		printf("usage: %s <servername> <protocol or portnum>\n", args[0]);
		exit(0);
	}

	if (msgprio == 0) {
        	printf("Usage: %s [-q] -p msg_prio\n", args[0]);
        	exit(1);
    	}

    /* opening the queue using default attributes  --  mq_open() */
  	    msgq_id = mq_open(MSGQOBJ_NAME, O_RDWR | O_CREAT , S_IRWXU | S_IRWXG, NULL);
    	if (msgq_id == (mqd_t)-1) {
        	perror("In mq_open()");
        	exit(1);
    	}
    		
	/*---Get server's IP and standard service connection--*/
	host = gethostbyname(args[1]);
	//printf("Server %s has IP address = %s\n", args[1],inet_ntoa(*(long*)host->h_addr_list[0]));
	if ( !isdigit(args[2][0]) )
	{
		struct servent *srv = getservbyname(args[2], "tcp");
		if ( srv == NULL )
			panic(args[2]);
		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	}
	else
		port = htons(atoi(args[2]));

	/*---Create socket and connect to server---*/
	sd = socket(PF_INET, SOCK_STREAM, 0);        /* create socket */
	if ( sd < 0 )
		panic("socket");
	memset(&addr, 0, sizeof(addr));       /* create & zero struct */
	addr.sin_family = AF_INET;        /* select internet protocol */
	addr.sin_port = port;                       /* set the port # */
	addr.sin_addr.s_addr = *(long*)(host->h_addr_list[0]);  /* set the addr */

	struct itimerval itv;

	signal(SIGALRM,checkstatus);

	//ualarm(300,300);	
	itv.it_interval.tv_sec = 60;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = 60;
	itv.it_value.tv_usec = 0;
	setitimer (ITIMER_REAL, &itv, NULL);


	/*---If connection successful, send the message and read results---*/
	if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
		{
            char name[20];
            printf("Enter your name: ");
            scanf("%[^\n]", name);
            if(send(sd,name,sizeof(name),0)<0)
            {
                printf("Unable to connect\n");
                return 0;
            }
			status = ONLINE;
            printf("Welcome %s\n",name);

			///////////////////////DAEMON PART///////////////////////////
			int pid,sid;
			pid = fork();
			// on error exit
			if (pid < 0) { 
				perror("fork");
				exit(EXIT_FAILURE);
			}

			// parent process (exit)
			if (pid > 0){  
				printf("Daemon PID: %d\n", pid);	
				exit(EXIT_SUCCESS); 
			}
			
            	//STEP 2 - create a new session
            sid = setsid(); // returns session ID (PID of leader process) 

            // on error exit
            if (sid < 0) { 
                perror("setsid");
                exit(EXIT_FAILURE);
            }
            
            
            //STEP 3 - change to the root directory ('/') 
            // on error exit
            if (chdir("/") < 0) { 
                perror("chdir");
                exit(EXIT_FAILURE);
            }
            umask(0);

            	//STEP 5 - close all open file descriptors that may be inherited from the parent process 
            close(STDIN_FILENO);  // close standard input file descriptor
            close(STDOUT_FILENO); // close standard output file descriptor
            close(STDERR_FILENO); // close standard error file descriptor

			//////////////////////////////////////////////////////////////

            pthread_create(&sendt, 0,sendfunction,&sd);
            pthread_create(&receivet, 0,receivefunction,&sd);

            //pthread_join(sendt,&sd);
            pthread_join(receivet,&sd);

            pthread_exit(NULL);
        }
    else
		panic("connect");
}
