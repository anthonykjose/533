#include	"unp.h"
#include 	<stdio.h>

/*********************************************************************
 *
 * Function    :  time_cli
 *
 * Description :  Performs client side interation with server for 
 *				  for time service.			
 *
 * Parameters  :
 *          1  :  FILE *fp = Input file descriptor
 *          2  :  int sockfd = client connection socket descriptor
 *			3  :  int status_pipe = Write end of pipe to the client 
 									parent process.
 * Returns     :  VOID
 *
 *********************************************************************/
void time_cli( FILE *fp, int sockfd, int status_pipe )
{
	char	recvline[ MAXLINE ];
	int 	err;
	char 	string[ MAXLINE ];

	while ( ( err = readline( sockfd, recvline, MAXLINE ) ) != 0 ) 
	{
		memset( string, 0, sizeof( string ) );
		strcpy( string, "TIME CLIENT : Reading from socket and writing to stdout..\n" );
		write( status_pipe, string, ( strlen(string)+1 ) );

		fputs( recvline, stdout );
		memset( recvline, 0, sizeof( recvline ) );
	}

	sprintf( string, "Error : TIME CLIENT : socket read returned with %d.\nExiting service..", err );
	write( status_pipe, string, ( strlen(string)+1 ) );
	return;	
}

int main( int argc, char **argv )
{
	int 				fd[2];
	int					sockfd;
	struct sockaddr_in	servaddr;
	char 				string[ MAXLINE ];

	/* Take in read and write end pipe fd's for writing statuses back to parent process */
	sscanf( argv[2], "%d %d", &fd[0], &fd[1] );
		
	if ( argc < 2 )
	{	
		err_quit("usage: tcpcli <IPaddress>");
	}

	strcpy( string, "Connecting to port 61617 for time service..\n" );
	write( fd[1], string, ( strlen(string)+1 ) );
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );

	/* Filling in the socket address structure with server address parameters for time service*/
	bzero( &servaddr, sizeof( servaddr ) );
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons( 61617 );
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
	strcpy( string, "Connected to server time service..\n" );
	write( fd[1], string, ( strlen(string)+1 ) );

	/* Calling time client function to handle service */
	time_cli( stdin, sockfd , fd[1] );		
	close( sockfd );
	close( fd[1] );
	return 1;
}
