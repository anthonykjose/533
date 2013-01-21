#include  "unp.h"
#include <netinet/ip.h>
#include    "ping.h"
#include "hw_addrs.h"
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#define ETH_FRAME_LEN 1514
#define IDENTIFIER 17537
#define MAX_PAYLOAD_SIZE 1476
#define USID_PROTO 0xDE  
#define HOSTNAME_LEN 255
#define BUFSIZE 1500
struct hwaddr 
{
    int             sll_ifindex;     /* Interface number */
    unsigned short  sll_hatype;  /* Hardware type */
    unsigned char   sll_halen;       /* Length of address */
    unsigned char   sll_addr[8];     /* Physical layer address */
};
struct proto proto_v4 = { proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP };
int pg_sock, packet_socket, if_index ;
int node_visited=0;
int mcast_timeout=0;
int mcast_port;
char mcast_ip_address[INET_ADDRSTRLEN];
int end_of_tour = 0;
int echo_reply_count = 0;
int visited_on_tour_atleast_once = 0;
int same_predecessor = 0;


nsent=0;

char  source_hw_mac_address[6], destination_hw_mac_address[6], source_ip_address[INET_ADDRSTRLEN], destination_ip_address[INET_ADDRSTRLEN];


struct payload
{
    char IPaddress_list[MAX_PAYLOAD_SIZE];
    int last_visited_index;
};

struct ping_list
{
    char IPaddress[INET_ADDRSTRLEN];
    struct ping_list * next;
}*pl_head,*pl_tmp;


void readloop()
{
    
    //setsockopt(pg_sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
   //sleep(1);
    sig_alrm(SIGALRM);      /* send first packet */

}

void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv)
{

    int             hlen1, icmplen;
    double          rtt;
    struct ip       *ip;
    struct icmp     *icmp;
    struct timeval  *tvsend;
    ip = (struct ip *) ptr;     /* start of IP h590eader */
    printf("check id of packet: %hu\n",ntohs(ip->ip_id));
    
    if(ntohs(ip->ip_id)!=IDENTIFIER)
    {
        printf("not our id\n");
    }else
    {
        hlen1 = ip->ip_hl << 2;     /* length of IP header */
        if (ip->ip_p != IPPROTO_ICMP)
            return;             /* not ICMP */

        icmp = (struct icmp *) (ptr + hlen1);   /* start of ICMP header */
        if ( (icmplen = len - hlen1) < 8)
            return;             /* malformed packet */

        if (icmp->icmp_type == ICMP_ECHOREPLY) {
            if (icmp->icmp_id != pid)
                return;         /* not a response to our ECHO_REQUEST */
            if (icmplen < 16)
                return;         /* not enough data to use */

            tvsend = (struct timeval *) icmp->icmp_data;
            tv_sub(tvrecv, tvsend);
            rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

            printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
                    icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
                    icmp->icmp_seq, ip->ip_ttl, rtt);

        } else if (verbose) {
            printf("  %d bytes from %s: type = %d, code = %d\n",
                    icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
                    icmp->icmp_type, icmp->icmp_code);
        }
    }
}
void recievePacketFromPGSock(int pg_sock)
{
    struct sockaddr pgaddr;
    int pglen;
    char            recvbuf[BUFSIZE];
    char            controlbuf[BUFSIZE];
    struct msghdr   msg;
    struct iovec    iov;
    ssize_t         n;
    struct timeval  tval;
    
    void* buffer = (void*)malloc(BUFSIZE); 
    int             size;
   
    size = 60 * 1024;       /* OK if setsockopt fails */
    memset(buffer,  0,BUFSIZE);
    setsockopt(pg_sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    pglen = sizeof( struct sockaddr );   

    iov.iov_base = recvbuf;
    iov.iov_len = sizeof(recvbuf);
    msg.msg_name = pr->sarecv;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = controlbuf;
    //for ( ; ; ) {
        msg.msg_namelen = pr->salen;
        msg.msg_controllen = sizeof(controlbuf);
        n = recvmsg(pg_sock, &msg, 0);
      /*  if (n < 0) {
           if (errno == EINTR)
                continue;
           else
             err_sys("recvmsg error");
        }*/
        if(n<0)
        {
            perror("recvmsg");
        }
        Gettimeofday(&tval, NULL);
        proc_v4(recvbuf, n, &msg, &tval);
   // }

//    if((n=recvfrom(pg_sock,buffer, BUFSIZE, 0, &pgaddr, &pglen)>0))
  //  {

    //    printf("Recieved %d bytes from whoever..\n",n );
    //} 
    return;   
}

void send_v4(void)
{
    int         len;
    struct icmp *icmp;
    //datalen = 56;
   // icmp = (struct icmp *) sendbuf+34;
    icmp = (struct icmp *)malloc( sizeof(struct icmp) );
    memset(icmp,0,sizeof(struct icmp) );
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = htons(IDENTIFIER);
    icmp->icmp_seq = htons(0);
    icmp->icmp_cksum = htons(0);

    //memset(icmp->icmp_data, 1/*0xa5*/, 1/*datalen*/); /* fill with pattern */
    //Gettimeofday((struct timeval *) icmp->icmp_data, NULL);

    //len = 8 + datalen;      /* checksum ICMP header and data */
    // icmp->icmp_cksum = 0;

   // icmp->icmp_cksum = in_cksum((u_short *) icmp, len);
    printf("icmp packet with id %d prepared for %s \n",ntohs(icmp->icmp_id), destination_ip_address);
    sendPingPacket( packet_socket ,icmp, source_hw_mac_address, destination_hw_mac_address , if_index , source_ip_address, destination_ip_address);

    //recievePacketFromPGSock(pg_sock);
    //Sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
}

void sig_alrm(int signo)
{
    send_v4();

    alarm(5);
    return;
}


/*
  Retrieve the destination canonical IP address in presentation format. 
*/

int retrieveDestinationCanonicalIpPresentationFormat(const char *server_vm, char *destination_canonical_ip_presentation_format)
{
  struct hostent *hptr;
  char *ptr, **pptr;
  if((hptr=gethostbyname(server_vm))==NULL)
  {
    err_msg("gethostbyname error for host: %s : %s",server_vm,hstrerror(h_errno));
    return -1;
  }
  if(hptr->h_addrtype==NULL)
  {
    fprintf(stderr,"Invalid IP address\n");
    return -1;
  }

  switch(hptr->h_addrtype)
  {
    case AF_INET:
    //printf("AF_INET type");

    pptr=hptr->h_addr_list;
    if(pptr!=NULL)
    {
      inet_ntop(hptr->h_addrtype,*pptr,destination_canonical_ip_presentation_format,100);
     // printf("Destination canonical IP in presentation format: %s\n", destination_canonical_ip_presentation_format);
      return 1;
    }
    break;

    default:
    fprintf(stderr,"unknown address type\n");
    return -1;
    break;
  }
}

void retrieveOwnCanonicalIPAddress( char * IPaddress )  
{
    char own_vm_name[HOSTNAME_LEN];   
    gethostname( own_vm_name, sizeof(own_vm_name) );
    retrieveDestinationCanonicalIpPresentationFormat(own_vm_name, IPaddress);      
    return;
}

/*
    Stores the IP addresses of the vm-tour in a comma-separated string to be passed as payload to the tour-members.
*/
int createIPTourString( char * IPaddress_list, char *argv[], char * IPmulticast_address, int port)
{
    int i;
    char IPaddress[INET_ADDRSTRLEN];
    char own_vm_name[HOSTNAME_LEN];
    /* Return the current node's eth0 interface's IP address. */
    gethostname( own_vm_name, sizeof(own_vm_name) );
    retrieveDestinationCanonicalIpPresentationFormat(own_vm_name, IPaddress_list);      

    if( strcmp(argv[1], own_vm_name) == 0 )
    {
        return -1;
    }    
        
    for( i=1; argv[i] != NULL; i++ )
    {
        /* Subsequent nodes should not be same. */
        if( strcmp(argv[i], argv[i-1]) == 0 )
        {
            return -1;
        }     
        retrieveDestinationCanonicalIpPresentationFormat( argv[i], IPaddress );
        strcat(IPaddress_list, "|");
        strcat(IPaddress_list, IPaddress);
    }   
    strcat(IPaddress_list, "|");

    strcat(IPaddress_list, IPmulticast_address);
    strcat(IPaddress_list, ":");
    sprintf( IPaddress_list, "%s%d",IPaddress_list, port );

    return 1;
}

struct payload * createPayload( char * IPaddress_list )
{
    struct payload * p = (struct payload *)malloc( sizeof(struct payload) );
    strcpy(p->IPaddress_list, IPaddress_list);
    p->last_visited_index = htons(0);
    return p;
}

char * retrieveNextTourIpAddress( char * IPaddress_list, int last_visited_index )
{
    int i;
    char * p;
    char IP_iterator[MAX_PAYLOAD_SIZE];
    strcpy(IP_iterator,IPaddress_list);
    p = strtok( IP_iterator,"|" );
    if( last_visited_index == -1 || last_visited_index == 256 )
    {
        return p;
    }    
    for(i=0;i<=last_visited_index;i++)
    {

        p = strtok( NULL,"|" );         
    }    
 
    return p;
}

/*
    Retrieves the Multicast address and port number from the string. 
*/
int  retrieveMulticastIpAddress( char * IPaddress_list, char * multicast_ip, int * port_pointer  )
{
    int i;
    char * p,* prev, * port;
    char IP_iterator[MAX_PAYLOAD_SIZE];

    strcpy(IP_iterator, IPaddress_list);
    p = strtok( IP_iterator,"|" );


    while( p != NULL )
    {
        prev = p;
        p = strtok( NULL,"|" );         
    }    
    p = strtok(prev,":");
    port = atoi( strtok(NULL, ":") );

    strcpy(multicast_ip,prev);
    *port_pointer = port;
    return 1;
}


void sendTourPacket( int sockfd, struct payload * p, char * destination_address, char * source_address )
{
           
    char            sendbuf1[BUFSIZE], str[10];
    size_t          len;
    socklen_t       servlen;
    struct sockaddr_in servaddr;
    struct in_addr ip_src, ip_dst; 
    int send_result=0;
    /* Pointer to beginning of payload */
    unsigned char sendbuf[BUFSIZE];

    unsigned char* iphead = sendbuf;
    unsigned char * data = sendbuf+20; 
    struct iphdr *ip_pkt;

    struct ip *iph = (struct ip *) iphead;  
    memset(sendbuf,0,BUFSIZE);

       /* start of IP header */

    /* Fill out the IP header. */
  ip_pkt = (struct iphdr *) (sendbuf);
  ip_pkt->ihl = 5;              /* Header length / 4.  Always 5 if no opts. */
  ip_pkt->version = 4;          /* IP version.         Always 4.            */
  ip_pkt->tos = 0;              /* Type of service.    8 bits.              */
                                /* Total length.       Bytes, up to 64K.    */
  ip_pkt->tot_len = htons (BUFSIZE);
  ip_pkt->id = 0;               /* Identification.                          */
  ip_pkt->frag_off = 0;         /* Fragment offset.                         */
  ip_pkt->ttl = 64;             /* Time to live.       8 bits.              */
  ip_pkt->protocol = USID_PROTO; /* Protocol.         IPPROTO_xxx          */
                                /* Source address.     IP address.          */
    
  ip_pkt->saddr=  inet_addr(source_address);
  ip_pkt->daddr=  inet_addr(destination_address);

  /* Compute the IP header checksum. */
  ip_pkt->check = 0;
 // ip_pkt->check = ip_fast_csum ((unsigned char *) ip_pkt, ip_pkt->ihl);
   /* 
    iph->ip_v=4;
    iph->ip_hl=20;
  

   iph->ip_tos=16;           
   iph->ip_len=htons(BUFSIZE);         
    iph->ip_id = 1;      
    iph->ip_off=0;         

   iph->ip_ttl=64;            
    iph->ip_p = USID_PROTO;       
    iph->ip_sum = 0;   
    */   
     /* source and dest address */

   // iph->ip_id = htons(IDENTIFIER);

    
   // inet_aton(source_address, &ip_src);
    //inet_aton(destination_address, &ip_dst);
    //memcpy((void *)&iph->ip_src,(void *)&ip_src,sizeof(struct in_addr));
    //memcpy((void *)&iph->ip_dst,(void *)&ip_dst,sizeof(struct in_addr));
   // inet_pton(AF_INET, destination_address, (struct in_addr *)&daddr.sin_addr.s_addr);
   // printf("size of payload: %d\n", sizeof(struct payload));        
   //  printf("size of payload: %s\n", p->IPaddress_list);       
    memcpy( (void *)data,(void *)p,sizeof(struct payload) ); 
   // len = sizeof(*sendbuf);

    bzero( &servaddr, sizeof( servaddr ) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons( 0 );

    inet_pton( AF_INET, destination_address,(struct in_addr *) &servaddr.sin_addr);


 //   printf("IP: %s",inet_ntop(AF_INET,&servaddr.sin_addr,str,100));
    servlen = sizeof(servaddr);

    printf("SENDING tour packet to %s from source : %s \n", destination_address, source_address);
    send_result = sendto(sockfd, sendbuf, BUFSIZE, 0, &servaddr, servlen);
    if (send_result == -1){ perror("sendto"); }

}


/*
    
The definition of struct ip_mreq is as follows:

    struct ip_mreq 
    {
        struct in_addr imr_multiaddr; 
        struct in_addr imr_interface; 
    }
*/
int joinMulticastGroup1( int sock, char * multicast_ip_address, char * joining_local_interface_ip_address )
{
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr =  inet_addr( multicast_ip_address );
    mreq.imr_interface.s_addr = inet_addr( joining_local_interface_ip_address );
 
    if( setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) == -1 )
    {
        printf("Unable to join multicast group.. Error code : %d\n", errno );
        return  0;
    }       

    return 1;
}

void sendPingPacket( int s , struct icmp * populated_icmp_frame , char * source_hw_mac_address, char * destination_hw_mac_address , int if_index ,char * source_ip_address, char * destination_ip_address)
{
    
    int j,i,len;
    /*target address*/
    struct sockaddr_ll socket_address;

    /*buffer for ethernet frame*/
    void* buffer = (void*)malloc(ETH_FRAME_LEN);
     
    /*pointer to ethenet header*/
    unsigned char* etherhead = buffer;

    /*pointer to ethenet header*/
    unsigned char* iphead = buffer + 14;
        
    /*pointer to userdata in ethernet frame*/
    unsigned char* icmphead = buffer + 14 + sizeof(struct iphdr);
    struct iphdr *ip_pkt;    
    /*another pointer to ethernet header*/
    struct ethhdr *eh = (struct ethhdr *)etherhead;
     unsigned char* datahead= buffer + 14 + sizeof(struct iphdr) + sizeof(struct icmp );
/*another pointer to ethernet header*/
  //  struct ip *iph = (struct ip *)iphead;
    

    int send_result = 0,k=0;

    /*our MAC address*/
    unsigned char src_mac[6], dest_mac[6]; 

    char * src_mac1=source_hw_mac_address; 
    char * dest_mac1=destination_hw_mac_address; 
    char data[4];
    int dataln=4;
    dataln=4;
    data[0]='r';
    data[1]='r';
    data[2]='r';
    data[3]='r';


//  unsigned char src_mac[6] = {0x00, 0x0c, 0x29, 0x11, 0x58, 0xa2};
    
    /*Broadcast MAC address*/
    //unsigned char dest_mac[6] = {0x00, 0x0c, 0x29, 0x24, 0x8f, 0x70};
    //unsigned char dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memset(buffer,  0,ETH_FRAME_LEN);
    i = IF_HADDR;
 printf("source: %s , destination:%s \n",source_ip_address ,destination_ip_address);
        printf("\n\nSending Ping Packet to  \n\t Destination H/W Address :\n");
    do 
    {   
        dest_mac[k] = *destination_hw_mac_address++ & 0xff;
        k++;
        printf("%.2x%s", *dest_mac1++ & 0xff, (i == 1) ? " " : ":");
    } while (--i > 0);
    printf("\n");
    

    printf("\tsending frame on socket: %d\n",s );
    //printf("\tpopulated_odr_frame: %d bytes.\n",sizeof(*populated_odr_frame));
    /*prepare sockaddr_ll*/
   
    printf("\t Source H/W Address :\n");
    i = IF_HADDR;
    k=0;
    do 
    {   
        src_mac[k] = *source_hw_mac_address++ & 0xff;
        k++;
        printf("%.2x%s", *src_mac1++ & 0xff, (i == 1) ? " " : ":");
    } while (--i > 0);


    /*RAW communication*/
    socket_address.sll_family   = PF_PACKET;    
    /*we don't use a protocoll above ethernet layer
      ->just use anything here*/
    socket_address.sll_protocol = htons(ETH_P_IP);  

    /*ARP hardware identifier is ethernet*/
    socket_address.sll_hatype   = ARPHRD_ETHER;
        
    /*target is another host*/
    socket_address.sll_pkttype  = PACKET_OTHERHOST;

    /*index of the network device
    see full code later how to retrieve it*/
    socket_address.sll_ifindex  = if_index;

    /*address length*/
    socket_address.sll_halen    = ETH_ALEN;     

    printf("Before socket_address..\n");    
    /*MAC - begin*/
    socket_address.sll_addr[0]  = dest_mac[0];      
    socket_address.sll_addr[1]  = dest_mac[1];      
    socket_address.sll_addr[2]  = dest_mac[2];
    socket_address.sll_addr[3]  = dest_mac[3];
    socket_address.sll_addr[4]  = dest_mac[4];
    socket_address.sll_addr[5]  = dest_mac[5];
    /*MAC - end*/
    socket_address.sll_addr[6]  = 0x00;/*not used*/
    socket_address.sll_addr[7]  = 0x00;/*not used*/

    /*set the frame header*/
    memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
    memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
    eh->h_proto = htons(ETH_P_IP);



    /* Fill out the IP header. */
      ip_pkt = (struct iphdr *) (iphead);
      ip_pkt->ihl = 5;              /* Header length / 4.  Always 5 if no opts. */
      ip_pkt->version = 4;          /* IP version.         Always 4.            */
      ip_pkt->tos = 0;              /* Type of service.    8 bits.              */
                                    /* Total length.       Bytes, up to 64K.    */
      ip_pkt->tot_len = htons ( sizeof(struct iphdr)+ sizeof(struct icmp) + dataln);
      ip_pkt->id = htons(IDENTIFIER);               /* Identification.                          */
      ip_pkt->frag_off = 0;         /* Fragment offset.                         */
      ip_pkt->ttl = 64;             /* Time to live.       8 bits.              */
      ip_pkt->protocol = IPPROTO_ICMP; /* Protocol.         IPPROTO_xxx          */
                                    /* Source address.     IP address.          */
        
      ip_pkt->saddr=  inet_addr(source_ip_address);
      ip_pkt->daddr=  inet_addr(destination_ip_address);

      /* Compute the IP header checksum. */
      ip_pkt->check = htons(0);

    /*fill the frame with some datip_src,a*/
     datalen = 56;
     len = 14 + sizeof(struct iphdr) + sizeof(struct icmp) + dataln;

    memcpy((void*)icmphead,(void*)populated_icmp_frame, sizeof(struct icmp) );
    memcpy((void*)datahead,(void*)data,dataln);

    printf("Just before send..identifier: %hu ,size of buffer:%d \n", ntohs(ip_pkt->id ), sizeof(*buffer));
    /*send the packet*/
    send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, 
              (struct sockaddr*)&socket_address, sizeof(socket_address));
    if (send_result == -1){ perror("sendto"); }
}

/*
  Retrieve the hostname using IP address.
*/
void retrieveHostName( const char *address , char * h_name)
{
  struct hostent     *hptr;
  in_addr_t           ipAdd;

  ipAdd =  inet_addr( address );

  if( (hptr = gethostbyaddr( (char*) &ipAdd, sizeof( ipAdd ), AF_INET )) == NULL )
  {
    printf("Address is invalid..\n");
    err_msg("gethostbyaddr error for host: %s: %s",
              address, hstrerror(h_errno));
    return;
  }
   h_name = hptr->h_name;
  printf("Client Hostname : `%s`\n",h_name);
 
 } 

void updateLastVisitedIndex()
{

}

/*
    Process the recieved packet. Convert to HBO and return the packet.
*/
struct payload * preprocessPacket(void * str_from_sock)
{
    //unsigned char sendbuf[BUFSIZE];
    void* buffer = (void*)malloc(BUFSIZE);    /*buffer for ip frame*/

    char str[MAX_PAYLOAD_SIZE];
    int n;
    unsigned char* iphead = buffer;
    unsigned char * data = buffer+sizeof(struct iphdr); 
    struct payload * pyld;  

    struct iphdr *ip_pkt;

    struct iphdr *iph = (struct iphdr *) iphead;  
    memset(buffer,  0,BUFSIZE);

    memcpy((void*)buffer, (void*)str_from_sock, BUFSIZE ); 
    pyld = (struct payload *) data;

//    payload = (char *)data;               

    n=strlen(pyld->IPaddress_list);
    strcpy(str,(char *)pyld->IPaddress_list);
    pyld->last_visited_index = ntohs( pyld->last_visited_index );
    printf("Recieved payload : %s\n",str);

    return pyld;


/*
    struct odr_frame * recieved_odr_frame;
    int j;
    struct sockaddr_ll socket_address;  

    void* buffer = (void*)malloc(ETH_FRAME_LEN);    
    unsigned char* etherhead = buffer; 
    void * data = buffer + 34;
    
    printf("\nBeginning processing of recieved packet..\n");
        
    printf("1\n");
        memcpy((void*)buffer, (void*)str_from_sock, ETH_FRAME_LEN ); 
        printf("2\n");
    recieved_odr_frame = (struct odr_frame *)data;
    printf("3\n");

    printf("\t- Converting to Host Byte Order..\n");
    recieved_odr_frame = convertToHostByteOrder( recieved_odr_frame );

    printf("Done processing of recieved packet..\n");
    return recieved_odr_frame;
*/

}

char * retrievePredecessorNodeIPaddress( struct payload * processed_received_payload )
{
    /* The predecessor can be obtained by returning the 'NextTourIPaddress using last_visited_index -1 .. wooo' */
    char IPaddress[INET_ADDRSTRLEN];
    int last_visited_index =  processed_received_payload->last_visited_index;

    strcpy(IPaddress ,retrieveNextTourIpAddress( processed_received_payload->IPaddress_list, last_visited_index -2 ));
    return IPaddress;
}

void handleMulticastjoining( int mcast_udp_sock, struct payload * processed_received_payload )
{
    struct sockaddr_in servaddr;
    retrieveMulticastIpAddress( processed_received_payload->IPaddress_list, mcast_ip_address, &mcast_port );
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(mcast_port);

    bind(mcast_udp_sock, (SA *) &servaddr, sizeof(servaddr));
    joinMulticastGroup( mcast_udp_sock, mcast_ip_address );

}
/*
    Retrieve Mac address from interface index.
*/
unsigned char * retrieveMacFromInterfaceIndex( int interface_index )
{
    unsigned char source_mac[6];
    struct hwa_info *hwa, *hwahead;
    struct sockaddr_in * ip_address_structure;
    int k,i;
    char   *ptr,*ptr1;
    
    /* Flood with broadcast address on all interfaces except eth0 and lo and recieved interface */
    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
    {
        if( hwa->if_index == interface_index )
        {   

            ptr = hwa->if_haddr;
            ptr1 = hwa->if_haddr;

            i = IF_HADDR;
            k=0;
            do 
            {   
                source_mac[k] = *ptr++ & 0xff;
                k++;
                printf("%.2x%s", *ptr1++ & 0xff, (i == 1) ? " " : ":");
            } while (--i > 0);

        
        }
    }

    return source_mac;
}

int retrieveInterfaceIndexFromIP( char * IPaddress )
{
    struct hwa_info *hwa, *hwahead;
    struct sockaddr_in * ip_address_structure;
    int k,i;
    char   *ptr,*ptr1;
    char * ifname_split;

    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
    {
        ifname_split = strtok(hwa->if_name, ":"); //get pointer to first token found and store in 0
    
        if( strcmp(ifname_split, "eth0")==0)
        {   
            return hwa->if_index;
        }
    } 
    return -1;  
}


void ping_list_insert( char * IPaddress )
{

    struct ping_list *node = (struct ping_list *)malloc( sizeof(struct ping_list) );

    strcpy( node->IPaddress, IPaddress );

    if( pl_head == NULL )
    {
      pl_head = node;
      pl_head->next = NULL;         
    } 
    else if( pl_head->next == NULL )
    {
      pl_head->next = node;
      node->next = NULL;            
    } 
    else
    {
      pl_tmp = pl_head->next;       
      pl_head->next = node;
      node->next = pl_tmp;            
    } 
    return;
 }


struct ping_list * ping_list_lookup( char * IPaddress )
{
    struct ping_list *node;     

    node = pl_head;
    while( node != NULL )
    {
        if( strcmp(node->IPaddress, IPaddress) == 0 )
        {
            return node;
        }   
        node = node->next;
    }
    return NULL;    
}


void recievePacketFromRTSock(int rt_sock, int mcast_udp_sock,char * predecessorIPaddress,struct hwaddr * HWaddr)
{
    struct sockaddr_ll rtaddr;
    char source_address[INET_ADDRSTRLEN], hostname[HOSTNAME_LEN], destination_address[INET_ADDRSTRLEN];
    int rtlen, n,ihw,khw;
    time_t ticks;
    struct sockaddr_in pgaddr;
    int pglen;        
    
    void* buffer = (void*)malloc(BUFSIZE); 

    struct payload * processed_recieved_payload; 


    rtlen = sizeof( struct sockaddr );   
    if((n=recvfrom(rt_sock,buffer, BUFSIZE, 0, &rtaddr, &rtlen)>0))
    {
        printf("RECIEVED A TOUR PACKET.\n");
        
         memcpy(source_hw_mac_address,retrieveMacFromInterfaceIndex( retrieveInterfaceIndexFromIP(source_ip_address)),6);
        processed_recieved_payload = preprocessPacket(buffer);
        
        /* UPDATE LAST VISITED INDEX */
        processed_recieved_payload->last_visited_index += 1;
        
        if(visited_on_tour_atleast_once == 0)
        {   
            handleMulticastjoining(mcast_udp_sock, processed_recieved_payload);
            visited_on_tour_atleast_once = 1;
        }
        

        if( checkIfEndOfTour( processed_recieved_payload->IPaddress_list, processed_recieved_payload->last_visited_index ) == 1 )
        {
            printf("We have reached the end of the tour.. Time to start the multicast (after 5 pings are done)..\n");
            end_of_tour = 1; 
        }    
        else
        {
            printf("New Updated last_visited_index - %d\n",processed_recieved_payload->last_visited_index );
         
            retrieveOwnCanonicalIPAddress( source_address );
            retrieveOwnCanonicalIPAddress( source_ip_address);

            
            strcpy( destination_address,retrieveNextTourIpAddress(processed_recieved_payload->IPaddress_list,processed_recieved_payload->last_visited_index));
            printf("Next IP on route is : %s\n",destination_address );

            processed_recieved_payload->last_visited_index = htons(processed_recieved_payload->last_visited_index);
            sendTourPacket( rt_sock, processed_recieved_payload, destination_address,source_address );

        }    
        
        if( end_of_tour == 0 )
            processed_recieved_payload->last_visited_index = ntohs(processed_recieved_payload->last_visited_index);

        strcpy(predecessorIPaddress,retrievePredecessorNodeIPaddress( processed_recieved_payload ));
        printf("Predecessor IP (Must ping to this address): %s \n",predecessorIPaddress );    
       
        if( ping_list_lookup(predecessorIPaddress) != NULL)
        {
            printf("Node was visited before by this predecessor.Must avoid pings. \n");
            same_predecessor=1;                
        }    
        else
        {
            ping_list_insert( predecessorIPaddress );
        }    

        if( same_predecessor == 0 )
        {
            bzero( &pgaddr, sizeof( pgaddr ) );
            pgaddr.sin_family = AF_INET;
            pgaddr.sin_port = htons( 0 );

            inet_pton( AF_INET, predecessorIPaddress,(struct in_addr *) &pgaddr.sin_addr);

            pglen = sizeof(pgaddr);

            HWaddr->sll_ifindex = 1;
            if_index=retrieveInterfaceIndexFromIP(source_ip_address);
            HWaddr->sll_hatype = ARPHRD_ETHER;
            HWaddr->sll_halen = 6;

            if( areq ((struct sockaddr *)&pgaddr, pglen, HWaddr) == -1)
            {
                same_predecessor = 1;
              //  exit(0);
                return;
            }    
          

                ihw = IF_HADDR;
                khw=0;
            
                do 
                {  
                    
                    printf("%.2x%s", HWaddr->sll_addr[khw] & 0xff, (ihw == 1) ? " " : ":");
                    khw++;
                } while (--ihw > 0);
               
        } 
    } 
    return;   
}


void recieveMulticastMessage( int sockfd )
{
    int n;
    char own_vm_name[HOSTNAME_LEN];   
    char recvline[MAXLINE];

    memset(recvline,0,MAXLINE);
    if( n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL) > 0 )
    {
       // recvline[n] = 0;    /* null terminate */
       
        gethostname( own_vm_name, sizeof(own_vm_name) );
        printf("Node %s .  Recieved: %s.\n",own_vm_name,recvline);
    }    
    return;
}

void sendMulticastMessage( int sockfd, char * sendline )
{
    int n,send_result;
    socklen_t maddrlen;
    struct sockaddr_in  maddr;
    char own_vm_name[HOSTNAME_LEN];   
    char mcast_ip_address[INET_ADDRSTRLEN]; 
    int mcast_port = 17537;
    strcpy(mcast_ip_address,"239.108.175.37");

    bzero(&maddr, sizeof(maddr));
    maddr.sin_family = AF_INET;
    maddr.sin_port = htons(mcast_port);
    inet_pton(AF_INET, mcast_ip_address, &maddr.sin_addr);
    maddrlen = sizeof(maddr);

    gethostname( own_vm_name, sizeof(own_vm_name) );
    printf("\n\nNode %s .  Sending: %s.\n",own_vm_name,sendline);
   
    send_result = sendto(sockfd, sendline, MAXLINE, 0, (SA *) &maddr, maddrlen);
    if (send_result == -1){ perror("sendto"); }
    return;
}

static void multicast_receive_timeout(int signo)
{
    printf("Multicast recieve timeout..\n");
    mcast_timeout = 1;   
}


void recieveFirstMulticastMessage(int sockfd)
{
    char sendbuf[MAXLINE];
    char own_vm_name[HOSTNAME_LEN];   
    recieveMulticastMessage( sockfd );   
    //       stop pinging activity 
    
    gethostname( own_vm_name, sizeof(own_vm_name) );
    sprintf(sendbuf,"<<<<< Node %s .  I am a member of the group. >>>>>",own_vm_name);
    sendMulticastMessage(sockfd, sendbuf );

    signal(SIGALRM,multicast_receive_timeout);    
    alarm(5);

//    put timeout of 5 seconds on subsequent recieves.
    while(1)
    {
        alarm(5);
        recieveMulticastMessage(sockfd);
        if (mcast_timeout == 1)
            return;
        alarm(0);
    }        
}

void joinMulticastGroup( int mcast_udp_sock, char * mcast_address_string )
{
    struct sockaddr_in  servaddr, grpaddr, cliaddr;
    int return_value;
    
    bzero(&grpaddr, sizeof(grpaddr));
    grpaddr.sin_family      = AF_INET;
    grpaddr.sin_addr.s_addr = inet_addr(mcast_address_string);

    printf("Joining multicast group..\n");
    printf("Multicast address : %s\n", mcast_address_string);
    
    if(return_value = mcast_join(mcast_udp_sock, &grpaddr, sizeof(grpaddr), NULL, 0) == -1 )
    {
        printf("Error in joining the multicast group..Exiting..\n");
        exit(1);
    }    
    else
    {
        printf("Successfully joined the multicast group..\n");
    }    
    return;    
}

int initiateMulticastSending( int sockfd )
{
    char sendline[MAXLINE];
    char own_vm_name[HOSTNAME_LEN];   
    
    gethostname( own_vm_name, sizeof(own_vm_name) );
    
    sprintf(sendline,"<<<<< This is node %s. Tour has ended. Group members please identify yourselves. >>>>>",own_vm_name);

    sendMulticastMessage(sockfd, sendline);
    return 1;
}

int checkIfEndOfTour(char * IPaddress_list, int last_visited_index)
{
    char multicast_address[INET_ADDRSTRLEN];
    char multicast_address_port_both[MAX_PAYLOAD_SIZE];
    int port_number;
    retrieveMulticastIpAddress( IPaddress_list , multicast_address, &port_number) ;
    sprintf( multicast_address_port_both, "%s:%d",multicast_address, port_number );

    if ( strcmp(retrieveNextTourIpAddress(IPaddress_list, last_visited_index), multicast_address_port_both) == 0 )
    {
        return 1;
    }
    return 0;
}

void stopPingRoutine()
{
    printf("Stopping Ping routine..\n");
    return;
}


int main(int argc, char const *argv[])
{
    int   rt_sock,  iptour_return, maxfd;
    const int   on = 1;
    char IPaddress_list[MAX_PAYLOAD_SIZE], IPmulticast_address[INET_ADDRSTRLEN] = "239.108.175.37", host[INET_ADDRSTRLEN];
    char source_address[INET_ADDRSTRLEN];
    int port_number = 17537;
    fd_set          rset,allset;
    int             c, nready;
    struct addrinfo *ai;
    char *h;
    //int datalen = 56;
    struct payload * p;
    char * destination_address;
    int                 mcast_udp_sock;
    struct sockaddr_in  servaddr, grpaddr, cliaddr;
     char predecessorIPaddress[INET_ADDRSTRLEN];
     struct hwaddr *HWaddr;
     retrieveOwnCanonicalIPAddress( source_ip_address);
    mcast_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if((packet_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP) ) )==-1)
    {
        printf("Error in creation of socket for PF_PACKET\n");
        perror("socket");
        return 0;
    }

 
    if((rt_sock = socket(AF_INET, SOCK_RAW, USID_PROTO))==-1)
    {
        printf("Error in creation of IP raw socket.rt_sock\n");
        perror("socket");
        return;
        
    }
    if((pg_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP))==-1)
    {
        printf("Error in creation of IP raw socket.pg_sock\n");
        perror("socket");
        return;
        
    }

    /* Setting the socket options to IP_HDRINCL */ 
    if( setsockopt( rt_sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1 )
    {
        printf("Unable to set socket to SO_REUSEADDR. Error code : %d\nExiting..\n", errno );
    }       
    
    if( argc > 1 )
    {    
        mcast_port = 17537;
        strcpy( mcast_ip_address, "239.108.175.37");
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port        = htons(mcast_port);

        bind(mcast_udp_sock, (SA *) &servaddr, sizeof(servaddr));
        joinMulticastGroup( mcast_udp_sock, mcast_ip_address );


        if( createIPTourString(IPaddress_list, argv, IPmulticast_address, port_number) == -1 )
        {
            printf("Error : Same node should not appear consequentively in the tour list..Exiting..\n");
            exit(1);
        }    
        else
            printf("\nIP Tour List : %s\n",IPaddress_list);
        
        p = createPayload( IPaddress_list );
        retrieveOwnCanonicalIPAddress( source_address );
        retrieveOwnCanonicalIPAddress( source_ip_address);
        destination_address = retrieveNextTourIpAddress(IPaddress_list,0);
        sendTourPacket( rt_sock, p, destination_address,source_address );

    }    

    printf("starting select procedure \n");
    FD_ZERO(&allset);
    FD_ZERO(&rset);
    FD_SET( rt_sock, &allset );
    FD_SET( mcast_udp_sock, &allset );
   // FD_ZERO( &rset );
    maxfd = max( mcast_udp_sock, rt_sock ) +1;
   // maxfd = max( maxfd, pg_sock ) + 1;

                
    printf("pg_sock %d, rt_sock %d, mcast_udp_sock : %d\n",pg_sock, rt_sock,mcast_udp_sock);
    for ( ; ; ) 
    {
            rset = allset;
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
 
            if( FD_ISSET(rt_sock, &rset) )
            {
                printf("Recieving packet from rt_sock..\n");
                   

                HWaddr = (struct HWaddr *)malloc(sizeof(struct hwaddr));
                recievePacketFromRTSock(rt_sock,mcast_udp_sock,destination_ip_address,HWaddr);
                
                if( same_predecessor == 0 )
                {
                    memcpy(destination_hw_mac_address,HWaddr->sll_addr,IF_HADDR);
                    pid = IDENTIFIER;  /* ICMP ID field is 16 bits */
                    Signal(SIGALRM, sig_alrm);
                    ai = Host_serv(destination_ip_address, NULL, 0, 0);

                    h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);
                        /* 4initialize according to protocol */
                    if (ai->ai_family == AF_INET) 
                    {
                        pr = &proto_v4;

                    } 
                    else
                        err_quit("unknown address family %d", ai->ai_family);

                    pr->sasend = ai->ai_addr;
                    pr->sarecv = Calloc(1, ai->ai_addrlen);
                    pr->salen = ai->ai_addrlen;
                    pr->icmpproto = IPPROTO_ICMP;

                    printf("PING %s (%s): %d data bytes\n",
                            ai->ai_canonname ? ai->ai_canonname : h,
                            h, datalen);

                        /* 4initialize according to protocol */

                    //readloop();
                }    
                same_predecessor=0;
                echo_reply_count = 5;
                 if( (end_of_tour == 1) && (echo_reply_count >= 5)  )
                 {
                      initiateMulticastSending(mcast_udp_sock);
                 }
     

            }   
//            else if( FD_ISSET(pg_sock, &rset) )
  //          {
    //            pr intf("Recieving packet from pg_sock..\n");
                //recievePacketFromPGSock(pg_sock);
      //      } 
            else if( FD_ISSET(mcast_udp_sock, &rset) )
            {
                printf("Recieving packet from mcast_udp_sock..\n");
                recieveFirstMulticastMessage(mcast_udp_sock);
                printf("Terminating the Tour Process..\n");
                
                return 1;
            } 
    }

    //pinging**************************
    



    




    //pinging**************************

}

/*
Tour application module specifications

    The application will create a total of four sockets: two IP raw sockets, a PF_PACKET socket and a UDP socket for multicasting.

        We shall call the two IP raw sockets the ‘rt ’ (‘route traversal’) and ‘pg ’ (‘ping’) sockets, respectively. 
        The rt socket should have the IP_HDRINCL option set. You will only be receiving ICMP echo reply messages 
        through the pg socket (and not sending echo requests), so it does not matter whether it has the IP_HDRINCL option set or not.

        The pg socket should have protocol value (i.e., protocol demultiplexing key in the IP header) IPPROTO_ICMP.

        The rt socket should have a protocol value that identifies the application - i.e., some value other than
        the IPPROTO_XXXX values in  /usr/include/netinet/in.h. However, remember that you will all be running your
        code using the same root account on the vm1 , . . . . . , vm10 nodes. So if two of you happen to choose 
        the same protocol value and happen to be running on the same vm node at the same time, your applications
        will receive each other’s IP packets. For that reason, try to choose a protocol value for your rt socket
        that is likely to be unique to yourself.

        The PF_PACKET socket should be of type SOCK_RAW (not SOCK_DGRAM). This socket should have a protocol value
        of  ETH_P_IP = 0x0800 (IPv4).

        The UDP socket for multicasting will be discussed below. Note that, depending on how you choose to bind that
        socket, you might actually need to have two UDP sockets for multicast communication – see bottom of  p. 576, Section 21.10.

    Your application will, of course, have to be running on every vm node that is included in the tour.

        When evoking the application on the source node, the user supplies a sequence of vm node names 
        (not IP addresses) to be visited in order. This command line sequence starts with the next node
        to be visited from the source node (i.e., it does not start with the source node itself). 
        The sequence can include any number of repeated visits to the same node. For example, suppose 
        that the source node is vm3 and the executable is called badr_tour :

        [root@vm3/root]# badr_tour vm2 vm10 vm4 vm7 vm5 vm2 vm6 vm2 vm9 vm4 vm7 vm2 vm6 vm5 vm1 vm10 vm8

        (but note that the tour does not necessarily have to visit every vm node; and the same node should not
        appear consequentively in the tour list – i.e., the next node on the tour cannot be the current node itself).

        The application turns the sequence into a list of IP addresses for source routing. It also adds the IP 
        address of the source node itself to the beginning of the list. The list thus produced will be carried 
        as the payload of an IP packet, not as a SSRR option in the packet header. It is our application which
        will ensure that every node in the sequence is visited in order, not the IP SSRR capability.

        The source node should also add to the list an IP multicast address and a port number of its choice.
        It should also join the multicast group at that address and port number on its UDP socket. 
        The TTL for outgoing multicasts should be set to 1.

        The application then fills in the header of an IP packet, designating itself as the IP source, and the next node to be visited as the IP destination. The packet is sent out on the rt socket. Note that on Linux, all the fields of the packet header must be in network byte order (Stevens, Section 28.3, p. 737, the fourth bullet point).

        When filling in the packet header, you should explicitly fill in the identification field (recall that, with the IP_HDRINCL socket option, if the identification field is given value 0, then the kernel will set its value). Try to make sure that the value you choose is likely to be unique to yourself (for reasons similar to those explained with respect to the IPPROTO_XXXX in 1. above).

    When a node receives an IP packet on its rt socket, it should first check that the identification field carries the right value (this implies that you will hard code your choice of identification field value determined in item 2 above in your code). If the identification field value does not check out, the packet is ignored. For a valid packet :

        Print out a message along the lines of:

                <time>   received source routing packet from <hostname>

        <time> is the current time in human-readable format (see lines 19 & 20 in Figure 1.9,  p. 14, and the corresponding explanation on  p. 14f.), and <hostname> is the host name corresponding to the source IP address in the header of the received packet.

        If this is the first time the node is visited, the application should use the multicast address and port number in the packet received to join the multicast group on its UDP socket. The TTL for outgoing multicasts should be set to 1.

        The application updates the list in the payload, so that the next node in the tour can easily identify what the next hop from itself will be when it receives the packet. How you do this I leave up to you. You could, for example, include as part of the payload a pointer field into the list of nodes to be visited. This pointer would then be updated to the next entry in the list as the packet progresses hop by hop (see Figure 27.1 and the associated explanation on  pp. 711-712). Other solutions are, of course, possible. The application then fills in a new IP header, designating itself as the IP source, and the next node to be visited as the IP destination. The identification field should be set to the same value as in the received packet. The packet is sent out on the rt socket.

        The node should also initiate pinging to the preceding node in the tour (the IP address of which it should pick up from the header of the received packet). However, unlike the Stevens ping code, it will be using the SOCK_RAW-type PF_PACKET socket of item 1 above to send the ICMP echo request messages.

        Before it can send echo request messages, the application has to call on the ARP module you will implement to get the Ethernet address of this preceding / ‘target’ node; this call is made using the API function areq which you will also implement (see sections ARP module specifications & API specifications below). Note that ARP has to be evoked every time the application wants to send out an echo request message, and not just the first time.

        An echo request message has to be encapsulated in a properly-formulated IP packet, which is in turn encapsulated in a properly-formulated Ethernet frame transmitted out through the PF_PACKET socket ;  otherwise, ICMP at the source node will not receive it. You will have to modify Stevens’ ping code accordingly, specifically, the send_v4 function. In particular, the Ethernet frame must have a value of  ETH_P_IP = 0x0800 (IPv4 – see <linux/if_ether.h>) in the frame type / ‘length’ field ;  and the encapsulated IP packet must have a value of  IPPROTO_ICMP = 0x01 (ICMPv4 – see <netinet_in.h>) in its protocol field.

        You should also simplify the ping code in its entirety by stripping all the ‘indirection’ IPv4 / IPv6 dual-operability paraphernalia and making the code work just for IPv4. Also note that the functions host_serv and freeaddrinfo, together with the associated structure addrinfo (see Sections 11.6, 11.8 & 11.11), in Figures 27.3, 27.6 & 28.5 ( pp. 713, 716 & 744f., respectively) can be replaced by the function gethostbyname and associated structure hostent (see Section 11.3) where needed. Also, there is no ‘-v’ verbose option, so this too should be stripped from Stevens’ code.

        When a node is ready to start pinging, it first prints out a ‘PING’ message similar to lines 32-33 of Figure 28.5,  p. 744. It then builds up ICMP echo request messages and sends them to the source node every 1 second through the PF_PACKET socket. It also reads incoming echo response messages off the pg socket, in response to which it prints out the same kind of output as the code of Figure 28.8,  p. 748.

        If this node and its preceding node have been previously visited in that order during the tour, then pinging would have already been initiated from the one to the other in response to the first visit, and nothing further should nor need be done during second and subsequent visits.

        In light of the above, note that once a node initiates pinging, it needs to read from both its rt and pg sockets, necessitating the use of the select function. As will be clear from what follows below, the application will anyway be needing also to simultaneously monitor its UDP socket for incoming multicast datagrams.

    When the last node on the tour is reached, and if this is the first time it is visited, it joins the multicast group and starts pinging the preceding node (if it is not already doing so). After a few echo replies are received (five, say), it sends out the multicast message below on its UDP socket (i.e., the node should wait about five seconds before sending the multicast message) :

    <<<<< This is node vmi .  Tour has ended .  Group members please identify yourselves. >>>>>

    where vmi is the name (not IP address) of the node. The node should also print this message out on stdout preceded, on the same line, by the phrase:

    Node vmi .  Sending: <then print out the message sent>.

            Each node vmj receiving this message should print out the message received preceded, on the same line, by the phrase:

            Node vmj .  Received <then print out the message received>.

            Each such node in step a above should then immediately stop its pinging activity.

            The node should then send out the following multicast message:

                    <<<<< Node vmj .  I am a member of the group. >>>>>

            and print out this message preceded, on the same line, by the phrase:

            Node vmj .  Sending: <then print out the message sent>.

            Each node receiving these second multicast messages (i.e., the messages that nodes – including itself – sent out in step c above) should print each such message out preceded, on the same line, by the phrase:

            Node vmk .  Received: <then print out the message received>.

            Reading from the socket in step d above should be implemented with a 5-second timeout. When the timeout expires, the node should print out another message to the effect that it is terminating the Tour application, and gracefully exit its Tour process.

            Note that under Multicast specifications, the last node in the tour, which sends out the End of Tour message, should itself receive a copy of that message and, when it does, it should behave exactly as do the other nodes in steps a. – e. above.

ARP module specifications

Your executable is evoked with no command line arguments. Like the Tour module, it will be running on every vm node.

    It uses the get_hw_addrs function of Assignment 3 to explore its node’s interfaces and build a set of  <IP address , HW address>  matching pairs for all eth0 interface IP addresses (including alias IP addresses, if any).

    Write out to stdout in some appropriately clear format the address pairs found.

    The module creates two sockets: a PF_PACKET socket and a Unix domain socket.

        The PF_PACKET should be of type SOCK_RAW (not type SOCK_DGRAM) with a protocol value of your choice (but not one of the standard values defined in <linux/if_ether.h>) which is, hopefully, unique to yourself. This value effectively becomes the protocol value for your implementation of ARP. Because this protocol value will be carried in the frame type / ‘length’ field of the Ethernet frame header (see Figure 4.3 of the ARP & RARP handout), the value chosen should be not less than 1536 (0x600) so that it is not misinterpreted as the length of an Ethernet 802.3 frame.

        The Unix domain socket should be of type SOCK_STREAM (not SOCK_DGRAM). It is a listening socket bound to a ‘well-known’ sun_path file. This socket will be used to communicate with the function areq that is implemented in the Tour module (see the section API specifications below). In this context, areq will act as the client and the ARP module as the server.

    The ARP module then sits in an infinite loop, monitoring these two sockets.

    As ARP request messages arrive on the PF_PACKET socket, the module processes them, and responds with ARP reply messages as appropriate.

    The protocol builds a ‘cache’ of matching  <IP address , HW address>  pairs from the replies (and requests – see below) it receives. For simplicity, and unlike the real ARP, we shall not implement timing out mechanisms for these cache entries.

    A cache entry has five parts: (i) IP address ;  (ii) HW address ;  (iii) sll_ifindex (the interface to be used for reaching the matching pair <(i) , (ii)>) ;  (iv) sll_hatype ;  and (v) a Unix-domain connection-socket descriptor for a connected client (see the section API specifications below for the latter three). When an ARP reply is being entered in the cache, the ARP module uses the socket descriptor in (v) to send a reply to the client, closes the connection socket, and deletes the socket descriptor from the cache entry.

    Note that, like the real ARP, when an ARP request is received by a node, and if the request pertains to that receiving node, the sender’s (see Figure 4.3 of the ARP & RARP handout) <IP address, HW address> matching pair should be entered into the cache if it is not already there (together, of course, with (iii) sll_ifindex &  (iv) sll_hatype), or updated if need be if such an entry already exists in the cache.

    If the ARP request received does not pertain to the node receiving it, but there is already an entry in that receiving node's cache for the sender’s <IP address, HW address> matching pair, that entry should be checked and updated if need be. If there is no such entry, no action is taken (in particular, and unlike the case above, no new entry should be made in the receiving node's cache of the sender’s <IP address, HW address> matching pair if such an entry does not already exist).

    ARP request and reply messages have the same format as Figure 4.3 of the ARP & RARP handout, but with an extra 2-byte identification field added at the beginning which you fill with a value chosen so that it has a high probability of being unique to yourself. This value is to be echoed in the reply message, and helps to act as a further filter in case some other student happens to have fortuitously chosen the same value as yourself for the protocol parameter of the ARP PF_PACKET. Values in the fields of our ARP messages must be in network byte order. You might find the system header file <linux/if_arp.h> useful for manipulating ARP request and reply messages, but remember that our version of these messages have an extra two-byte field as mentioned above.

    Your code should print out on stdout, in some appropriately clear format, the contents of the Ethernet frame header and ARP request message you send. As described in Section 4.4 of the ARP & RARP handout, the node that responds to the request should, in its reply message, swap the two sender addresses with the two target addresses, as well as, of course, echo back the extra identification field sent with the request. The protocol at this responding node should print out, in an appropriately clear format, both the request frame (header and ARP message) it receives and the reply frame it sends. Similarly, the node that sent the request should print out the reply frame it receives. Finally, recall that the node issuing the request sends out a broadcast Ethernet frame, but the responding node replies with a unicast frame.

API specifications

    The API is for communication between the Tour process and the ARP process. It consists of a single function, areq, implemented in the Tour module. areq is called by send_v4 function of the application every time the latter want to send out an ICMP echo request message:

    int areq (struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr);

    IPaddr contains the primary or alias IPaddress of a ‘target’ node on the LAN for which the corresponding hardware address is being requested.

    hwaddr is a new structure (and not a pre-existing type) modeled on the sockaddr_ll of PF_PACKET; you will have to declare it in your code. It is used to return the requested hardware address to the caller of areq :

*/

/*
    areq creates a Unix domain socket of type SOCK_STREAM and connects to the ‘well-known’ sun_path file of the ARP listening socket. It sends the IP address from parameter IPaddr and the information in the three fields of parameter HWaddr to ARP. It then blocks on a read awaiting a reply from ARP. This read should be backed up by a timeout since it is possible that no reply is received for the request. If a timeout occurs, areq should close the socket and return to its caller indicating failure (through its int return value).

    Your application code should print out on stdout, in some appropriately clear format, a notification every time areq is called, giving the IP address for which a HW address is being sought. It should similarly print out the result when the call to areq returns (HW address returned, or failure).

    When the ARP module receives a request for a HW address from areq through its Unix domain listening socket, it first checks if the required HW address is already in the cache. If so, it can respond immediately to the areq and close the Unix domain connection socket.  Else :  it makes an ‘incomplete’ entry in the cache, consisting of parts (i), (iii), (iv) and (v) ;  puts out an ARP request message on the network on its PF_PACKET socket; and starts monitoring the areq connection socket for readability  –  if the areq client closes the connection socket (this would occur in response to a timeout in areq), ARP deletes the corresponding incomplete entry from the cache (and ignores any subsequent ARP reply from the network if such is received). On the other hand, if ARP receives a reply from the network, it updates the incomplete cache entry, responds to areq, and closes the connection socket.

Hand-in

Submit your code on the minix node using the  electronic hand-in  procedure provided. Your submission must absolutely include :

    a Readme file which identifies the members of the group;

    a Makefile which

        compiles your code using, where necessary, the Stevens’ environment in the course account on the minix node,  /home/users/cse533/Stevens/unpv13e ;  and

        gives the standard names <login>_tour  &  <login>_arp for the executables produced (note the underscore in the executable names), where <login> is the login name your group will use to hand in its copy of the assignment.

*/