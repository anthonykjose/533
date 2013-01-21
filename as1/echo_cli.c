/*********************************************************************
 *
 * File        :  echo_cli.c
 *
 * Purpose     :  Called by tcpechocli as the the client echo 
 * 				  echo service.	
 *  
 *********************************************************************/

#include	"unp.h"
#include <stdio.h>
#include <stdlib.h>

/*********************************************************************
 *
 * Function    :  str_cli1
 *
 * Description :  Performs client side interation with server for 
 *				  for echo service.	
 *
 * Parameters  :
 *          1  :  FILE *fp = Input file descriptor
 *          2  :  int sockfd = client connection socket descriptor
 *			3  :  int status_pipe = Write end of pipe to the client 
 *									parent process.
 * Returns     :  VOID
 *
 *********************************************************************/
void str_cli1( FILE *fp, int sockfd, int status_pipe )
{
	int			maxfdp1, stdineof;
	fd_set		rset;
	char		buf[ MAXLINE ], string[ MAXLINE ];
	int			n, nready;

	stdineof = 0;
	FD_ZERO( &rset );
	for ( ; ; ) 
	{
		if ( stdineof == 0 )
		{	
			FD_SET( fileno( fp ), &rset );
		}	
		FD_SET( sockfd, &rset );
		maxfdp1 = max( fileno( fp ), sockfd ) + 1;
		
		if( ( nready = select( maxfdp1, &rset, NULL, NULL, NULL ) ) < 0 )
		{
			/* Slow system call */
			if( errno == EINTR )
			{
				memset( string, 0, sizeof( string ) );
				strcpy( string, "Slow system call ..retrying..\n" );
				write( status_pipe, string, ( strlen(string)+1 ) );
				continue;
			}	
			else
			{
				memset( string, 0, sizeof( string ) );
				strcpy( string, "Select call failing ..Exiting..\n" );
				write( status_pipe, string, ( strlen(string)+1 ) );
				return;
			}	
		}	

		/* Check if socket descriptor is readable */
		if ( FD_ISSET( sockfd, &rset ) ) 
		{
			memset( string, 0, sizeof( string ) );
			strcpy( string, "ECHO CLIENT : Writing data recieved from socket to stdout..\n" );
			write( status_pipe, string, ( strlen(string)+1 ) );

			if ( ( n = read( sockfd, buf, MAXLINE ) ) == 0 ) 
			{
				if ( stdineof == 1 )
				{
					memset( string, 0, sizeof( string ) );
					strcpy( string, "Server ACK to FIN recieved..Terminating echo service..\n" );
					write( status_pipe, string, ( strlen(string)+1 ) );
					return;
							
				}
				else
				{
					memset( string, 0, sizeof( string ) );
					strcpy( string, "SERVER_NORESP\n" );
					write( status_pipe, string, ( strlen(string)+1 ) );
					return;
				}
			}

			write( fileno(stdout), buf, n );
		}

		if ( FD_ISSET(fileno(fp), &rset) ) 
		{  
			if ( ( n = read( fileno(fp), buf, MAXLINE ) ) == 0) 
			{
				stdineof = 1;
				shutdown( sockfd, SHUT_WR );	
				FD_CLR( fileno(fp), &rset );
				
				memset( string, 0, sizeof( string ) );
				strcpy( string, "Sending FIN to server..\n" );
				write( status_pipe, string, ( strlen(string)+1 ) );

				continue;
			}
		
			memset( string, 0, sizeof( string ) );
			strcpy( string, "ECHO CLIENT : Reading from stdin and writing to socket..\n" );
			write( status_pipe, string, ( strlen(string)+1 ) );
	
			if( ( n = writen( sockfd, buf, n ) ) < 0 )
			{
				memset( string, 0, sizeof( string ) );
				sprintf( string,"Error : ECHO CLIENT : Error in writing to socket : Error No. : %d",errno );
				write( status_pipe, string, ( strlen( string )+1 ) );
			}	
		}
	}
}



int main( int argc, char **argv )
{
	int 				fd[2];
	int					sockfd;
	struct sockaddr_in	servaddr;
	char 				string[MAXLINE];

	/* Take in read and write end pipe fd's for writing statuses back to parent process */
	sscanf( argv[2], "%d %d", &fd[0], &fd[1] );
	
	strcpy( string, "Connecting to port 61616 for echo service..\n" );
	write( fd[1], string, ( strlen(string)+1 ) );
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );

	/* Filling in the socket address structure with server address parameters for time service*/
	bzero( &servaddr, sizeof( servaddr ) );
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons( 61616 );
	inet_pton( AF_INET, argv[1], &servaddr.sin_addr );

	/* Attempting connection with server */
	if( connect( sockfd, (SA *) &servaddr, sizeof( servaddr ) ) == -1 )
	{
		memset( string, 0, sizeof( string ) );
		strcpy( string, "SERVER_UNAVAILABLE\n" );
		write( fd[1], string, ( strlen(string)+1 ) );
		return 1;
	}	

	memset( string, 0, sizeof( string ) );
	strcpy( string, "Connected to server echo service..\n" );
	write( fd[1], string, ( strlen(string)+1 ) );

	/* Calling time client function to handle service */
	str_cli1( stdin, sockfd, fd[1] );		/* do it all */
	close( sockfd );
	return 1;
}
