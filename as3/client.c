#include    "unp.h"
#define HOSTNAME_LEN 255
#define UNIXDG_PATH "testpath"
#include <setjmp.h>
static sigjmp_buf jmpbuf;
static void message_receive_timeout(int signo);

 char server_vm[HOSTNAME_LEN], client_vm[HOSTNAME_LEN];
 int                 sockfd;
 char message_to_be_sent[100] = "Request TIME from server\n";
 char template[100];
 char destination_canonical_ip_presentation_format[100];
char  route_rediscovery_flag[]="0";
int flag=0;

int main(int argc, char **argv)
{
    int                 l;
    socklen_t           len;
    struct sockaddr_un  cliaddr, addr2, servaddr;
    struct hostent *hptr;
    char *ptr, **pptr;
    char message_received[MAXLINE];  
    char source_canonical_ip_presentation_format[100];
    char source_port_number[10];
    int fd;

    char str;
    char  buf[100];
    time_t ticks;
    int fp;
    int result=-1;
    strcpy(template,"/tmp/fileXXXXXX");
    fp=mkstemp(template);
    printf("file pointer from mkstemp :%d\n", fp);
    close(fp);
    unlink(template);
    bzero(&cliaddr, sizeof(cliaddr));      
    cliaddr.sun_family = AF_LOCAL; 
    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0); 
    strcpy(cliaddr.sun_path, template);
    printf("sun path %s :  SUN_LEN(&cliaddr) %d \n",cliaddr.sun_path , SUN_LEN(&cliaddr));   
    bind(sockfd, (SA *) &cliaddr, SUN_LEN(&cliaddr));
    len = sizeof(addr2);
    getsockname(sockfd, (SA *) &addr2, &len);
    printf("bound name = %s, returned len = %d servaddr.sun_path= %s \n", addr2.sun_path, len,template);


    signal(SIGALRM,message_receive_timeout);
    while(1)
    {
        requestClient:
        printf("Please select the server VM : vm1,vm2, ... vm10 :\n");
        scanf("%s",&server_vm);
    
        gethostname( client_vm, sizeof(client_vm) );
 
        
        result=retrieveDestinationCanonicalIpPresentationFormat(server_vm, destination_canonical_ip_presentation_format);      
        if(result==1)
        {
            printf("Client at node %s sending request to server at  %s\n", client_vm, server_vm);

        }
        else
        {
            printf("Server VM not found..\n");
            continue;
        }
        strcpy(route_rediscovery_flag,"0");
        alarm(50);
        //printf("after alarm set\n");
        msg_send( sockfd,  destination_canonical_ip_presentation_format, "80",  message_to_be_sent, route_rediscovery_flag );
        
        
        if(sigsetjmp(jmpbuf,1)!=0)
        {

            if(flag)
            {
                flag=0;
                goto requestClient;

            }
            else
            {
                flag=1;
                goto receive_message;
            }
        }
        receive_message:
        printf("Waiting for response...\n");
        msg_recv( sockfd, message_received, source_canonical_ip_presentation_format, source_port_number);
        flag=0;
        alarm(0);

        ticks=time(NULL);
        memset(buf,0,sizeof(buf));
 
        snprintf(buf,sizeof(buf),"%.24s\r\n",ctime(&ticks));
             printf("Time: %s\n",buf );
        printf("Client at node  %s received from %s at : %s\n", client_vm, server_vm, buf);
    }

    unlink(template);
    exit(0);
}

/*
    Timeout on message recieve at client. Call to re-send request.
*/
static void message_receive_timeout(int signo)
{
     if(flag==0)
     {
      printf("Client at node   %s  : timeout on response from   %s\n", client_vm, server_vm);
      printf("Retransmitting message with Forced Route Discovery\n");
      strcpy(route_rediscovery_flag,"1");
      alarm(50);
      msg_send( sockfd,  destination_canonical_ip_presentation_format, "80",  message_to_be_sent, route_rediscovery_flag );
      
    }else
    {
        printf("Request could not be transmitted from client at %s to %s. Please try again\n",client_vm,server_vm);
    }
      siglongjmp(jmpbuf,1);
        return;
}
