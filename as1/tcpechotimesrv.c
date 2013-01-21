#include 	<fcntl.h>
#include 	<sys/file.h>
#include	"unpthread.h"
#include 	<time.h>

static void	*doitecho(void *);		/* each thread executes this function */
static void	*doittime(void *);		/* each thread executes this function */

/*********************************************************************
 *
 * Function    :  readable_timeo
 *
 * Description :  This function will make use of select to simulate 
 *				  a wait of 5 seconds before the time_cli function
 *				  can write the timestamp to the tcp socket. Apart
 *				  from this, it will help catch server termination. 
 *
 * Parameters  :
 *          1  :  int fd = socket fd on which to perform select.
 *          2  :  int sec = Time wait to simulate (in seconds).
 *
 * Returns     :  0 => When the timeout occurs.
 *
 *********************************************************************/
int readable_timeo( int fd, int sec )
{
	fd_set			rset;
	struct timeval	tv;

	FD_ZERO( &rset );
	FD_SET( fd, &rset );

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	return( select( fd+1, &rset, NULL, NULL, &tv ) );
		/* > 0 if descriptor is readable */
}

/*********************************************************************
 *
 * Function    :  sig_chld
 *
 * Description :  Signal handler for SIG_CHLD signal. Will be caught
 * 				  when client process is terminated.	  
 *
 * Parameters  :
 *          1  :  int signo 
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
		printf("child %d terminated.\n", pid);
	}
	return;

}

/*********************************************************************
 *
 * Function    :  str_echo
 *
 * Description :  Performs the str_echo service for the server.
 *
 * Parameters  :
 *          1  :  int sockfd = client connection socket descriptor
 *	
 * Returns     :  VOID
 *
 *********************************************************************/
void str_echo( int sockfd )
{
	ssize_t		n;
	char		line[MAXLINE];

	for ( ; ; ) 
	{
		memset( line, 0, sizeof( line ) );
		if ( ( n = readline( sockfd, line, MAXLINE ) ) <= 0 )
		{
			if( n == 0)
			{
				fputs( "Client termination : Socket read returned 0.\n", stdout );
			}
			else if( n < 0 )
			{
				printf( "Client termination : Socket read returned %d.\n", n );
			}
			return;		/* connection closed by other end */
		}
		n = strlen( line );
		writen( sockfd, line, n );
	}
}

/*********************************************************************
 *
 * Function    : str_time
 *
 * Description : Performs the str_echo service for the server.  
 *
 * Parameters  :
 *          1  : int sockfd = Client connection socket descriptor.
 *	
 * Returns     : VOID
 *
 *********************************************************************/
void str_time( int sockfd )
{
	time_t	ticks;
	char	buff[MAXLINE];
	ssize_t		n;
	
	for ( ; ; ) 
	{
		if( readable_timeo( sockfd, 5 ) == 0 )
		{
		    ticks = time(NULL);
			memset( buff, 0, sizeof( buff ) );
		    snprintf( buff, sizeof( buff ), "%.24s\r\n", ctime( &ticks ) );
		    if( write( sockfd, buff, strlen(buff) ) <= 0 )
		    {
		    	fputs( "Server Error : Unable to write to socket..\n", stdout );
		    }	
		}	
		else
		{
			if ( ( n = readline( sockfd, buff, MAXLINE ) ) <= 0 )
			{
				if( n == 0)
				{
					fputs( "Client termination : Socket read returned 0.\n", stdout );
				}
				else if( n < 0 )
				{
					printf( "Client termination : Socket read returned %d.\n", n );
				}
				return;		/* connection closed by other end */
			}
			else
			{
				fputs("We should not be receiving stuff from the client other than a FIN for time_cli !!\n", stdout);
			}	
		}	
	}
}

/*********************************************************************
 *
 * Function    : doitecho
 *
 * Description : Detaches the thread from the server and calls 
 				 echo service to proceed.    
 *
 * Parameters  :
 *          1  : void *arg = Pointer to client connection socket 
 * 							 descriptor.
 * Returns     : VOID*
 *
 *********************************************************************/
static void * doitecho( void *arg )
{
	int connfd; 

	connfd = *( (int *) arg);
	free( arg );

	fputs("Detaching thread from main server process..\n", stdout);
	pthread_detach( pthread_self() );

	fputs("Running echo client service..\n", stdout);
	str_echo( connfd );	/* same function as before */

	fputs("Closing connection socket..\n", stdout);
	close( connfd );		/* done with connected socket */
	return(NULL);
}

/*********************************************************************
 *
 * Function    : doittime
 *
 * Description : Detaches the thread from the server and calls 
 				 time service to proceed.    
 *
 * Parameters  :
 *          1  : void *arg = Pointer to client connection socket 
 * 							 descriptor.
 * Returns     : VOID*
 *
 *********************************************************************/
static void * doittime( void *arg )
{
	int connfd; 

	connfd = *( (int *) arg);
	free( arg );


	fputs("Detaching thread from main server process..\n", stdout);
	pthread_detach( pthread_self() );
	fputs("Running time client service..\n", stdout);
	str_time( connfd );
	fputs("Closing connection socket..\n", stdout);
	close( connfd );		/* done with connected socket */
	return(NULL);
}


int main(int argc, char **argv)
{
	int					i, maxfd, listenfd1, listenfd2, *iptr, err, nready, fileflags;
	fd_set				rset;
	socklen_t			clilen, len;
	struct sockaddr_in	*cliaddr, servaddr;
	pthread_t			tid;
	char 				*service;
	const int			on = 1;

	/*Creating socket */
	if( ( listenfd1 = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		printf("Unable to create socket. Error code : %dExiting..\n",errno );
		exit(1);
	}	

	/* Making listening socket non-blocking. */
	if ( ( fileflags = fcntl( listenfd1, F_GETFL, 0 ) ) == -1 )  
	{
	   perror( "fcntl F_GETFL" );
	   exit(1);
	}
	if ( fcntl( listenfd1, F_SETFL, fileflags | FNDELAY ) == -1 )  
	{
	   perror( "fcntl F_SETFL, FNDELAY" );
	   exit(1);
	}

	/*Creating socket */
	if( ( listenfd2 = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		printf("Unable to create socket. Error code : %dExiting..\n",errno );
		exit(1);
	}	

	/* Making listening socket non-blocking. */
	if ( ( fileflags = fcntl( listenfd2, F_GETFL, 0 ) ) == -1 )  
	{
	   perror( "fcntl F_GETFL" );
	   exit(1);
	}
	if ( fcntl( listenfd2, F_SETFL, fileflags | FNDELAY ) == -1 )  
	{
	   perror( "fcntl F_SETFL, FNDELAY" );
	   exit(1);
	}

	/* Filling in the socket address structure with server address parameters for time service*/
	bzero( &servaddr, sizeof( servaddr ) );
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
	servaddr.sin_port        = htons( 61617 );

	/* Setting the socket options to address reuse ( SO_REUSEADDR ) */ 
	if( setsockopt( listenfd1, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1 )
	{
		printf("Unable to set socket to SO_REUSEADDR. Error code : %d\nExiting..\n", errno );
	}	
	/* Binding the socket to server address */ 
	if( bind( listenfd1, (SA *) &servaddr, sizeof( servaddr ) ) == -1 )
	{
		printf( "Error binding to socket :: Error code : %d\nExiting..\n",errno );
		exit(1);
	}	

	/* Filling in the socket address structure with server address parameters for echo service*/
	bzero( &servaddr, sizeof( servaddr ) );
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
	servaddr.sin_port        = htons( 61616 );

	/* Setting the socket options to address reuse ( SO_REUSEADDR ) */ 
	if( setsockopt( listenfd2, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 )
	{
		printf("Unable to set socket to SO_REUSEADDR. Error code : %d\nExiting..\n", errno );
	}	

	/* Binding the socket to server address */ 
	if( bind( listenfd2, (SA *) &servaddr, sizeof( servaddr ) ) == -1 )
	{
		printf( "Error binding to socket :: Error code : %d\nExiting..\n",errno );
		exit(1);
	}	

	/* Listening on socket */
	if( listen( listenfd1, LISTENQ ) == -1 )
	{
		printf("Unable to listen on port. Error Code : %d\nExiting..\n", errno);
		exit(1);
	}	

	/* Listening on socket */
	if( listen( listenfd2, LISTENQ ) )
	{
		printf("Unable to listen on port. Error Code : %d\nExiting..\n", errno);
		exit(1);
	}	

	FD_ZERO( &rset );
	maxfd = max( listenfd1, listenfd2 ) + 1;

	cliaddr = malloc( clilen );

	for ( ; ; ) 
	{
		len = clilen;
		iptr = malloc( sizeof( int ) );
		FD_SET( listenfd1, &rset );
		FD_SET( listenfd2, &rset );

		/* I/O multiplexing on 2 listening socket descriptors */
		if( ( nready = select( maxfd, &rset, NULL, NULL, NULL ) ) < 0 )
		{
			if( errno == EINTR )
			{
				fputs("Encountered EINTR.. retrying..\n", stdout);
				continue;
			}	
			else
			{
				fputs("Select call failed..Exiting..\n", stdout);
				exit(1);
			}
		}	

		if ( FD_ISSET( listenfd1, &rset ) || FD_ISSET( listenfd2, &rset )  ) 
		{	
			if( FD_ISSET( listenfd1, &rset ) )
			{
				clilen = sizeof( cliaddr );

				/* Accept client connection for time service */
				if( ( *iptr = accept( listenfd1, (SA *)cliaddr, &len ) ) == -1 )
				{
					printf("Unable to accept client connection :: Error code : %d\nRetrying..\n",errno );
					continue;
				}
				service = "time\n";	
			}				
			else if( FD_ISSET( listenfd2, &rset ) )
			{
				clilen = sizeof( cliaddr );
			
				/* Accept client connection for time service */
				if( ( *iptr = accept( listenfd2, (SA *)cliaddr, &len ) ) == -1 )
				{
					printf("Unable to accept client connection :: Error code : %d\nRetrying..\n",errno );
					continue;
				}					
				service = "echo\n";	
			}	

			/* Make newly created socket blocking */ 
			fileflags &= ~FNDELAY;	
			if ( fcntl( *iptr, F_SETFL, fileflags ) == -1 )  
			{
			   perror( "fcntl F_SETFL, FNDELAY" );
			   exit(1);
			}


			if( strcmp( service,"echo\n" ) == 0 )
			{	
				/* Thread off to Echo service */	
				if( ( err = pthread_create( &tid, NULL, &doitecho, iptr ) ) != 0  )
				{
					printf( "Error detected : Server Thread creation :: Error code : %d\nRetrying..\n",err );
					close( *iptr );
					free( iptr );
					continue;
				}		
			}
			else
			{
				/* Thread off to Time service */	
				if( ( err = pthread_create( &tid, NULL, &doittime, iptr ) ) != 0 ) 
				{
					printf( "Error detected : Server Thread creation :: Error code : %d\nRetrying..\n",err );
					close( *iptr );
					free( iptr );
					continue;
				}	
			}	

		}

	}
}
