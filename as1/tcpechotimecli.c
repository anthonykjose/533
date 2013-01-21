#include	"unp.h"
#include 	<stdio.h>

/*********************************************************************
 *
 * Function    :  sig_chld
 *
 * Description :  Handles SIG_CHLD signal.			
 *
 * Parameters  :  int signo
 *	
 * Returns     :  VOID
 *
 *********************************************************************/
void sig_chld( int signo )
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{	
		printf("child %d terminated : SIGCHLD recieved..\n", pid);
	}
	return;

}

/*********************************************************************
 *
 * Function    :  checkIfIP
 *
 * Description :  Performs checks to see if input is an IPv4 address.			
 *
 * Parameters  :
 *          1  :  char *s = IP address string (input)
 *	
 * Returns     :  1=> If valid IPv4 address, 0 otherwise
 *
 *********************************************************************/
int checkIfIP( char *s )
{
	char *p;
	int count = 0;

	if( strlen( s ) > 15 )
	{
		return 0;
	}	
	p = strtok( s,"." );

	while( p != NULL )
	{
		if( atoi( p ) > 255 || atoi( p ) < 0 )
        {
			return 0;
        }	
		p = strtok( NULL,"." );		
		count++;
	}	

	if( count != 4 )
	{
		return 0;
	}	

	return 1;
}

int main( int argc, char **argv )
{
	int					sockfd;
	char			*ptr, **pptr;
	struct sockaddr_in	servaddr;
	char   			 service[15]; 
	int 	   serv_port = 55555;
	struct 		    hostent	*hptr;
	in_addr_t 				ipAdd;
	char			str[INET_ADDRSTRLEN];
	char 			*serverIP;

	/* Handle SIGCHLD signal in client process */
	signal( SIGCHLD, sig_chld );

	if ( argc != 2 )
	{	
		err_quit("usage: tcpcli <IPaddress>/<domain_name>");
	}

	/*Preparing to check if argument 1 is a valid IP address or not. */
	char *ipcheck = (char *)malloc( ( strlen( argv[1] ) + 1 )*sizeof(char) );
	strcpy( ipcheck, argv[1] );

	if( checkIfIP( (char *)ipcheck ) == 1 )
	{
		/* Retrieving host structure using IP address  */	
		serverIP = argv[1];
		ipAdd =  inet_addr( argv[1] );

		if( (hptr = gethostbyaddr( (char*) &ipAdd, sizeof( ipAdd ), AF_INET )) == NULL )
		{
				printf("Address is invalid..\n");
				err_msg("gethostbyaddr error for host: %s: %s",
						argv[1], hstrerror(h_errno));
				return 0;
		}
		printf("Server Hostname : `%s`\n", hptr->h_name);
		printf("Server IP : %s\n", serverIP);
		
	}	
	else
	{	
		/* When domain name is given we pick an IP address from the IP address list. */
		ptr = *++argv;

		/* Retrieve host structure using domain name */
		if ( (hptr = gethostbyname( ptr ) ) == NULL) 
		{
			err_msg("gethostbyname error for host: %s: %s",
					ptr, hstrerror(h_errno));
			return 0;
		}
		
		/* Pick an IP address */
		switch ( hptr->h_addrtype ) 
		{
			case AF_INET:
				pptr = hptr->h_addr_list;
				serverIP = inet_ntop( hptr->h_addrtype, *pptr, str, sizeof(str) ) ;
				break;

			default:
				err_ret("unknown address type");
				break;
		}
		printf("Server IP : %s\n", serverIP );
	}	

	for(;;)
	{
 		printf( "*************************************************************************\n" );
       	fputs( "Enter the service you would like to use : echo / time / quit(to exit)\n", stdout );
 		printf( "*************************************************************************\n" );
       
		if( fgets( service, 15, stdin ) == NULL )
		{
			fputs( "Please retry input..\n", stdout );
			continue;
		}		
		
		if( strcmp( service, "quit\n" ) == 0 )
		{
			exit(1);
		}
		else if( strcmp( service, "echo\n" ) != 0 
				 && strcmp( service, "time\n" ) != 0 )
		{
				fputs( "Not a valid service or command..Please retry..\n", stdout );
				continue;
		}		
		
		pid_t childpid, pid; 
		int stat, fd[2]; 
		char buf[80];

		printf( "Forking off a child..\n" );
		if( pipe( fd ) == -1 )
		{
			printf("Error in creating pipe for child-parent IPC. Error code : %d \n",errno );
		}	
		if( ( childpid = fork() ) == 0 )
		{
			/* Close the read end of the pipe. */	
			close( fd[0] );
			sprintf(buf,"%d %d",fd[0],fd[1]);

			if( strcmp( service, "echo\n" ) == 0 )
			{
				if ( execlp( "xterm" , "xterm" , "-e" , "./echo_cli" , serverIP, (char *) buf , (char *) NULL ) )
				{
					printf( "This is such a big ERROR !" );
					exit(1);	
				}
			}	
			else if( strcmp( service, "time\n" ) == 0 )
			{
				if ( execlp( "xterm" , "xterm" , "-e" , "./time_cli" , serverIP, (char *) buf , (char *) NULL ) )
				{
					printf( "This is such a big ERROR !" );
					exit(1);	
				}
			}	
		}
		else if( childpid < 0 )
		{
			/* fork() failed */
			printf("Forking parent process failed on client. Error code : %d \n", errno);
		}
		else
		{
			/* Close the write end of the pipe at the parent */
			close( fd[1] ); 
			char readbuffer[ MAXLINE ];
			int nbytes;

       		printf( "*************************************************************************\n" );
       		printf( "\tChild Status Information\t\n" );
        	printf( "*************************************************************************\n" );
       
			while( ( pid = waitpid( -1, &stat, WNOHANG ) ) == 0 )
			{
				memset( readbuffer, 0, sizeof( readbuffer ) );
			    if( nbytes = read( fd[0], readbuffer, sizeof( readbuffer ) ) != 0 )
		        {
		        	if( strcmp( readbuffer,"SERVER_NORESP\n" ) == 0 )
		        	{
		        		printf( "No response from server..Closing client..\n" );
						exit(0);		        			
		        	}	
		        	if( strcmp( readbuffer , "SERVER_UNAVAILABLE\n" ) == 0 )
		        	{
		        		/* The server is unavailable.... */
		        		printf( "Unable to connect to server..Closing client..\n" );
						exit(0);		        			
		        	}	
		        	printf( "Child %d Status: %s\n", childpid ,readbuffer );
				}
			}	
		}	
	}	
}
