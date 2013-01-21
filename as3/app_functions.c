#include  "unp.h"
#define UNIXDG_PATH "testpath"
/*
  This function will write this info in a single char format to sockfd_for_write.
*/
void msg_send( int sockfd_for_write, 
               char *destination_canonical_ip_presentation_format, 
               char  *destination_port_number, 
               char *message_to_be_sent, 
               char *route_rediscovery_flag )
{
    struct sockaddr_un  odraddr;  
    char output_to_sock[MAXLINE];
   
    bzero(&odraddr, sizeof(odraddr)); /* fill in server's address */
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, UNIXDG_PATH);

    sprintf(output_to_sock,"%s|%s|%s|%s\n", destination_canonical_ip_presentation_format, 
                                          destination_port_number,
                                          message_to_be_sent,
                                          route_rediscovery_flag);
    printf("%s\n", output_to_sock);
    sendto(sockfd_for_write,output_to_sock,strlen(output_to_sock),0,&odraddr,sizeof(odraddr));
}

/*
 This function will write this info in a single char format to sockfd_for_write
*/
int msg_recv( int sockfd_for_read,char *message_received,
               char *source_canonical_ip_presentation_format, 
               char  *source_port_number )
{

    char str_from_sock[MAXLINE];
    char *msg_fields[MAXLINE];
    struct sockaddr_un  odraddr;  
    int i=0,j,nready,n;
    socklen_t oddrlen;
    fd_set        rset;

    FD_ZERO(&rset);
    
    for ( ; ; ) 
    {
        FD_SET(sockfd_for_read, &rset);
       
        if ( (nready = select(sockfd_for_read + 1, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR)
              continue;   /* back to for() */
            else
              err_sys("select error");
        }
        if (FD_ISSET(sockfd_for_read, &rset)) 
        {
            
            printf("in FD_ISSET\n");
            oddrlen=sizeof(odraddr);
            if((n=recvfrom(sockfd_for_read,str_from_sock,MAXLINE,0,&odraddr,&oddrlen))>=0)
            {
                printf("MESSAGE RECIEVE :Recieved string :%s\n", str_from_sock);
                msg_fields[0] = strtok(str_from_sock, "|"); //get pointer to first token found and store in 0
                while(msg_fields[i]!= NULL) 
                {   /* ensure a pointer was found */
                    i++;
                    msg_fields[i] = strtok(NULL, "|"); /* continue to tokenize the string */
                }
                
                for(j = 0; j <= i-1; j++) {
                    printf("\nPrinting out all tokens recieved..\n");
                    printf("%s\n", msg_fields[j]); /* print out all of the tokens */
                }
                printf("Error 1\n");
               strcpy(message_received,msg_fields[0]);
                printf("Error 2\n");

               strcpy(source_canonical_ip_presentation_format,msg_fields[1]);
                printf("Error 3----------source_canonical_ip_presentation_format----%s-------\n", source_canonical_ip_presentation_format);

               strcpy(source_port_number, msg_fields[2]);
                 printf("Error 3----------source_port_number--------%s--\n", source_port_number);

               return 1; //port to read from
           }
        }
    
  }
  return -1;    

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
  printf("Address type: ....%ld\n",hptr->h_addrtype);

  switch(hptr->h_addrtype)
  {
    case AF_INET:
    //printf("AF_INET type");

    pptr=hptr->h_addr_list;
    if(pptr!=NULL)
    {
      inet_ntop(hptr->h_addrtype,*pptr,destination_canonical_ip_presentation_format,100);
      printf("Destination canonical IP in presentation format: %s\n", destination_canonical_ip_presentation_format);
      return 1;
    }
    break;

    default:
    fprintf(stderr,"unknown address type\n");
    return -1;
    break;
  }
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