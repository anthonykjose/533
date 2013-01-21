
#include <sys/socket.h>
//#include <netpacket/packet.h>
//#include <net/ethernet.h> /* the L2 protocols */
#include "hw_addrs.h"
#include "unp.h"
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#define USID_PROTO 0x4481
#define UNIXDG_PATH "testpath"
#define ETH_FRAME_LEN 1514
#define IDENTIFIER 72217

char own_ip_address[INET_ADDRSTRLEN];
struct arp_frame
{
	uint16_t id;	
	uint16_t hard_type;
	uint16_t proto_type;
	uint8_t hard_size;
	uint8_t proto_size;
	uint16_t op;
	char sender_ethernet_address[6];
	char sender_ip_address[INET_ADDRSTRLEN];
	char target_ethernet_address[6];
	char target_ip_address[INET_ADDRSTRLEN];
};

struct IP_hw_address_mpg
{
	char ip_address[INET_ADDRSTRLEN];
	char hw_address[6];
	int sll_ifindex;
	unsigned short sll_hatype;
	int unix_domain_confd;
	struct routing_entry * next;
}*rt_head, *rt_tmp,*rt_head_own, *rt_tmp_own;


void insert_own_to_collection( char * ip_address,char * hw_address , int if_index)
{

    struct IP_hw_address_mpg *node = (struct IP_hw_address_mpg *)malloc( sizeof(struct IP_hw_address_mpg) );
	
    strcpy( node->ip_address, ip_address );
   // memcpy( node->next_hop_node_ethernet_address, next_hop_node_ethernet_address, 6 );
    node->sll_ifindex =  if_index ;
    memcpy( (void *)node->hw_address, (void *) hw_address, 6 );
	
    if( rt_head_own == NULL )
    {
      rt_head_own = node;
      rt_head_own->next = NULL;			
    } 
    else if( rt_head_own->next == NULL )
    {
      rt_head_own->next = node;
      node->next = NULL;			
    } 
    else
    {
      rt_tmp_own = rt_head_own->next;       
      rt_head_own->next = node;
      node->next = rt_tmp_own;            
    } 
 	return;
 }

/*
	Routing table lookup.
*/
struct IP_hw_address_mpg * get_own_ethernet_from_ip( char * ip_address)
{
	struct IP_hw_address_mpg *node; 
	char * h_name;	

	node = rt_head_own;
	while( node != NULL )
	{
		if( strcmp( node->ip_address, ip_address ) == 0)
		{
			//retrieveHostName( node->destination_canonical_ip_address , h_name);

			return node;
		}	
		
		node = node->next;
	}
	return NULL;	
}


/*
	Retrieve Mac address from interface index.
*/
unsigned char * retrieveMacFromInterfaceIndex( int interface_index )
{
	unsigned char source_mac[6];
	struct hwa_info	*hwa, *hwahead;
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
				//printf("%.2x%s", *ptr1++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);

		
		}
	}

	return source_mac;
}



void sendARPframe( int s , struct arp_frame * populated_arp_frame , char * source_hw_mac_address, char * destination_hw_mac_address , int if_index )
{
	
	int j,i;
	/*target address*/
	struct sockaddr_ll socket_address;

	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN);
	 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
		
	/*pointer to userdata in ethernet frame*/
	unsigned char* data = buffer + 14;
		
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	 
	int send_result = 0,k=0;

	/*our MAC address*/
	unsigned char src_mac[6], dest_mac[6]; 

	char * src_mac1=source_hw_mac_address; 
	char * dest_mac1=destination_hw_mac_address; 


//	unsigned char src_mac[6] = {0x00, 0x0c, 0x29, 0x11, 0x58, 0xa2};
	
	/*Broadcast MAC address*/
	//unsigned char dest_mac[6] = {0x00, 0x0c, 0x29, 0x24, 0x8f, 0x70};
	//unsigned char dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	
	i = IF_HADDR;

	printf("\n\nSending ARP Frame \n Destination H/W Address :\t");
	do 
	{	
		dest_mac[k] = *destination_hw_mac_address++ & 0xff;
		k++;
		printf("%.2x%s", *dest_mac1++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);
	printf("\n");
	

	//printf("\tsending frame on socket: %d\n",s );
	//printf("\tpopulated_odr_frame: %d bytes.\n",sizeof(*populated_odr_frame));
	/*prepare sockaddr_ll*/
	printf(" Source IP Address :  %s\n",populated_arp_frame->sender_ip_address);
	printf(" Source H/W Address :\t");
	i = IF_HADDR;
	k=0;
	do 
	{	
		src_mac[k] = *source_hw_mac_address++ & 0xff;
		k++;
		printf("%.2x%s", *src_mac1++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

	printf("\n");
	printf(" Destination IP Address :  %s\t",populated_arp_frame->target_ip_address);
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

	//printf("Before socket_address..\n");	
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
	eh->h_proto = htons(USID_PROTO);
	/*fill the frame with some data*/
	memcpy((void*)data,(void*)populated_arp_frame, sizeof( struct arp_frame ));
/*
	for (j = 0; j < 1500; j++) {
		data[j] = (unsigned char)((int) (255.0*rand()/(RAND_MAX+1.0)));
	}
*/
	//printf("Just before send.. \n");
	/*send the packet*/
	send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, 
		      (struct sockaddr*)&socket_address, sizeof(socket_address));
	if (send_result == -1){ perror("sendto"); }
	//printf("Done sending..WOO\n");
}

struct arp_frame * processRecievedPacket(char * str_from_sock)
{

	struct arp_frame * recieved_arp_frame;
	int j;
	struct sockaddr_ll socket_address; 	/*target address*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN); 	/*buffer for ethernet frame*/
	unsigned char* etherhead = buffer; 	/*pointer to ethenet header*/
	void * data = buffer + 14;
	
	//printf("\nBeginning processing of recieved packet..\n");
		
	/*pointer to userdata in ethernet frame*/
	
	memcpy((void*)buffer, (void*)str_from_sock, ETH_FRAME_LEN ); 
		
	recieved_arp_frame = (struct arp_frame *)data;
	
	//printf("Converting to Host Byte Order..\n");
	recieved_arp_frame = convertToHostByteOrder( recieved_arp_frame );

	//printf("Done processing of recieved packet..\n");
	return recieved_arp_frame;

}

struct arp_frame * convertToHostByteOrder(struct arp_frame * recieved_arp_frame)
{

	recieved_arp_frame->id=ntohs(recieved_arp_frame->id);	
	recieved_arp_frame->hard_type=ntohs(recieved_arp_frame->hard_type);
	recieved_arp_frame->proto_type=ntohs(recieved_arp_frame->proto_type);
	//recieved_arp_frame->hard_size=ntohs(recieved_arp_frame->hard_size);
	//recieved_arp_frame->proto_size=ntohs(recieved_arp_frame->proto_size);
	recieved_arp_frame->op=ntohs(recieved_arp_frame->op);
	return recieved_arp_frame;
}

void insert_to_cache( char * ip_address,char * hw_address, int if_index, unsigned short hatype, int connfd )
{

    struct IP_hw_address_mpg *node = (struct IP_hw_address_mpg *)malloc( sizeof(struct IP_hw_address_mpg) );
	//printf("Failed..1\n");

	memset(node,0,sizeof(struct IP_hw_address_mpg));
    strcpy( node->ip_address, ip_address );
	//printf("Failed..2\n");
 
    if( strcmp(hw_address,"") != 0 )
    {
    //	printf("Before memcpy in hw_add!=NULL if\n");
    	memcpy( node->hw_address, hw_address, 6 );
    }
 
 	//printf("Here..1\n");
    node->sll_ifindex =  if_index ;
 	//printf("Here..2\n");

//    memcpy( (void *)node->sll_hatype, (void *) hatype, sizeof(unsigned short) );
    node->sll_hatype = hatype;
	//printf("Here..3\n");

	node->unix_domain_confd =  connfd ;
 	//printf("Here..\n");
    if( rt_head == NULL )
    {
 //	printf("1..\n");

      rt_head = node;
      rt_head->next = NULL;			
    } 
    else if( rt_head->next == NULL )
    {
 	//printf("2..\n");

      rt_head->next = node;
      node->next = NULL;			
    } 
    else
    {
 	//printf("3..\n");

      rt_tmp = rt_head->next;       
      rt_head->next = node;
      node->next = rt_tmp;            
    } 
 	return;
 }

/*
	Print the message store. 
*/
void print()
{
	struct IP_hw_address_mpg *node; 	
	printf("LOOKING FOR THE NODE> ::\n ");
	node = rt_head;
	while( node != NULL )
	{

		printf("IP address : %s\n", node->ip_address );
		node = node->next;
	}
	printf("\n***************************\n");

	printf("\n");
	return;	
}

/*
	Routing table lookup.
*/
struct IP_hw_address_mpg * get_ethernet_from_ip( char * ip_address,char * hw_address, char * if_index, char * hatype, int connfd )
{
	struct IP_hw_address_mpg *node; 
	char * h_name;	

	node = rt_head;
	while( node != NULL )
	{
		//printf("searching\n");
		if( strcmp( node->ip_address, ip_address ) == 0)
		{
			//retrieveHostName( node->destination_canonical_ip_address , h_name);
		//	printf("obtained %s in cache..\n", ip_address);
			return node;
		}	
		
		node = node->next;
	}
	//printf("returning null");
	return NULL;	
}
struct IP_hw_address_mpg * get_cache_entry_from_IP( char * ip_address )
{
	struct IP_hw_address_mpg *node; 
	char * h_name;	

	node = rt_head;
	while( node != NULL )
	{
		if( (strcmp( node->ip_address, ip_address ) == 0))
		{
			//retrieveHostName( node->destination_canonical_ip_address , h_name);

			return node;
		}	
		
		node = node->next;
	}
	return NULL;	
}


/*
	Deletes an entry from the routing table entry. 
*/
int cache_delete_entry(int connfd )
{
	struct IP_hw_address_mpg *node; 	
	struct IP_hw_address_mpg *prev; 	
	int returnval=-1;
	//printf("Deleting cache table entry for connfd: %d\n", connfd );

	node = rt_head; 
	while( node != NULL )	
	{

		if( node->unix_domain_confd == connfd )
		
		{
			//delete logic goes here.connfd
			prev->next = node->next;
			node->next = NULL;
			free(node);	
			returnval=1;

		}
		/*else if( strcmp( node->ip_address, ip_address ) == 0 )
		{
			//delete logic goes here.
			prev->next = node->next;
			node->next = NULL;
			free(node);	
		}	*/
		prev = node;
		node = node->next;
	}	
	return returnval;
}

int cache_update_entry( char * ip_address,char * hw_address, char * if_index, unsigned short hatype, int connfd)
{
	struct IP_hw_address_mpg *node; 	
	struct IP_hw_address_mpg *prev; 	
	int returnval=-1;
	

	node = rt_head; 
	while( node != NULL )	
	{

		if( node->unix_domain_confd == connfd && connfd!=-1)
		{
			strcpy(node->hw_address, hw_address);
			returnval=1;

		}
		else if( strcmp( node->ip_address, ip_address ) == 0)
		{
		
            strcpy( node->ip_address, ip_address );
		    if(hw_address!=NULL)
		    {
		    	memcpy( node->hw_address, hw_address, 6 );
		    }
		    node->sll_ifindex =  if_index ;
		    node->sll_hatype = hatype;	
		    node->unix_domain_confd=connfd;	
		    returnval=1;
		}	
		prev = node;
		node = node->next;
	}	
	return returnval;
}



/*
	Get source eth0 canonical IP address 
*/
void getOwnCanonicalIPAddress(char* own_canonical_ip_address)
{
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr_in * ip_address_structure;
	

	/* Flood with broadcast address on all interfaces except eth0 and lo and recieved interface */
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		if( strcmp(hwa->if_name, "eth0")==0 )
		{	
			//printf("Entered if..\n");
			ip_address_structure = (struct sockaddr_in *)hwa->ip_addr; 		
			inet_ntop( AF_INET, &(ip_address_structure->sin_addr), own_canonical_ip_address, 100 );
			//printf("\nSelf's canonical IP address : %s\n", own_canonical_ip_address);	
			return;	
		}
	}	
	strcpy(own_canonical_ip_address, "Not found..");
	//("\nSelf's canonical IP address : %s\n", own_canonical_ip_address);	
	return;
	//return own_canonical_ip_address;
}
struct arp_frame * create_arp( char * target_ip_address, uint16_t op)
{
	
	struct arp_frame * arp_req= (struct arp_frame *)malloc( sizeof(struct arp_frame) );
	char own_canonical_ip_address[INET_ADDRSTRLEN];

	//printf("creating arp\n");
	memset(arp_req,0,sizeof(struct arp_frame));
		//printf("1--\n");
	//printf("1\n");
	arp_req-> hard_type=htons(1);
	//printf("2\n");
	arp_req-> proto_type=htons(USID_PROTO);
	//printf("3\n");
	arp_req-> hard_size=6;
	//printf("4\n");
	arp_req-> proto_size=4;
	//printf("5\n");
	arp_req-> op=htons(op);

	//getOwnCanonicalIPAddress(own_canonical_ip_address);
	//memcpy(arp_req-> sender_ethernet_address, sender_ethernet_address, 6);
	//printf("own_ip_address: %s\n",own_ip_address );
	strcpy(arp_req->sender_ip_address, own_ip_address);
	//memcpy(arp_req-> target_ethernet_address,target_ethernet_address, 6);
	//printf("target_ip_address: %s\n", target_ip_address );
	strcpy(arp_req->target_ip_address, target_ip_address);
	//printf("created arp\n");
	return arp_req;
}

void floodARP( int pf_socket, char * target_ip_address)
{
	struct hwa_info	*hwa, *hwahead;
	char * ifname_split;
	unsigned char flood_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	struct sockaddr	*sa;
	struct arp_frame * populated_arp;
//	printf("inside flood arp\n");
	populated_arp =  create_arp(target_ip_address, 1);
//		printf("populated_arp filled\n");
	//printf("At floodRREQ, size of frame was : %d\n",sizeof(*populated_odr_frame)  );
	/* Flood with broadcast address on all interfaces except eth0 and lo and recieved interface */

		printf("**************************************************************\n");
    	printf("FLOODING ARP REQUEST:\n");
    	printf("**************************************************************\n");
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		//printf("get hw addrss loop\n");
		ifname_split = strtok(hwa->if_name, ":"); //get pointer to first token found and store in 0
	
		
		if( strcmp(ifname_split, "eth0")==0)
		{	
			//printf("Entering sendARPframe..\n");
			if ( (sa = hwa->ip_addr) != NULL)
			{
			//printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
			strcpy(populated_arp->sender_ip_address,Sock_ntop_host(sa, sizeof(*sa)));
			sendARPframe(pf_socket, populated_arp, hwa->if_haddr, flood_mac, hwa->if_index);
			}
			//printf("Leaving SendODR..\n");

		}
	}	
	//printf("Leaving the FloodARP..\n");
	return;
}
void sendARPResponse( int pf_socket, char * target_ip_address,char * target_hw_address, int sll_ifindex )
{
	struct hwa_info	*hwa, *hwahead;
	char * ifname_split;
	unsigned char source_mac[6];

	struct arp_frame * populated_arp;
	int ihw,khw;;
	populated_arp =  create_arp(target_ip_address, 2);
	memcpy(populated_arp->target_ethernet_address,target_hw_address,6);
	memcpy(source_mac,retrieveMacFromInterfaceIndex( sll_ifindex ),6);
	printf("**************************************************************\n");
	printf("SENDING ARP RESPONSE:\n");
	printf("**************************************************************\n");
	ihw = IF_HADDR;
	khw=0;
				        	
	printf("Destination IP address: %s ", target_ip_address);
	
	printf("\n");
	printf("hard_type: %hu, proto_type: %hu, hard_size: %hu ,proto_size %hu,op : %hu\n", populated_arp->hard_type, ntohs(populated_arp->proto_type), populated_arp->hard_size,populated_arp->proto_size,ntohs(populated_arp->op));
	
	sendARPframe(pf_socket, populated_arp, source_mac, target_hw_address, sll_ifindex);

	return;
}


int main(int argc, char const *argv[])
{
		char *msg_fields[MAXLINE], str[MAXLINE],str1[MAXLINE] ;
		struct hwa_info	*hwa, *hwahead;
		char * ifname_split;
		int i, prflag, k, pf_socket, unix_domain_socket, arplen,unixarplen, ihw, khw, maxfdp,len;
		char   *ptr,*ptr1, *hw_address1, sender_ethernet_address[6], buffer[ETH_FRAME_LEN+1], unix_buffer[ETH_FRAME_LEN+1], tmp_hw_address[6], tmp_ip_address[INET_ADDRSTRLEN]; 
		struct sockaddr	*sa;
		struct sockaddr_un unixaddr, unixarpaddr;
		struct sockaddr_ll arpaddr;
		char * dest_mac1; 
		struct IP_hw_address_mpg *IP_hw_address_mpg_collection, *cache, *entry_from_cache, *own_ip_hw_entry;
		struct arp_frame *recvd_packet, *new_arp_rreq_frame;
		int nready, client[FD_SETSIZE], connfd=-1, j;
		ssize_t	n;
		fd_set	rset, allset;
		printf("set of  <IP address , HW address>  matching pairs for all eth0 interface IP addresses\n");
		printf("****************************************************************************************\n");
		for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
		{
			ifname_split = strtok(hwa->if_name, ":"); //get pointer to first token found and store in 0
		
			if( strcmp(ifname_split, "eth0")==0 )
			{	
				printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
				
				if ( (sa = hwa->ip_addr) != NULL)
				{
					strcpy(tmp_ip_address, Sock_ntop_host(sa, sizeof(*sa)));

					printf("IP: %s\t", tmp_ip_address);
				}	
				prflag = 0;
				i = 0;
				do 
				{

					if (hwa->if_haddr[i] != '\0') 
					{
						prflag = 1;
						break;
					}
				} while (++i < IF_HADDR);
			
			k=0;
			if (prflag) 
			{
				printf("HW addr = ");
				ptr = hwa->if_haddr;
				hw_address1=ptr;
				i = IF_HADDR;
				do {
					tmp_hw_address[k] = *ptr++ & 0xff;
					k++;
					printf("%.2x%s", *hw_address1++ & 0xff, (i == 1) ? " " : ":");
				} while (--i > 0);
				insert_own_to_collection( tmp_ip_address,tmp_hw_address, hwa->if_index );
				printf("\n\n");
			}
			}
		}

 	getOwnCanonicalIPAddress(own_ip_address);

	if((pf_socket = socket(PF_PACKET, SOCK_RAW, htons(USID_PROTO)))==-1)
 	{
 		printf("Error in creation of socket for PF_PACKET\n");
 		perror("socket");
 		return 0;
 	}
 
 	if((unix_domain_socket = socket(AF_UNIX, SOCK_STREAM, 0))==-1)
 	{
 		printf("Error in creation of Unix Domain socket\n");
		perror("socket");
		return;
		
	}


	bzero(&unixaddr, sizeof(unixaddr));
	unixaddr.sun_family = AF_UNIX;
	strcpy(unixaddr.sun_path, UNIXDG_PATH);
	unlink(UNIXDG_PATH);
	 len = strlen(unixaddr.sun_path) + sizeof(unixaddr.sun_family);
    if(bind(unix_domain_socket, (SA *) &unixaddr, len)<0)
    {       fprintf(stderr,"bind() failed. errorno =  %d\n",errno); 
            exit(1);
    }
	
    if(listen(unix_domain_socket, LISTENQ)<0)
    {       fprintf(stderr,"listen() failed. errorno = %d\n",errno);       
            exit(1);
    }

	//printf("listening on : %d\n", unix_domain_socket);
	//unixarplen=sizeof(unixarpaddr);
	//connfd = accept(unix_domain_socket, (struct sockaddr_un *) &unixarpaddr, &unixarplen);
	//printf("accepted : %d\n", unix_domain_socket);
	FD_ZERO(&allset);
	//FD_SET(connfd, &allset);
	FD_SET(unix_domain_socket, &allset);
	FD_SET(pf_socket, &allset);
	maxfdp=max(unix_domain_socket,pf_socket)+1;
	/* end fig01 */


	/* include fig02 */
	for ( ; ; ) 
	{
		
		rset = allset;		/* structure assignment */
		if((nready = select(maxfdp, &rset, NULL, NULL, NULL))<0)
		{	
			if(errno==EINTR)	
				continue;
			else
				err_sys("select error");	
		}		
		if (FD_ISSET(unix_domain_socket, &rset)) 
		{	
			
        	unixarplen=sizeof(unixarpaddr);
	        if((connfd = accept(unix_domain_socket, (struct sockaddr_un *) &unixarpaddr, &unixarplen))>=0)
	        {
	   //     	printf("new conn recieved: \n",connfd);
	        	 if ( (n = readline(connfd, unix_buffer, MAXLINE)) >= 0)
				{          
	        	//(n=recvfrom(unix_domain_socket,unix_buffer, ETH_FRAME_LEN+1, 0, &unixarpaddr, &unixarplen)>0)
	      //  		printf("Received  request from Tour module..%s\n",unix_buffer);
	        		

	        		msg_fields[0] = strtok(unix_buffer, "|"); //get pointer to first token found and store in 0
	                while(msg_fields[i]!= NULL) 
	                {   /* ensure a pointer was found */
	                    i++;
	                    msg_fields[i] = strtok(NULL, "|"); /* continue to tokenize the string */
	                }
                
                	for(j = 0; j <= i-1; j++) {
          
                   		 printf("%s\n", msg_fields[j]); /* print out all of the tokens */
                	}
 					//memset(entry_from_cache,0,sizeof(struct IP_hw_address_mpg));
 			//		printf("check in cache\n");
					print();
                	entry_from_cache=get_ethernet_from_ip(msg_fields[0],NULL,msg_fields[1],msg_fields[2],connfd);
               		print();
               		//get_ethernet_from_ip( char * ip_address,char * hw_address, char * if_index, char * hatype, int connfd )
			      //  printf("out of cache\n");
			        if(entry_from_cache!=NULL && entry_from_cache->hw_address!=NULL)
			        {
			        	printf("hw address obtained from cache\n");
			        	write(connfd,entry_from_cache->hw_address, strlen(entry_from_cache->hw_address));
			        	close(connfd);
			        	/*
						A cache entry has five parts:
						 (i) IP address ;  
						(ii) HW address ;  
						(iii) sll_ifindex (the interface to be used for reaching the matching pair <(i) , (ii)>) ;  
						(iv) sll_hatype ;  and
						 (v) a Unix-domain connection-socket descriptor
			        	*/
						 
			        }else
			        {
			    //    	printf("hw address NOT obtained from cache\n");
			        	floodARP(pf_socket,msg_fields[0]/*target ip address*/ );
			      //  	printf("ARP request sent on pf_socket\n");
			        	
			        	insert_to_cache( msg_fields[0],"", msg_fields[1],(unsigned short)atoi(msg_fields[2]),connfd );
			        	//void insert_to_cache( char * ip_address,char * hw_address, int if_index, char * hatype, int connfd )

			        	FD_SET(connfd, &rset);
			        	maxfdp=connfd+1;

			        }
			        
		        }else
		        {
		        	perror("readline");
		        }


	      
			}else{
				perror("connect");
			}
//---------------------------------------------------------------------------------------------------------
		}
        else if (FD_ISSET(pf_socket, &rset)) 
        {       
 
 			//printf("Receiving packet ...\n");
        	arplen=sizeof(arpaddr);
	        if((n=recvfrom(pf_socket,buffer, ETH_FRAME_LEN+1, 0, &arpaddr, &arplen)>0))
	        {



	        	

	        	if (n == -1)
	        	{ 
	        		printf("Error in recieving data from ARP..\n"); 
	        		exit(0);
	        	}
	        	else
	        	{ 
	        		//printf("Recieved Packet successfully\n" ); 
	        	}

	            recvd_packet = (struct arp_frame *)processRecievedPacket(buffer);

	            ihw = IF_HADDR;
	        	khw=0;
	        	
	        //		printf("Received packet from hw address:\n");
				do 
				{	
					sender_ethernet_address[khw] = arpaddr.sll_addr[khw] & 0xff;
					recvd_packet->sender_ethernet_address[khw] = arpaddr.sll_addr[khw] & 0xff;
					
					//printf("%.2x%s", sender_ethernet_address[khw] & 0xff, (ihw == 1) ? " " : ":");
					khw++;
				} while (--ihw > 0);
			//	printf("\n at interface %d...\n",arpaddr.sll_ifindex);

	            if(recvd_packet->op==1)//request
	            {
	            	///printf("ARP request recieved\n");


	            	        ihw = IF_HADDR;
				        	khw=0;
				        	printf("**************************************************************\n");
				        	printf("ARP REQUEST RECEIVED:\n");
				        	printf("**************************************************************\n");
				        	printf("Sender IP Address : %s \n" , recvd_packet->sender_ip_address);
				        	printf("Sender hw address: ");
							do 
							{	
								
								
								printf("%.2x%s", recvd_packet->sender_ethernet_address[khw] & 0xff, (ihw == 1) ? " " : ":");
								khw++;
							} while (--ihw > 0);	


							printf("\nDestination IP Address : %s \n" , recvd_packet->target_ip_address);
				        	
							printf("\n");
							printf("hard_type: %hu, proto_type: %hu, hard_size: %hu ,proto_size %hu,op : %hu\n", recvd_packet->hard_type, recvd_packet->proto_type, recvd_packet->hard_size,recvd_packet->proto_size,recvd_packet->op);
				        	printf("**************************************************************\n");





	            	 own_ip_hw_entry = get_own_ethernet_from_ip( recvd_packet->target_ip_address );
	            	if(own_ip_hw_entry!=NULL)
	            	{
		      //      	printf("make entry in cache table\n");
		            	if(cache=get_ethernet_from_ip( recvd_packet->sender_ip_address,recvd_packet->sender_ethernet_address, arpaddr.sll_ifindex, recvd_packet->hard_type, -1 )!=NULL)
		            	{
			            //	printf("make entry in cache table123\n");

		            	//	printf("gellooo :%hu\n",ntohs(recvd_packet->hard_type) );
			            //	printf("make entry in cache table456\n");

						//	printf("IFIindex :%d\n",arpaddr.sll_ifindex );

		            		//cache->sll_ifindex=(int)arpaddr.sll_ifindex;
		            		
		            	//	printf("gellooo :%hu\n",recvd_packet->hard_type );
		            		//cache->sll_hatype=ntohs(recvd_packet->hard_type);
		            		

		            	}else{
		            		insert_to_cache( recvd_packet->sender_ip_address,recvd_packet->sender_ethernet_address, arpaddr.sll_ifindex, recvd_packet->hard_type, -1 );	
		            	}

		            	sendARPResponse(pf_socket,recvd_packet->sender_ip_address/*target ip address*/ ,sender_ethernet_address, arpaddr.sll_ifindex/*arp response*/);
	            	
	            	}else
	            	{
	            		cache=get_cache_entry_from_IP(recvd_packet->sender_ip_address);
	            		if(cache!=NULL)
	            		{
	            			cache_update_entry( recvd_packet->sender_ip_address,recvd_packet->sender_ethernet_address,arpaddr.sll_ifindex, recvd_packet->hard_type, -1);
	            		}
	            	}
	            }else if(recvd_packet->op==2)
	            {
	            //	printf("ARP response recieved\n");
	            	cache=get_cache_entry_from_IP(recvd_packet->sender_ip_address);
            		if(cache!=NULL)
            		{
            			if(cache->unix_domain_confd!=-1)
            			{
            	//			printf("conn fd from cache: %d\n",cache->unix_domain_confd );
            				//strcpy(str1,"sender_ethernet_address");
            				//sleep(5);
            				ihw = IF_HADDR;
				        	khw=0;
				        	printf("**************************************************************\n");
				        	printf("ARP RESPONSE RECEIVED:\n");
				        	printf("**************************************************************\n");
				        	printf("Sender IP Address : %s \n" , recvd_packet->sender_ip_address);
				        	printf("Sender hw address: ");
							do 
							{	
								
								
								printf("%.2x%s", sender_ethernet_address[khw] & 0xff, (ihw == 1) ? " " : ":");
								khw++;
							} while (--ihw > 0);	


							printf("\nDestination IP Address : %s \n" , recvd_packet->target_ip_address);
				        	printf("Destination hw address: ");
							ihw = IF_HADDR;
				        	khw=0;
				        	
							do 
							{	
								
								
								printf("%.2x%s", recvd_packet->target_ethernet_address[khw] & 0xff, (ihw == 1) ? " " : ":");
								khw++;
							} while (--ihw > 0);	
							printf("\n");
							printf("hard_type: %hu, proto_type: %hu, hard_size: %hu ,proto_size %hu,op : %hu\n", recvd_packet->hard_type, recvd_packet->proto_type, recvd_packet->hard_size,recvd_packet->proto_size,recvd_packet->op);
				        	printf("**************************************************************\n");

				        	printf("**************************************************************\n");
				        	printf("ARP RESPONSE SENT TO TOUR MODULE :\n");
				        	printf("**************************************************************\n");
				        	printf("hw address for requested IP is : ");
							ihw = IF_HADDR;
				        	khw=0;
				        	
							do 
							{	
								
								
								printf("%.2x%s", sender_ethernet_address[khw] & 0xff, (ihw == 1) ? " " : ":");
								khw++;
							} while (--ihw > 0);	


            				if(write(cache->unix_domain_confd,sender_ethernet_address,100)<0)
            					perror("write");
            				


            				FD_CLR(cache->unix_domain_confd,&rset);
							close(cache->unix_domain_confd);
            			}
            			cache_update_entry( recvd_packet->sender_ip_address,recvd_packet->sender_ethernet_address,arpaddr.sll_ifindex, recvd_packet->hard_type, -1);
	            			//printf("I AM HERE ...4\n");

            		}

	            }

	        }

        }
        else if(FD_ISSET(connfd, &rset))
        {
        //	printf("connfd readable\n");
        	n = readline(connfd, str, MAXLINE);
        	if ( n == 0)
			{ 
		//		printf("Connection terminated\n");
				 cache_delete_entry(connfd );
				close(connfd);
				connfd=-1;
        	}
        	/*else if(n>0)
        	{
        		printf("hw address obtained from ARP\n");
        		cache_update_entry(msg_fields[0],msg_fields[1],msg_fields[2],msg_fields[3],connfd, hw_address_from_arp);
        		writen(connfd,hw_address_from_arp, strlen(hw_address_from_arp));
   				close(connfd);
        	
        	}*/
        }

    }
	return 0;
}
