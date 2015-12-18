#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include<string.h>
#include<sys/time.h>

int create_socket, new_socket,port,n;
struct sockaddr_in address;
struct hostent *server;
char *request_pointer,hostname[256],con_type[2],filename[128];
bool close_connection=true;
char *buffer,file_content[1024],p_file_content[1024];
socklen_t fromlen;
//Returns the appropriate connection header.
char* addConnectionHeader()
{
    if(close_connection)
    {
        return "Connection: close\n\n";
    }
    else
    {
        return "Connection: keep-alive\n\n";
    }
}

//Generates a request string to be sent to the server
char* setRequestString(char *filename)
{
    char request[1024];
    sprintf(request,"GET /%s HTTP/1.1\nHost: %s:%d\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\nAccept-Language: en-US,en;q=0.5\nAccept-Encoding: gzip, deflate\n",filename,hostname,port);
    
    strcat(request,addConnectionHeader());
    request_pointer = request;
    return request_pointer;
}
//Get the content length from the response header
int getContentLength(char *header)
{
    char conlen[256],len[100];
    strcpy(conlen, strtok(header, "\n"));
    while(strncmp(conlen,"Content-length",14)!=0)
    strcpy(conlen, strtok(NULL, "\n"));
    
    strcpy(len,strtok(conlen, " \t\n"));
    strcpy(len,strtok(NULL, " \t\n"));
    return atoi(len);
    
    
}
//This function send the request to the server and prints the response sent by the server. Takes the filename as the argument to generate the request
void getFileContents(char *filename)
{
    char *terminate = "\\c";
    int byte_counter=0;
    fromlen = sizeof(address);
    int bytes_read,content_length=0,total=0;
    char reply[2000];
    struct timeval start,end;
    printf("Send request to Fetch the file %s\n",filename);
    buffer = setRequestString(filename);
    printf("Request to be sent:\n%s\n",buffer);
    
    n=sendto(create_socket,buffer,strlen(buffer),0,(const struct sockaddr *)&address,fromlen);
    if (n < 0)
        error("ERROR writing to socket");
//     bzero(buffer,1024);
//     n=recvfrom(create_socket,buffer,1024,0,(struct sockaddr *)&address,&fromlen);
//     content_length = getContentLength(buffer);
    do{
        bzero(buffer,1024);
        n=recvfrom(create_socket,buffer,1024,0,(struct sockaddr *)&address,&fromlen);
        //printf("\n%d\n",(int)strlen(buffer));
        
	if (n < 0){
            error("ERROR reading from socket");
        exit(1);}
        
        byte_counter+=(int)strlen(buffer);
        if(strcmp(buffer,terminate)!=0)
	  printf("%s",buffer);
	else
	  break;
	
	
	
        //printf("Last char: %c",buffer[strlen(buffer)]);
    }while(strcmp(buffer,terminate)!=0); //Make sure we receive till EOF
    
    printf("Client received %d bytes of data\n",byte_counter);
}




int main(int argc, char* argv[])
{
    
    char *p_filename;
    struct timeval start,end;
    int time=0;
    if(argc != 4)
    {
        printf("Invalid number of arguments\nUsage:server_host port file_name\n");
        exit(1);
    }
    else
    {
        strcpy(hostname,argv[1]);
        strcpy(filename,argv[3]);
        port = atoi(argv[2]);
    }
    printf("I came here\n");
    create_socket = socket(AF_INET, SOCK_DGRAM, 0); //Creates a datagram socket
    if(create_socket>0)
    printf("Socket has been created\n");
    
    server = gethostbyname(hostname);
    if(server == NULL)
    {
        printf("Invalid server name: %s\n",hostname);
        exit(1);
    }
    //do a bzero here
    
    address.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
    (char *)&address.sin_addr.s_addr,
    server->h_length);
    address.sin_port = htons(port);
    gettimeofday(&start,NULL);
    getFileContents(filename); //Send the requesnt and print the server response
    gettimeofday(&end,NULL);
    time = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
    printf("Time taken to receive data is: %.2f seconds\n",((double)time/1000000));
    close(create_socket);
    return 0;
    
    
}
