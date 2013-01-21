#include 	"unp.h"
#define HOSTNAME_LEN 255
#define ETH_RUAN537 0x67C6C81
#define UNIX_SERV_PATH "unixservpath"
/*
*******************************************************************************************************************************

*******************************************************************************************************************************
*/

int main()
{
	int					sockfd,len;
	struct sockaddr_un	cliaddr, servaddr,addr2;
    char 				message_received[MAXLINE];  
    char 				client_vm[HOSTNAME_LEN], server_vm[HOSTNAME_LEN];
    char 				source_canonical_ip_presentation_format[100];
    char 				source_port_number[10];
	char 				route_rediscovery_flag[]="0";
	time_t 				ticks;
	char  				buf[100];
	
	sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIX_SERV_PATH);
	unlink(UNIX_SERV_PATH);
	bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
	len = sizeof(addr2);
    getsockname(sockfd, (SA *) &addr2, &len);
    
    printf("Socket is bound to sunpath = %s, Returned len of address = %d \n", addr2.sun_path, len);
	printf("Bind error for host(if any): %s",hstrerror(h_errno));	
	
    printf("%d gethostname done:  %s server_vm\n",gethostname( server_vm, sizeof( server_vm) ), server_vm);    
	while(1)
	{
		printf("Waiting for request...\n");

        msg_recv( sockfd, message_received, source_canonical_ip_presentation_format, source_port_number);

        printf("in loop: msg_recv done %s weqweqweqwewqeqw\n", source_canonical_ip_presentation_format);
		retrieveHostName( source_canonical_ip_presentation_format, client_vm );
		
		ticks=time(NULL);
		memset(buf,0,sizeof(buf));
 
		snprintf(buf,sizeof(buf),"%.24s\r\n",ctime(&ticks));
		printf("*******************************************\n");
		printf("Server at node %s responding to request from  %s\n", server_vm, client_vm );
        printf("Time: %s\n",buf );
        printf("*******************************************\n");
        msg_send(  sockfd,  source_canonical_ip_presentation_format, source_port_number,  buf, route_rediscovery_flag );

	}
	return 0;
}