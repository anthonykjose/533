
#include <sys/socket.h>
//#include <netpacket/packet.h>
//#include <net/ethernet.h> /* the L2 protocols */
#include "hw_addrs.h"
#include "unp.h"
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#define USID_PROTO 0x4481
#define ETH_FRAME_LEN 1514
#define ROUTING_BUF_SIZE 100
#define UNIXDG_PATH "testpath"
#define UNIX_SERV_PATH "unixservpath"
#define APP_DATA_PAYLOAD_LEN 1436
#define SERVER_PORT 80
#define TIME_TO_LIVE_MS 100000
int client_port=101;

/*
	Routing table entry node. 
*/
struct routing_entry 
{

	char destination_canonical_ip_address[100];
	char next_hop_node_ethernet_address[6];
	int outgoing_interface_index;
	int number_of_hops_to_destination;
	struct timeval made_or_last_reconfirmed_or_updated_timestamp;
	struct routing_entry * next;

}*rt_head, *rt_tmp;

/*
	Port-Sunpath mapping maintained at the ODR. 
*/
struct port_sunpath_mapping_entry
{
	int  port;
	char sunpath[100];
	char message[100];
	int destination_port_number;
	struct timeval last_updated_timestamp;
	struct port_sunpath_mapping_entry * next;	

}*psm_head,*psm_tmp;

/*
	List of all rreq's recieved at a node.
*/
struct rreq_list
{
	int broadcast_id;
	char source_canonical_ip_address[INET_ADDRSTRLEN];
	struct rreq_list * next;
}*rl_head,*rl_tmp;

/* ODR FRAME ####### */
/*
	Principal Odr-frame that is populated and sent on the PF-PACKET socket. 
*/
struct odr_frame
{
	uint32_t control_msg_type; /* RREQ / RREP/ application payload */

	char source_canonical_ip_address[INET_ADDRSTRLEN];
	//source_sequence_#
	uint32_t broadcast_id; //incremented every time the source issues new RREQ
	char destination_canonical_ip_address[INET_ADDRSTRLEN];
	//dest_sequence_#
	uint32_t number_of_hops_to_destination;

	uint32_t RREP_sent_flag; /* Only for RREQ's */
	uint32_t route_rediscovery_flag; /* RREQs & RREPs */

	/* Application payload specific information. */
	uint32_t source_application_port_number;
	uint32_t destination_application_port_number;
	uint32_t number_of_bytes_in_application_message;	

	char application_data_payload[APP_DATA_PAYLOAD_LEN];
};

/*
	Message store keeps track of messages that are waiting to be sent with routing information waiting to be received. 
*/
struct msg_store
{
	int broadcast_id;
	int source_application_port_number;
	int destination_application_port_number; 
	char message[100];
	struct msg_store * next;
}*ms_head,*ms_tmp;

/*********************************************************************
 *
 * Function    :  insert_to_msg_store
 *
 * Description :  Insert into message store.                      
 *
 * Parameters  :  int broadcast_id ,int source_application_port_number, int destination_application_port_number , char * message
 *      
 * Returns     :  VOID
 *
 *********************************************************************/
void insert_to_msg_store( int broadcast_id ,int source_application_port_number, int destination_application_port_number , char * message )
{

    struct msg_store *node = (struct msg_store *)malloc( sizeof(struct msg_store) );

    strcpy( node->message, message );
    node->broadcast_id =  broadcast_id ;
  	node->source_application_port_number = source_application_port_number;
  	node->destination_application_port_number = destination_application_port_number;

    if( ms_head == NULL )
    {
      ms_head = node;
      ms_head->next = NULL;			
    } 
    else if( ms_head->next == NULL )
    {
      ms_head->next = node;
      node->next = NULL;			
    } 
    else
    {
      ms_tmp = ms_head->next;       
      ms_head->next = node;
      node->next = ms_tmp;            
    } 
 	return;
 }

/*
	Message store lookup.
*/
struct msg_store * msg_store_lookup( int broadcast_id )
{
	struct msg_store *node; 	

	node = ms_head;
	while( node != NULL )
	{
		if( node->broadcast_id == broadcast_id )
		{
			return node;
		}	
		node = node->next;
	}
	return NULL;	
}

/*
	Delete an entry from the message store. 
*/
int msg_store_delete_entry( int broadcast_id )
{
	struct msg_store *node; 	
	struct msg_store *prev; 	

	node = ms_head; 
	while( node != NULL )	
	{
		if( node->broadcast_id == broadcast_id )
		{
			prev->next = node->next;
			node->next = NULL;
			free(node);	
			return 1;
		}	
		prev = node;
		node = node->next;
	}	
	return -1;
}

/*
	Print the message store. 
*/
void print_msg_store()
{
	struct msg_store *node; 	
	printf("\n***************************\n");
	printf("Printing Msg Store For Node\n");
	printf("\n***************************\n");

	node = ms_head;
	while( node != NULL )
	{

		printf("-->Message : %s,Broadcast ID : %d, Source Port Number : %d", node->message,node->broadcast_id, 
																			 node->source_application_port_number );

		node = node->next;
	}
	printf("\n***************************\n");

	printf("\n");
	return;	
}

/*
	Insert entry into port-sunpath mapping table.
*/
struct port_sunpath_mapping_entry * insert_to_port_sunpath_mapping( char * sunpath, int port )
{
    struct port_sunpath_mapping_entry *node = (struct port_sunpath_mapping_entry *)malloc( sizeof(struct port_sunpath_mapping_entry) );

    struct timeval curr_time_ms;
	strcpy( node->sunpath, sunpath );
	gettimeofday(&curr_time_ms, NULL);
	node->last_updated_timestamp = curr_time_ms;

    node->port = port;

    if( psm_head == NULL )
    {
      psm_head = node;
      psm_head->next = NULL;	
    } 
    else if( psm_head->next == NULL )
    {
      psm_head->next = node;
      node->next = NULL;
    } 
    else
    {
      psm_tmp = psm_head->next;       
      psm_head->next = node;
      node->next = psm_tmp;            
    } 
    printf("New entry for sunpath, port : %s, %d\n",sunpath, port);
 	return node;
 }

/*
	Port-sunpath mapping lookup.
*/
struct port_sunpath_mapping_entry * port_sunpath_lookup( char * sunpath, int application_port_number )
{
	struct port_sunpath_mapping_entry *node; 	

	struct timeval curr_time_ms;

	node = psm_head;
	while( node != NULL )
	{
		if( sunpath != NULL )
		{	
			if( strcmp( node->sunpath, sunpath ) == 0 )
			{
				gettimeofday(&curr_time_ms, NULL);
				node->last_updated_timestamp = curr_time_ms;
	
				if(timevaldiff(&(node->last_updated_timestamp),&curr_time_ms)< TIME_TO_LIVE_MS 
					|| node->port == 80)
				{
					return node;
				}
				else
				{
					printf("Entry for port, sunpath : %d, %s  is stale\n", node->port, node->sunpath);
					if(port_sunpath_delete(node->sunpath)!=-1)
					{
						printf("Entry for port, sunpath : %d, %s  deleted\n", node->port, node->sunpath);
					}
					else
					{
						printf("Error while deleting entry for port, sunpath : %d, %s\n", node->port, node->sunpath);
					}
					return NULL;
				}
			}
		}
		else
		{
			if( node->port == application_port_number )
			{
				gettimeofday(&curr_time_ms, NULL);
				node->last_updated_timestamp = curr_time_ms;
	
				if(timevaldiff(&(node->last_updated_timestamp),&curr_time_ms)< TIME_TO_LIVE_MS)
				{
					return node;
				}
				else
				{
					printf("Entry for port, sunpath : %d, %s  is stale\n", node->port, node->sunpath);
					if(port_sunpath_delete(node->sunpath)!=-1)
					{
						printf("Entry for port, sunpath : %d, %s  deleted\n", node->port, node->sunpath);
					}
					else
					{
						printf("Error while deleting entry for port, sunpath : %d, %s\n", node->port, node->sunpath);
					}
					return NULL;
				}
			}	
		}	
		node = node->next;
	}
	return NULL;	
}


/*
	Delete port-sunpath mapping entries.
*/
int port_sunpath_delete( char * sunpath )
{
	struct port_sunpath_mapping_entry *node; 	
	struct port_sunpath_mapping_entry *prev; 	

	node = psm_head; 
	while( node != NULL )	
	{
		if( strcmp( node->sunpath, sunpath ) == 0 )
		{
			//delete logic goes here.
			prev->next = node->next;
			node->next = NULL;
			free(node);	
		}	
		prev = node;
		node = node->next;
	}	
	return -1;
}

/*
	Insert entry to routing table. 
*/
void insert_to_routing_table( 	char* destination_canonical_ip_address,	char* next_hop_node_ethernet_address,
								int outgoing_interface_index, int number_of_hops_to_destination )
{

    struct routing_entry *node = (struct routing_entry *)malloc( sizeof(struct routing_entry) );
	struct timeval curr_time_ms;

	gettimeofday(&curr_time_ms, NULL);
	printf("%s,%d\n",destination_canonical_ip_address, outgoing_interface_index  ); 
    strcpy( node->destination_canonical_ip_address, destination_canonical_ip_address );
    memcpy( node->next_hop_node_ethernet_address, next_hop_node_ethernet_address, 6 );
    node->outgoing_interface_index =  outgoing_interface_index ;
    
    node->number_of_hops_to_destination = number_of_hops_to_destination;
	node->made_or_last_reconfirmed_or_updated_timestamp = curr_time_ms;
   
    if( rt_head == NULL )
    {
      rt_head = node;
      rt_head->next = NULL;			
    } 
    else if( rt_head->next == NULL )
    {
      rt_head->next = node;
      node->next = NULL;			
    } 
    else
    {
      rt_tmp = rt_head->next;       
      rt_head->next = node;
      node->next = rt_tmp;            
    } 
 	return;
 }

/*
	Routing table lookup.
*/
struct routing_entry * routing_table_lookup( char * destination_canonical_ip_address )
{
	struct routing_entry *node; 	

	node = rt_head;
	while( node != NULL )
	{
		if( strcmp( node->destination_canonical_ip_address, destination_canonical_ip_address ) == 0 )
		{
			return node;
		}	
		printf("node->destination_canonical_ip_address : %s\n",node->destination_canonical_ip_address );
		node = node->next;
	}
	return NULL;	
}

/*
	Deletes an entry from the routing table entry. 
*/
int routing_table_delete_entry( char * destination_canonical_ip_address )
{
	struct routing_entry *node; 	
	struct routing_entry *prev; 	

	printf("Deleting routing table entry for : %s\n", destination_canonical_ip_address );

	node = rt_head; 
	while( node != NULL )	
	{
		if( strcmp( node->destination_canonical_ip_address, destination_canonical_ip_address ) == 0 )
		{
			//delete logic goes here.
			prev->next = node->next;
			node->next = NULL;
			free(node);	
		}	
		prev = node;
		node = node->next;
	}	
	return -1;
}

/*
	Print routing table.
*/
void print_routing_table()
{
	struct routing_entry *node; 	
	printf("\n***************************\n");
	printf("Printing Routing Table\n");
	printf("\n***************************\n");

	node = rt_head;
	while( node != NULL )
	{

		printf("-->Destination IP : %s,Outgoing Interface Index : %d", node->destination_canonical_ip_address,node->outgoing_interface_index );

		node = node->next;
	}
	printf("\n***************************\n");

	printf("\n");
	return;	
}

/*
	Print the Sunpath Port mapping.
*/
void print_mapping()
{
	struct port_sunpath_mapping_entry *node; 	

	node = psm_head;
	while( node != NULL )
	{
	printf("Sunpath,PORT mapping entries\n ");
	
		printf("-->%s,%d", node->sunpath,node->port );
		node = node->next;
	}
	printf("\n");
	return;	
}


/* ETHERNET LEVEL FUNCTIONS - creating ethernet headers and sending messages. */

void update_routing_table( char* destination_canonical_ip_address_presentation_format,	char* next_hop_node_ethernet_address,
						   int outgoing_interface_index, int number_of_hops_to_destination )
{
	struct routing_entry * node;
	struct timeval curr_time_ms;

	node = routing_table_lookup( destination_canonical_ip_address_presentation_format );
	gettimeofday(&curr_time_ms, NULL);

	printf("Updating the routing table :\nDestination Canonical IP address : %s\nNew Hop count:%d\n",
									 destination_canonical_ip_address_presentation_format, number_of_hops_to_destination );

	/* Updating the routing table entry. */
	printf("1\n");
	
	strcpy(node->destination_canonical_ip_address, destination_canonical_ip_address_presentation_format);
	printf("2\n");
	
	strcpy(node->next_hop_node_ethernet_address, next_hop_node_ethernet_address);
	printf("3\n");
	node->outgoing_interface_index = outgoing_interface_index;
	printf("4\n");
		
	node->number_of_hops_to_destination = number_of_hops_to_destination;
	printf("5\n");
	
	node->made_or_last_reconfirmed_or_updated_timestamp = curr_time_ms;
	printf("6\n");
	
	return;
}


/* DO THIS BEFORE GENERATING AN RREP WHEN APPROPRIATE */
int enterReverseRoute( char* destination_canonical_ip_address_presentation_format,	char* rreq_ethernet_header_next_hop_node_ethernet_address,
						int outgoing_interface_index, int number_of_hops_to_destination, int flag, int route_rediscovery_flag )
{
	struct routing_entry * node;
	printf("enterReverseRoute 1\n");
	printf("enterReverseRoute 2\n");
	
    node = routing_table_lookup( destination_canonical_ip_address_presentation_format );

    if( node == NULL )
	{
		printf("enterReverseRoute 3\n");
		insert_to_routing_table( destination_canonical_ip_address_presentation_format, rreq_ethernet_header_next_hop_node_ethernet_address,
								 outgoing_interface_index, number_of_hops_to_destination );
		printf("enterReverseRoute 4\n");
		return 1;
	}
	else
	{
		if( (route_rediscovery_flag == 1) )
		{
			printf("Updating the routing entry irrespective of path efficiency (route_rediscovery_flag = 1)..\n");
			update_routing_table( destination_canonical_ip_address_presentation_format, rreq_ethernet_header_next_hop_node_ethernet_address,
								  outgoing_interface_index, number_of_hops_to_destination );
			return 0;			
		}	
		else if(  ( strcmp(node->next_hop_node_ethernet_address, rreq_ethernet_header_next_hop_node_ethernet_address)!=0 ) 
					&& (flag == 0)
					&& (node->number_of_hops_to_destination == number_of_hops_to_destination) )
		{
			printf("Updating route with route through a different neighbour..\n");
			update_routing_table( destination_canonical_ip_address_presentation_format, rreq_ethernet_header_next_hop_node_ethernet_address,
								  outgoing_interface_index, number_of_hops_to_destination );
		}
		else if( (node->number_of_hops_to_destination == number_of_hops_to_destination) )
		{
			printf("Reconfirming route..\n");
			update_routing_table( destination_canonical_ip_address_presentation_format, rreq_ethernet_header_next_hop_node_ethernet_address,
								  outgoing_interface_index, number_of_hops_to_destination );
			return 0;			
		}	
		else if( (node->number_of_hops_to_destination > number_of_hops_to_destination) )
		{
			printf("Updating the routing table with a better path. (Lesser hop count : %d)\n",number_of_hops_to_destination );
			update_routing_table( destination_canonical_ip_address_presentation_format, rreq_ethernet_header_next_hop_node_ethernet_address,
								  outgoing_interface_index, number_of_hops_to_destination );
			return 1;			
		}	
	}
	return 0;	
}



void updateReverseRoute( char* source_canonical_ip_address_presentation_format,char* next_hop_node_ethernet_address,
						int outgoing_interface_index, int number_of_hops_to_destination )
{
	return;
}


/* 
	#Flood's an RREQ on all interfaces  
 	
 	recieved_interface_index = -1 if source node.
 */
void floodRREQ( int sockfd, int recieved_interface_index, char * source_canonical_ip_address,
				int broadcast_id, char * destination_canonical_ip_address,   
				int number_of_hops_to_destination, int RREP_sent_flag,
				int route_rediscovery_flag )
{
	struct hwa_info	*hwa, *hwahead;
	char * ifname_split;
	unsigned char flood_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	struct odr_frame * populated_odr_frame;
	populated_odr_frame =  createRREQ( source_canonical_ip_address,
							   		   broadcast_id, destination_canonical_ip_address,   
							   		   number_of_hops_to_destination, RREP_sent_flag,
									   route_rediscovery_flag);
	printf("At floodRREQ, size of frame was : %d\n",sizeof(*populated_odr_frame)  );
	/* Flood with broadcast address on all interfaces except eth0 and lo and recieved interface */
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		ifname_split = strtok(hwa->if_name, ":"); //get pointer to first token found and store in 0
	
		if( strcmp(ifname_split, "eth0")!=0
			&& strcmp(hwa->if_name,"lo")!=0
			&& hwa->if_index!=recieved_interface_index )
		{	
			printf("Entering SendODR..\n");
			sendODRframe(sockfd, populated_odr_frame, hwa->if_haddr, flood_mac, hwa->if_index);
			printf("Leaving SendODR..\n");

		}
	}	
	printf("Leaving the FloodRREQ..\n");
	return;
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
				printf("%.2x%s", *ptr1++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);

		
		}
	}

	return source_mac;
}

/*
	Send RREP back to the source.
*/
void sendRREP( int sockfd, struct odr_frame * recieved_odr_frame )
{
	struct hwa_info	*hwa, *hwahead;
	struct routing_entry * re; 
	unsigned char source_mac[6];

	re =  check_if_route_exists( recieved_odr_frame->source_canonical_ip_address , recieved_odr_frame->route_rediscovery_flag );
	if( re == NULL )
	{
		printf("Reverse route not found! Probably the staleness_param is too short..\n");
		//floodRREQ()
	}	
	else
	{	
		memcpy(source_mac,retrieveMacFromInterfaceIndex( re->outgoing_interface_index ),6);

		printf("Sending an RREP back on interface %d..\n", re->outgoing_interface_index );
		sendODRframe(sockfd, recieved_odr_frame , source_mac, re->next_hop_node_ethernet_address, re->outgoing_interface_index);
	}	
	return;
}

/*
	Called when an intermediate node recieves the transmit AppPayloadMessage. 
*/
void transmitAppPayloadMessage( int s, struct routing_entry * re ,struct odr_frame * recieved_odr_frame )
{
	char source_mac[6];
	memcpy(source_mac, retrieveMacFromInterfaceIndex( re->outgoing_interface_index ), 6);

	sendODRframe( s , recieved_odr_frame , source_mac, re->next_hop_node_ethernet_address, re->outgoing_interface_index );
	return;
}

/*
	Send Application payload message to the application layer.
*/
int sendToAppLayer( int sockfd, char * application_data_payload ,char * sunpath ,char *source_canonical_ip_address, int source_port_number)
{
	struct sockaddr_un  odraddr;  
    char output_to_sock[MAXLINE];
    char strPort[15] ;
    int n;
    sprintf(strPort,"%d",source_port_number);
      	
    strcpy(output_to_sock, application_data_payload);  

    sprintf(output_to_sock,"%s|%s|%s\n", application_data_payload, 
                                    
                                          source_canonical_ip_address,
                                          strPort);
    
    bzero(&odraddr, sizeof(odraddr)); /* fill in server's address */
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, sunpath);

	printf("Sending data `%s` to application at %s\n", output_to_sock, odraddr.sun_path);
    n=sendto(sockfd,output_to_sock,strlen(output_to_sock),0,&odraddr,SUN_LEN(&odraddr));
    if(n==-1)
    {
    	perror( "sendto" );
    	return -1;
    }

    print_mapping();
    printf("Sent data to application: %s\n", output_to_sock);
	return 1;
}

/*
	Recieve application payload message from the PF-PACKET socket.
*/
void recvAppPayloadMessage( int sockfd, int packet_socket, struct odr_frame * recieved_odr_frame )
{
	char own_canonical_ip_address[INET_ADDRSTRLEN];
	char application_data_payload[APP_DATA_PAYLOAD_LEN];
	int application_port_number, n=-1;
	struct port_sunpath_mapping_entry * psme;
	char sunpath[100];
	struct routing_entry * re;
	struct timeval curr_time_ms;

	getOwnCanonicalIPAddress(own_canonical_ip_address);
	// (if at destination)Stuff the data_payload into message array and send to x based on port mapping data. 
	if( strcmp(recieved_odr_frame->destination_canonical_ip_address,own_canonical_ip_address )==0 )
	{
       

		strcpy(application_data_payload,recieved_odr_frame->application_data_payload);

       	application_port_number = recieved_odr_frame->destination_application_port_number;	 
		
        //printf("\n\nApplication port number : %d\n", application_port_number );
        
        print_mapping();        
        psme = port_sunpath_lookup( NULL, recieved_odr_frame->destination_application_port_number );
		
        printf("sunpath, port entry found : %s,  %d\n", psme->sunpath, recieved_odr_frame->destination_application_port_number);
        
       
        strcpy(sunpath,psme->sunpath);
		
           printf("Entered inside..5\n");

        if(psme!=NULL)
        {
			n = sendToAppLayer( sockfd, application_data_payload, sunpath ,recieved_odr_frame->source_canonical_ip_address, recieved_odr_frame->source_application_port_number );
		}

		if(n==-1)
		{
			printf("Could not write to procces! \n");
		}
		else
		{
			
			gettimeofday(&curr_time_ms, NULL);
			psme->last_updated_timestamp = curr_time_ms;
			printf("Reconfirmed port, sunpath entry : %s, %d\n",psme->sunpath , psme->port );
		}

	}	
	else
	{
		re = check_if_route_exists( recieved_odr_frame->destination_canonical_ip_address, recieved_odr_frame->route_rediscovery_flag );	
		if( re == NULL )
		{
		   //printf("Cannot find Reverse route!! staleness_param too short.\n");
			//FloodRREQ()
		}
		else
		{	
			recieved_odr_frame = preparePacketForResending( recieved_odr_frame );
			transmitAppPayloadMessage(packet_socket,  re ,recieved_odr_frame );
		}
	}	
}

/*
	CREATE RREQ
	Packs up the RREQ as a struct with all necessary variables in Network Byte Order. 
*/
struct odr_frame * createRREQ( char * source_canonical_ip_address,
							   int broadcast_id, char * destination_canonical_ip_address,   
							   int number_of_hops_to_destination, int RREP_sent_flag,
							   int route_rediscovery_flag )

{
	struct odr_frame * populated_odr_frame = (struct odr_frame *) malloc( sizeof( struct odr_frame ) );
	


	populated_odr_frame->control_msg_type = htonl(0);		
	strcpy(populated_odr_frame->source_canonical_ip_address, source_canonical_ip_address);
	populated_odr_frame->broadcast_id = htonl( broadcast_id );
	strcpy(populated_odr_frame->destination_canonical_ip_address, destination_canonical_ip_address);
	populated_odr_frame->number_of_hops_to_destination = htonl(number_of_hops_to_destination);
	populated_odr_frame->RREP_sent_flag = htonl( RREP_sent_flag );
	populated_odr_frame->route_rediscovery_flag = htonl( route_rediscovery_flag );
	printf("Size of the ODR Frame : %d bytes\n", sizeof( *populated_odr_frame ) );	

	return populated_odr_frame;
}


/*
	CREATE RREP
	Packs up the RREP as a struct with all necessary variables in Network Byte Order. 
*/
struct odr_frame * createRREP( char * source_canonical_ip_address,
							   int broadcast_id, char * destination_canonical_ip_address,
							   int number_of_hops_to_destination, int route_rediscovery_flag )
{
	
	struct odr_frame * populated_odr_frame = (struct odr_frame *) malloc( sizeof( struct odr_frame ) );

	printf("Size of the ODR Frame : %d bytes\n", sizeof( struct odr_frame ) );	
	populated_odr_frame->control_msg_type = htonl( 1 );		
	strcpy(populated_odr_frame->source_canonical_ip_address, source_canonical_ip_address);
	populated_odr_frame->broadcast_id = htonl( broadcast_id );
	strcpy(populated_odr_frame->destination_canonical_ip_address, destination_canonical_ip_address);
	populated_odr_frame->number_of_hops_to_destination = htonl(number_of_hops_to_destination);

	populated_odr_frame->route_rediscovery_flag = htonl( route_rediscovery_flag );

	return populated_odr_frame;
}

/*
	Creates an APPLICATION PAYLOAD MESSAGE 
	Packs the data into an odr frame in Network Byte Order and returns a pointer to it. 
*/
struct odr_frame * createApplicationPayloadMessage( char * source_canonical_ip_address,
							   						int broadcast_id, char * destination_canonical_ip_address,   
							   						int number_of_hops_to_destination, int source_application_port_number,
							   						int destination_application_port_number, char * application_data_payload ,
							   						int number_of_bytes_in_application_message )
{
	
	struct odr_frame * populated_odr_frame = (struct odr_frame *) malloc( sizeof( struct odr_frame ) );
	printf("Size of the ODR Frame : %d bytes\n", sizeof( struct odr_frame ) );	
	populated_odr_frame->control_msg_type = htonl(2);		
	strcpy(populated_odr_frame->source_canonical_ip_address, source_canonical_ip_address);
	populated_odr_frame->broadcast_id = htonl( broadcast_id );
	strcpy(populated_odr_frame->destination_canonical_ip_address, destination_canonical_ip_address);
	populated_odr_frame->number_of_hops_to_destination = htonl(number_of_hops_to_destination);

	populated_odr_frame->source_application_port_number = htonl( source_application_port_number );
	populated_odr_frame->destination_application_port_number = htonl( destination_application_port_number );
	populated_odr_frame->number_of_bytes_in_application_message = htonl( number_of_bytes_in_application_message );	
	strcpy(populated_odr_frame->application_data_payload, application_data_payload);

	return populated_odr_frame;
}

/*
	Pushes in the source eth0 canonical IP address into a global variable. 
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
			printf("Entered if..\n");
			ip_address_structure = (struct sockaddr_in *)hwa->ip_addr; 		
			inet_ntop( AF_INET, &(ip_address_structure->sin_addr), own_canonical_ip_address, 100 );
			printf("\nSelf's canonical IP address : %s\n", own_canonical_ip_address);	
			return;	
		}
	}	
	strcpy(own_canonical_ip_address, "Not found..");
	printf("\nSelf's canonical IP address : %s\n", own_canonical_ip_address);	
	return;
	//return own_canonical_ip_address;
}


struct routing_entry * check_if_route_exists( char * destination_canonical_ip_presentation_format , int route_rediscovery_flag);
//routing_entry recv_ODR();
long staleness_param;


/*
	Principal frame-sending function. Packs the data and sends it on PF-PACKET socket.
*/
void sendODRframe( int s , struct odr_frame * populated_odr_frame , char * source_hw_mac_address, char * destination_hw_mac_address , int if_index )
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
	printf("\n\nSending ODR Frame \n\t Destination H/W Address :\n");
	do 
	{	
		dest_mac[k] = *destination_hw_mac_address++ & 0xff;
		k++;
		printf("%.2x%s", *dest_mac1++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);
	printf("\n");
	

	printf("\tsending frame on socket: %d\n",s );
	printf("\tpopulated_odr_frame: %d bytes.\n",sizeof(*populated_odr_frame));
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
	eh->h_proto = htons(USID_PROTO);
	/*fill the frame with some data*/
	memcpy((void*)data,(void*)populated_odr_frame, sizeof( struct odr_frame ));
/*
	for (j = 0; j < 1500; j++) {
		data[j] = (unsigned char)((int) (255.0*rand()/(RAND_MAX+1.0)));
	}
*/
	printf("Just before send.. \n");
	/*send the packet*/
	send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, 
		      (struct sockaddr*)&socket_address, sizeof(socket_address));
	if (send_result == -1){ perror("sendto"); }
	printf("Done sending..WOO\n");
}

/*
	Convert relevant data from the frame to Network Byte Order before sending on network.
*/
struct odr_frame * convertToNetworkByteOrder( struct odr_frame * recieved_odr_frame )
{
	uint32_t control_msg_type;

	control_msg_type = recieved_odr_frame->control_msg_type;

	recieved_odr_frame->broadcast_id = htonl( recieved_odr_frame->broadcast_id );
	recieved_odr_frame->number_of_hops_to_destination = htonl( recieved_odr_frame->number_of_hops_to_destination );
	
	if( control_msg_type == 0 )
	{
		recieved_odr_frame->RREP_sent_flag = htonl( recieved_odr_frame->RREP_sent_flag );
	}
	if( (control_msg_type == 1) 
		|| (control_msg_type == 0) )
	{
		recieved_odr_frame->route_rediscovery_flag = htonl(recieved_odr_frame->route_rediscovery_flag);
	}	
	if( control_msg_type == 2 )
	{
		recieved_odr_frame->source_application_port_number = htonl(recieved_odr_frame->source_application_port_number);
		recieved_odr_frame->destination_application_port_number = htonl( recieved_odr_frame->destination_application_port_number );
		recieved_odr_frame->number_of_bytes_in_application_message = htonl( recieved_odr_frame->number_of_bytes_in_application_message );
	}	

	recieved_odr_frame->control_msg_type = htonl( recieved_odr_frame->control_msg_type );

	return recieved_odr_frame;
}

/*
	Prepare packet before sending on the network.
*/
struct odr_frame * preparePacketForResending( struct odr_frame * recvd_packet )
{
	printf("\nPreparing packet for resending..\n\t - Converting to NBO.\n");
	return convertToNetworkByteOrder( recvd_packet );
}

/*
	CONVERT TO HOST BYTE ORDER
	Converts the recieved odr_frame structure to NBO.
*/
struct odr_frame * convertToHostByteOrder( struct odr_frame * recieved_odr_frame )
{
	uint32_t control_msg_type;

	recieved_odr_frame->control_msg_type = ntohl( recieved_odr_frame->control_msg_type );
	control_msg_type = recieved_odr_frame->control_msg_type;

	recieved_odr_frame->broadcast_id = ntohl( recieved_odr_frame->broadcast_id );
	recieved_odr_frame->number_of_hops_to_destination = ntohl( recieved_odr_frame->number_of_hops_to_destination );
	
	if( control_msg_type == 0 )
	{
		recieved_odr_frame->RREP_sent_flag = ntohl( recieved_odr_frame->RREP_sent_flag );
	}
	if( (control_msg_type == 1) 
		|| (control_msg_type == 0) )
	{
		recieved_odr_frame->route_rediscovery_flag = ntohl(recieved_odr_frame->route_rediscovery_flag);
	}	
	if( control_msg_type == 2 )
	{
		recieved_odr_frame->source_application_port_number = ntohl(recieved_odr_frame->source_application_port_number);
		recieved_odr_frame->destination_application_port_number = ntohl( recieved_odr_frame->destination_application_port_number );
		recieved_odr_frame->number_of_bytes_in_application_message = ntohl( recieved_odr_frame->number_of_bytes_in_application_message );
	}	
	return recieved_odr_frame;
}

/*
	Process the recieved packet. Convert to HBO and return the packet.
*/
struct odr_frame * processRecievedPacket(char * str_from_sock)
{
	struct odr_frame * recieved_odr_frame;
	int j;
	struct sockaddr_ll socket_address; 	/*target address*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN); 	/*buffer for ethernet frame*/
	unsigned char* etherhead = buffer; 	/*pointer to ethenet header*/
	void * data = buffer + 14;
	
	printf("\nBeginning processing of recieved packet..\n");
		
	/*pointer to userdata in ethernet frame*/
	printf("1\n");
		memcpy((void*)buffer, (void*)str_from_sock, ETH_FRAME_LEN ); 
		printf("2\n");
	recieved_odr_frame = (struct odr_frame *)data;
	printf("3\n");

	printf("\t- Converting to Host Byte Order..\n");
	recieved_odr_frame = convertToHostByteOrder( recieved_odr_frame );

	printf("Done processing of recieved packet..\n");
	return recieved_odr_frame;

}

/*
	Insert into the RREQ-list. 
*/
void insert_to_rreq_list( int broadcast_id ,char* source_canonical_ip_address )
{

    struct rreq_list *node = (struct rreq_list *)malloc( sizeof(struct rreq_list) );

    strcpy( node->source_canonical_ip_address, source_canonical_ip_address );
    node->broadcast_id =  broadcast_id ;
    
    if( rl_head == NULL )
    {
      rl_head = node;
      rl_head->next = NULL;			
    } 
    else if( rl_head->next == NULL )
    {
      rl_head->next = node;
      node->next = NULL;			
    } 
    else
    {
      rl_tmp = rl_head->next;       
      rl_head->next = node;
      node->next = rl_tmp;            
    } 
 	return;
 }

struct rreq_list * rreq_list_lookup( char * source_canonical_ip_address )
{
	struct rreq_list *node; 	

	node = rl_head;
	while( node != NULL )
	{
		if( strcmp( node->source_canonical_ip_address, source_canonical_ip_address ) == 0 )
		{
			return node;
		}	
		node = node->next;
	}
	return NULL;	
}

int rreq_list_delete_entry( char * source_canonical_ip_address )
{
	struct rreq_list *node; 	
	struct rreq_list *prev; 	

	node = rl_head; 
	while( node != NULL )	
	{
		if( strcmp( node->source_canonical_ip_address, source_canonical_ip_address ) == 0 )
		{
			prev->next = node->next;
			node->next = NULL;
			free(node);	
			return 1;
		}	
		prev = node;
		node = node->next;
	}	
	return -1;
}

void print_rreq_list()
{
	struct rreq_list *node; 	
	printf("\n***************************\n");
	printf("Printing RREQ List For Node\n");
	printf("\n***************************\n");

	node = rl_head;
	while( node != NULL )
	{

		printf("-->Source IP : %s,Broadcast ID : %d", node->source_canonical_ip_address,node->broadcast_id );

		node = node->next;
	}
	printf("\n***************************\n");

	printf("\n");
	return;	
}

void update_rreq_list( int broadcast_id, char * source_canonical_ip_address )
{
	struct rreq_list * rl;
	rl = rreq_list_lookup( source_canonical_ip_address );
	if( rl == NULL )
	{	
		insert_to_rreq_list( broadcast_id, source_canonical_ip_address );
	}
	else
	{
		rl->broadcast_id = broadcast_id;
	}	
	return;
}

int enterNewRREQtoList( int broadcast_id, char * source_canonical_ip_address )
{
	struct rreq_list * rl;
	rl = rreq_list_lookup( source_canonical_ip_address );
	
	if( rl == NULL )
	{	
		insert_to_rreq_list( broadcast_id, source_canonical_ip_address );
		return 1;
	}
	else if( rl->broadcast_id < broadcast_id )
	{
		printf("Found higher BROADCAST ID, so updating source BROADCAST ID list.\n");
		update_rreq_list(broadcast_id, source_canonical_ip_address);		
		printf("Done updating..\n");
		return 1;
	}	
	return 0;	
}

/*
	Process the recieved RREQ packet.
*/
void processRREQPacket( int packet_socket, struct odr_frame * recvd_packet,  
						char * next_hop_node_ethernet_address, struct sockaddr_ll  odraddr, char * source_addr )
{
	int route_exists, notifyOthers, new_rreq;
	struct rreq_list * rl;	
	int old_rreq_good;

	printf("5\n");

	new_rreq = enterNewRREQtoList( recvd_packet->broadcast_id, recvd_packet->source_canonical_ip_address );
    print_routing_table();
	notifyOthers = enterReverseRoute( recvd_packet->source_canonical_ip_address,
									  next_hop_node_ethernet_address,
									  /*interface index*/odraddr.sll_ifindex,recvd_packet->number_of_hops_to_destination, 0 , recvd_packet->route_rediscovery_flag);
	
    print_routing_table();
	printf("notifyOthers----->%d\n",notifyOthers );
    rl = rreq_list_lookup( recvd_packet->source_canonical_ip_address );									  
    printf("NOTIFY OTHERS : %d, NEW RREQ ? : %d, PACKET BROADCAST ID : %d\n", notifyOthers, new_rreq, recvd_packet->broadcast_id );

/*
	if( rl->broadcast_id == recvd_packet->broadcast_id )
	{
		new_rreq = 0;
	}
	else
	{
		new_rreq = 1;
	}	
*/
	if (new_rreq == 0)
	{
		if( notifyOthers == 1 )
		{
			old_rreq_good = 1;
		}
	}
	else
	{
			old_rreq_good=0;	
	}
			
	printf("6\n");
	if(strcmp(recvd_packet->destination_canonical_ip_address,source_addr)==0)
	{
		printf("recvd_packet->destination_canonical_ip_address %s, source_addr %s 7\n",recvd_packet->destination_canonical_ip_address,source_addr);
		//odr is at the destination node 
		recvd_packet->number_of_hops_to_destination=0;
		recvd_packet->route_rediscovery_flag = 0;

		if(!recvd_packet->RREP_sent_flag && (old_rreq_good || new_rreq) )
		{
			recvd_packet->control_msg_type = 1;
			recvd_packet = preparePacketForResending( recvd_packet );
			sendRREP( packet_socket, recvd_packet);
		}
        				
	}else /* Intermediate node recieved an RREQ */
	{
		printf("8\n");

		if(!recvd_packet->route_rediscovery_flag)
		{
			printf("checking entry for `%s` in routing table  \n", recvd_packet->destination_canonical_ip_address);
			route_exists=check_if_route_exists(recvd_packet->destination_canonical_ip_address, recvd_packet->route_rediscovery_flag ); //pass address of existing_entry so that its value can be set inside the function
		}
		else
		{	
			route_exists=0;
		}

		if(route_exists)
		{
			printf("Route found! \n SEND RREP\n");
							 
			if((!recvd_packet->RREP_sent_flag) && (old_rreq_good || new_rreq) )
			{
				recvd_packet->control_msg_type = 1;
				recvd_packet = preparePacketForResending( recvd_packet );
				sendRREP( packet_socket, recvd_packet);
				recvd_packet->RREP_sent_flag=1;
			}

			if( (old_rreq_good || new_rreq) )
			{	
				floodRREQ( packet_socket, odraddr.sll_ifindex/*recieved_interface_index*/, recvd_packet->source_canonical_ip_address,
						   recvd_packet->broadcast_id, recvd_packet->destination_canonical_ip_address,   
						   recvd_packet->number_of_hops_to_destination, recvd_packet->RREP_sent_flag, recvd_packet->route_rediscovery_flag );
			}

		}
		else
		{
			if( !recvd_packet->route_rediscovery_flag )
				printf("Route not found..\n");
			else
				printf( "Route Rediscovery Flag is set. Ignoring existing routes..\n" );				
		
			if( notifyOthers || recvd_packet->route_rediscovery_flag )
			{	
				floodRREQ( packet_socket, odraddr.sll_ifindex/*recieved_interface_index*/, recvd_packet->source_canonical_ip_address,
						   recvd_packet->broadcast_id, recvd_packet->destination_canonical_ip_address,   
						   recvd_packet->number_of_hops_to_destination, recvd_packet->RREP_sent_flag, recvd_packet->route_rediscovery_flag );
			}
		
		}
		
		print_routing_table();		
	}
	return;
}

/*
	Send the application payload message onto the network.
*/
void sendAppPayload( int packet_socket, struct routing_entry * re, char * source_canonical_ip_address, char * destination_canonical_ip_address,
					 int source_application_port_number, int destination_application_port_number, char * application_data_payload,
					 int number_of_bytes_in_application_message )
{
	
    struct odr_frame * populated_odr_frame;
	char * source_mac;
    printf("sendAppPayload 1 %s \n", destination_canonical_ip_address);
	populated_odr_frame = createApplicationPayloadMessage( source_canonical_ip_address, -1,
													  	   destination_canonical_ip_address,   
								 						   0, source_application_port_number,
							   							   destination_application_port_number, application_data_payload ,
							   							   number_of_bytes_in_application_message );
	printf("sendAppPayload 2\n");
    source_mac = retrieveMacFromInterfaceIndex( re->outgoing_interface_index );
	printf("sendAppPayload 3\n" );
    sendODRframe( packet_socket ,populated_odr_frame, source_mac, 
				  re->next_hop_node_ethernet_address, re->outgoing_interface_index );
	return;
}

int main(int argc, char const *argv[])
{
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	char   *ptr;
	int    i, j, prflag,broadcast_id=0,n,s,len,clilen,pathlen,prolen;
	struct odr_frame * route_exists;
	int packet_socket;
	int nready , odrlen;
    struct sockaddr_un procaddr;
	int					sockfd;
	struct sockaddr_un	cliaddr, servaddr;
	struct sockaddr_ll  odraddr;  
		struct sockaddr_in addr2;
	char data_stream[MAXLINE];
	char *msg_fields[MAXLINE];
	char str_from_sock[MAXLINE], source_addr[INET_ADDRSTRLEN];
	char*	 destination_canonical_ip_presentation_format;
	char * source_mac;
	int	 destination_port_number;
	char*	 message_to_be_sent;
	int	 route_rediscovery_flag;
	struct odr_frame RREQ,RREP;
	struct routing_entry existing_entry, *re;
	struct odr_frame req_type;
	struct odr_frame * recvd_packet;
	struct port_sunpath_mapping_entry *node;
	struct msg_store * msg_store_entry;
	char own_canonical_ip_address[INET_ADDRSTRLEN];

	int maxfdp1;
	fd_set rset;
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0; /*length of the received frame*/ 
	int notifyOthers;
  	
  	char next_hop_node_ethernet_address[6];
  	int ihw,khw=0;
	if(argc<2)
	{
		printf("Invalid args: ODR <Staleness parameter in seconds>\n");
		return 0;
	}
	else
	{
		staleness_param = atoi( argv[1] );
		printf( "\nStaleness parameter is : %ld seconds.\n", staleness_param );
		

	}
	getOwnCanonicalIPAddress(source_addr);
	printf("Source address: %s\n", source_addr);

    insert_to_port_sunpath_mapping( UNIX_SERV_PATH, SERVER_PORT );

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		
		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		
		if ( (sa = hwa->ip_addr) != NULL)
			printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\n         interface index = %d\n\n", hwa->if_index);
	}

 	if((packet_socket = socket(PF_PACKET, SOCK_RAW, htons(USID_PROTO)))==-1)
 	{
 		printf("Error in creation of socket for PF_PACKET\n");
 		perror("socket");
 		return 0;
 	}
 
 	if((sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0))==-1)
 	{
 		printf("Error in creation of Unix Domain socket\n");
		perror("socket");
		return;
		
	}
	unlink(UNIXDG_PATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXDG_PATH);

	bind(sockfd, (SA *) &servaddr, SUN_LEN(&servaddr));

	printf("unix domain socket bound %d\n", sockfd);

	printf(" packet_socket socket bound %d\n", packet_socket);
   
    maxfdp1 = max(packet_socket, sockfd) + 1;
    for ( ; ; ) 
    {
        printf("in loop...\n" );
         FD_ZERO(&rset);
        FD_SET(packet_socket, &rset);
        FD_SET(sockfd, &rset);
        if ( (nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR)
              continue;   /* back to for() */
            else
              err_sys("select error");
        }
        printf("nready...%d\n",nready);
        if (FD_ISSET(sockfd, &rset)) 
        {
        	printf("Receiving from client/server..%d bytes.\n", n);
        	
        	memset( data_stream, 0, MAXLINE ); 
        	prolen=sizeof(procaddr);
        	n=recvfrom(sockfd,data_stream,MAXLINE,0,&procaddr,&prolen);
            printf("received from sun_path: %s\n", procaddr.sun_path);
            node = port_sunpath_lookup( procaddr.sun_path, 0);
			if (node ==NULL)
			{
				client_port++;
				node = insert_to_port_sunpath_mapping( procaddr.sun_path, client_port );

			}
			print_mapping();
           	if( n == -1 )
           		perror("recvfrom");
           
           // if(data_stream)
            {
				printf("data_stream: %s\n",data_stream );

				msg_fields[0] = strtok(data_stream, "|"); //get pointer to first token found and store in 0
			    i=0;                                   //place in array
			    while(msg_fields[i]!= NULL) 
			    {   //ensure a pointer was found
			       
			        i++;
			        msg_fields[i] = strtok(NULL, "|"); //continue to tokenize the string
			    }
			    			
				 destination_canonical_ip_presentation_format=msg_fields[0];
				 destination_port_number=atoi(msg_fields[1]);
				 message_to_be_sent=msg_fields[2];
				 route_rediscovery_flag=atoi(msg_fields[3]);
				 
				 
				
				if(!route_rediscovery_flag)
				{
					printf("checking entry for `%s` in routing table  \n", destination_canonical_ip_presentation_format);
					route_exists=check_if_route_exists(destination_canonical_ip_presentation_format, route_rediscovery_flag ); //pass address of existing_entry so that its value can be set inside the function
				}
				else
				{	
					route_exists=NULL;

				}

				if(route_exists != NULL)
				{
					node = port_sunpath_lookup( procaddr.sun_path, 0 );	
					printf("Route found! \n SEND MESG\n");
					getOwnCanonicalIPAddress( own_canonical_ip_address );
					
					sendAppPayload(  packet_socket, route_exists, own_canonical_ip_address, destination_canonical_ip_presentation_format,
			  					    node->port, destination_port_number, message_to_be_sent,
					 				sizeof(message_to_be_sent) );

				}else
				{
					if(!route_rediscovery_flag)
					{
						printf("Route not found..\n");
					}
					else{
						printf("Forced discovery flag set..\n" );
					}

					broadcast_id++;
					printf("ANTHONY BROADCAST ID : %d\n PORT\n,node port = %d,DESTINATION PORT NUMBER : %d\n, MESSAGE TO BE SENT : %s\n", 
													broadcast_id, node->port, destination_port_number, message_to_be_sent  );		
					//recieved_interface_index
					insert_to_msg_store(broadcast_id, node->port , destination_port_number ,message_to_be_sent);

					floodRREQ( packet_socket, -1, source_addr,
								broadcast_id, destination_canonical_ip_presentation_format,   
								0, 0,
								route_rediscovery_flag );
					printf("RREQ broadcast sent..\n");
					//sleep(2);
				}
				
			}//else
			//{
			//	printf("receive error : %s\n",hstrerror(h_errno) );
			//}

				
        }else if(FD_ISSET(packet_socket,&rset))
        {
        	printf("Receiving packet from ODR...\n");
        	odrlen=sizeof(odraddr);
	        if((n=recvfrom(packet_socket,buffer, ETH_FRAME_LEN, 0, &odraddr, &odrlen)>0))
	        {

				ihw = IF_HADDR;
	        	khw=0;

	        		printf("Received packet from hw address:\n");
				do 
				{	
					next_hop_node_ethernet_address[khw] = odraddr.sll_addr[khw] & 0xff;
					
					printf("%.2x%s", next_hop_node_ethernet_address[khw] & 0xff, (ihw == 1) ? " " : ":");
					khw++;
				} while (--ihw > 0);
				printf("\n at interface %d...\n",odraddr.sll_ifindex);

	        	

	        	if (n == -1)
	        	{ 
	        		printf("Error in recieving data from client..\n"); 
	        		exit(0);
	        	}
	        	else
	        	{ 
	        		printf("Recieved Packet Size : %d\n",sizeof(*buffer) ); 
	        	}

	            recvd_packet = (struct odr_frame *)processRecievedPacket(buffer);
	            /* Packet is now in Host Byte Order with all the data in the odr_frame structure. */

	            printf("processRecievedPacket done.PACKET SIZE : %d.\n", sizeof(*recvd_packet) );
	            recvd_packet->number_of_hops_to_destination=recvd_packet->number_of_hops_to_destination+1;
	            printf("sRecievedPacket accessed..NUMBER OF HOPS :%d\n" , recvd_packet->number_of_hops_to_destination);
	 
	            if(recvd_packet->control_msg_type==0) //RREQ
	            {
	            	printf("RREQ received..\n");
	     			processRREQPacket(packet_socket,recvd_packet,next_hop_node_ethernet_address, odraddr, source_addr);
		     		printf("RREQ processing over..\n");
		     	}
	         	else if(recvd_packet->control_msg_type==1) //RREP
	            {
	            	printf("RREP received..\n");
	          //  	recvd_packet->route_rediscovery_flag=1;
					
					/* Forward routes. */
					enterReverseRoute( recvd_packet->destination_canonical_ip_address,
										next_hop_node_ethernet_address,
										/*interface index*/odraddr.sll_ifindex,recvd_packet->number_of_hops_to_destination, 1 , 0);


					if( strcmp(  recvd_packet->source_canonical_ip_address, source_addr)==0 )
					{
						/* We are at the source finally. */
						printf("PACKET RECIEVED AT THE SOURCE : IT TOOK %d hops to accomplish this.\n", recvd_packet->number_of_hops_to_destination );
						recvd_packet->number_of_hops_to_destination=0;
	 
						printf("\nRETRIEVED A PATH !!\n");
						/*  */
						re = check_if_route_exists( recvd_packet->destination_canonical_ip_address, recvd_packet->route_rediscovery_flag );	
						if( re == NULL )
						{
							printf("Unable to find route ! staleness_param is too short..\n");
						}	
						else
						{
							//printf("BROADCAST ID %d\n",recvd_packet->broadcast_id );
							//print_msg_store();
							msg_store_entry = msg_store_lookup( recvd_packet->broadcast_id );
                            //print_msg_store();	
							recvd_packet->control_msg_type = 2;
							recvd_packet->RREP_sent_flag = 0;
							recvd_packet->route_rediscovery_flag=0;
							recvd_packet->number_of_hops_to_destination=0;
							recvd_packet->source_application_port_number= msg_store_entry->source_application_port_number;
							recvd_packet->destination_application_port_number= msg_store_entry->destination_application_port_number;
							printf("source_application_port_number %d\n",recvd_packet->source_application_port_number);
                            printf("destination_application_port_number %d\n",recvd_packet->destination_application_port_number );
                            strcpy(recvd_packet->application_data_payload,msg_store_entry->message);
							recvd_packet->number_of_bytes_in_application_message = sizeof(msg_store_entry->message);
							recvd_packet = preparePacketForResending( recvd_packet );
							
							source_mac = retrieveMacFromInterfaceIndex( re->outgoing_interface_index );

							sendODRframe( packet_socket , recvd_packet , source_mac, re->next_hop_node_ethernet_address , re->outgoing_interface_index );
							//msg_store_delete_entry( ntohl(recvd_packet->broadcast_id) );
						}
					}		            
		           	else
		           	{
          				recvd_packet->control_msg_type = 1;
		           		recvd_packet = preparePacketForResending( recvd_packet );
		           		sendRREP( packet_socket, recvd_packet);
	            	}
	            }else// APPLICATION DATA PAYLOAD RECIEVED
	            {
	            	//recvd_packet	
	            	printf("application_data_payload received\n ");
                  	
					enterReverseRoute( recvd_packet->source_canonical_ip_address,	
                                        next_hop_node_ethernet_address,
                                        /*interface index*/odraddr.sll_ifindex,recvd_packet->number_of_hops_to_destination, 1 , 0);
                    recvAppPayloadMessage( sockfd, packet_socket, recvd_packet );
		            	
	            }
		     }
		    }
	       
	   
    
  }

	/* code */
	return 0;
}

long timevaldiff(struct timeval *starttime, struct timeval *finishtime)
{
	long msec;
	
	msec = (finishtime->tv_sec-starttime->tv_sec)*1000;
	msec += (finishtime->tv_usec-starttime->tv_usec)/1000;
	return msec;
}

struct routing_entry * check_if_route_exists( char * destination_canonical_ip_presentation_format , int route_rediscovery_flag)
{
  	long msec;
	int count=0;
	int route_found=0;
	struct timeval curr_time_ms; 
	struct routing_entry * node; 
	
    printf("Destination : %s\n", destination_canonical_ip_presentation_format );
    print_routing_table();
	node = routing_table_lookup( destination_canonical_ip_presentation_format );
	
	if( node != NULL )
	{
			printf("1\n");
			gettimeofday(&curr_time_ms, NULL);
			printf("2\n");
		  	msec = timevaldiff( &(node->made_or_last_reconfirmed_or_updated_timestamp), &curr_time_ms );
			printf("3\n");
				
			if(msec >= staleness_param*1000 
				|| route_rediscovery_flag == 1)
			{

				printf("Time diff : %ld\n", msec);
				routing_table_delete_entry( destination_canonical_ip_presentation_format );
            }
			else
			{
				route_found = 1;
			printf("4\n");

				node->made_or_last_reconfirmed_or_updated_timestamp = curr_time_ms;
			printf("5\n");

				return node; 
			}
	}
	return NULL;
}
