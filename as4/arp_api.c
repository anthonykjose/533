#include  "unp.h"
#include "hw_addrs.h"
#define UNIXDG_PATH "testpath"

struct hwaddr 
{
    int             sll_ifindex;     /* Interface number */
    unsigned short  sll_hatype;  /* Hardware type */
    unsigned char   sll_halen;       /* Length of address */
    unsigned char   sll_addr[8];     /* Physical layer address */
};
int unix_domain_socket;
int closed =0 ;

static void arp_receive_timeout(int signo)
{
    printf("ARP Request timeout\n");
    closed=1;
    close(unix_domain_socket);
    return;
}
int areq (struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr)
{
    int send_result, replen, n, ihw, khw, len, nready, maxfd;
    struct sockaddr_un unixaddr, repaddr;
    char ip_address[100], str_from_sock[MAXLINE], output_to_sock[MAXLINE], node_ethernet_address[MAXLINE];
    fd_set  rset, allset;
    closed = 0;
    if((unix_domain_socket = socket(AF_UNIX, SOCK_STREAM, 0))==-1)
    {
        printf("Error in creation of Unix Domain socket\n");
        perror("socket");
        return;
        
    }

   
    bzero(&unixaddr, sizeof(unixaddr));
    unixaddr.sun_family = AF_UNIX;
    strcpy(unixaddr.sun_path, UNIXDG_PATH);

    // unlink(UNIXDG_PATH);
  //  printf("sun path: %s\n",unixaddr.sun_path);
       len = strlen(unixaddr.sun_path) + sizeof(unixaddr.sun_family);
    //  printf("IP recieved in areq: %s\n", Sock_ntop_host(IPaddr, sizeof(struct sockaddr)));
    
    strcpy(ip_address,Sock_ntop_host(IPaddr, sizeof(*IPaddr)));

    //printf("bound %d\n",unix_domain_socket);
    if(connect(unix_domain_socket, (struct sockaddr_un *) &unixaddr, len)<0)
    {     perror("connect");
            exit(1);
    }
    //printf("connected\n");


   // printf("1\n");
   // inet_ntop( AF_INET, (IPaddr->sin_addr), ip_address, INET_ADDRSTRLEN );

   // printf("areq() called for IP address: %s\n",ip_address );
    sprintf(output_to_sock,"%s|%d|%hu|%u\n", ip_address, 
                                          HWaddr->sll_ifindex,
                                          HWaddr->sll_hatype,
                                          HWaddr->sll_halen);
     //printf("1\n");
//    printf("%s\n", output_to_sock);

    
   // send_result = sendto(unix_domain_socket,output_to_sock,strlen(output_to_sock),0,&unixaddr,sizeof(unixaddr));
   // if (send_result == -1){ perror("sendto"); exit(0); }

    if (write(unix_domain_socket, output_to_sock, sizeof(output_to_sock)) < 0)
        perror("write");

    printf("\nMessage sent to ARP unix domain socket..%s\n",output_to_sock );
    signal(SIGALRM,arp_receive_timeout);    
    alarm(4);

   // if((n=recvfrom(unix_domain_socket,str_from_sock,MAXLINE,0,&repaddr,&replen))>=0)
    FD_ZERO(&allset);
    FD_ZERO(&rset);
  //FD_SET(connfd, &allset);
    FD_SET(unix_domain_socket, &allset);

  maxfd=unix_domain_socket;
  for ( ; ; ) 
  {
    if(closed == 1)
    {
      return 1;
    }  
    rset = allset;    /* structure assignment */
    if((nready = select(maxfd+1, &rset, NULL, NULL, NULL))<0)
    { 
      if(errno==EINTR)  
        continue;
      else
        err_sys("select error");  
    }   
    if (FD_ISSET(unix_domain_socket, &rset)) 
    { 
    //printf("in areq:readable \n");
        if((n=read(unix_domain_socket,str_from_sock,MAXLINE))>=0)
        {
      //    printf("entering if\n");
            alarm(0);

            if(n == 0 )
            {
              if(closed == 0)
                close(unix_domain_socket);
              return 1; 
            }  

           n= sizeof(str_from_sock);
           str_from_sock[n]=0;
            ihw = IF_HADDR;
            khw=0;
            printf("\nMessage recieved from ARP unix domain socket..%d..%s \n" ,n, str_from_sock);
             memcpy(HWaddr->sll_addr,str_from_sock,IF_HADDR);
            do 
            {   
       
               
                
                printf("%.2x%s", HWaddr->sll_addr[khw] & 0xff, (ihw == 1) ? " " : ":");
                khw++;
            } while (--ihw > 0);
        //   printf("done...\n");
  
       }else{
        perror("read");
       }
    }
  }
   close(unix_domain_socket);

   return -1;



}
