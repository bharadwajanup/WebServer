#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include<string.h>

int create_socket, new_socket,port,n;
struct sockaddr_in address;
struct hostent *server;
char *request_pointer,hostname[256],con_type[2],filename[128];
bool close_connection=true;
char *buffer,file_content[1024],p_file_content[1024];

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
    int bytes_read,content_length,total=0;
    char reply[2000];
    printf("Send request to Fetch the file %s\n",filename);
    buffer = setRequestString(filename);
    printf("Request to be sent:\n%s\n",buffer);
    if((n = send(create_socket,buffer,strlen(buffer),0))<0) //Send the request
    {
        printf("Send Failed\n");
    }
    if (n < 0)
        error("ERROR writing to socket");
    
    bytes_read=0;
    content_length=0;
    do
    {
        bzero(reply,1024);
        if((bytes_read=recv(create_socket,reply,1024,0))<0) //Receive the response
        {
            printf("Problem reading\n");
        }
        printf("%s",reply);
        if(content_length ==0)
        content_length = getContentLength(reply);
        total+=bytes_read;
    }while((total<content_length)); //Make sure you have received the complete response. Comparing with the content length in the header.
    
    if (n < 0)
        error("ERROR reading from socket");
}






int main(int argc, char* argv[])
{
    
    char *p_filename;
    bool isPersistent = false;
    FILE *fp;
    if(argc != 5)//client host port_no p/np file_name
    {
        printf("Invalid number of arguments\nUsage:server_host port con_type file_name\n");
        exit(1);
    }
    else
    {
        strcpy(hostname,argv[1]);
        if(strlen(argv[3])>2)
        {
            printf("Invalid connection type: Has to be either \"p\" or \"np\"\n");
            exit(1);
        }
        strcpy(con_type,argv[3]);
        if(strcmp(con_type,"p")==0)
        {
            isPersistent = true; //Boolean to check if the connection to be made will be persistent or not.
        }
        else
            isPersistent = false;
        strcpy(filename,argv[4]);
        port = atoi(argv[2]);
    }
    create_socket = socket(AF_INET, SOCK_STREAM, 0); //Creates the socket
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
    
    if (connect(create_socket,(struct sockaddr *)&address,sizeof(address)) < 0) //Connect
    {
        perror("ERROR connecting");
        exit(1);
    }
    
    
    if(isPersistent) //Access the filename supplied and retrieve the list of file names to be fetched from the server.
    {
        fp = fopen(filename,"r");
        bzero(p_file_content,1024);
        if(fp == NULL)
        {
            perror("File open:");
            exit(1);
        }
        close_connection=false; //Make the connection persistent
        while ((fgets(p_file_content, 1024, fp)) != NULL )
        {
            getFileContents(strtok(p_file_content,"\n")); //Server does not close its socket till all the files are sent to the client.
        }
    }
    else
    {
        getFileContents(filename); //Send the request to fetch the filename supplied. This will be a non persistent call.
    }
    close(create_socket);
    return 0;
    
}
