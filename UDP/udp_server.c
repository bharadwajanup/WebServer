#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<stdbool.h>
#define BUF_SIZE 1024

int create_socket, new_socket;
socklen_t fromlen;
char buffer[BUF_SIZE], port[6], *cur_dir;
struct sockaddr_in address;
int n;
struct request_head //Structure to parse the contents of the request header
{
    char method[64];
    char protocol[128];
    char filename[128];
    char connection_type[256];
};
bool close_socket=true;

void setupSocket()//Function to create and bind the socket to a port
{
    if ((create_socket = socket(AF_INET, SOCK_DGRAM, 0)) > 0){
        printf("The socket was created\n");
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(port));
    
    if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
        printf("Binding Socket at port %d\n" ,atoi(port));
    }
    else
    {
        perror("server:bind");
        exit(1);
    }
    
    
}

//Takes the request and returns a structure with header information on it
struct request_head getHeaderStructure(char *request)
{
    struct request_head header;
    strcpy(header.method, strtok(request, " \t\n"));
    strcpy(header.filename, strtok(NULL, " \t\n"));
    strcpy(header.protocol, strtok(NULL, " \t\n"));
    
    strcpy(header.connection_type, strtok(NULL, "\n"));
    while(strncmp(header.connection_type,"Connection",10)!=0)
    strcpy(header.connection_type, strtok(NULL, "\n"));
    return header;
}
//The heart of the program. Processes the request and sends the appropriate response to the client.
void sendResponse()
{
    char *pch, file_path[500], pathname[500],file_content[1024],response_header[1024];
    int file_read,read_size,n,packets_sent=0;
    FILE *fd;
    struct request_head header;
    struct stat st;
    header = getHeaderStructure(buffer);

    //if(strcmp(header.method, "GET\0") ==0)
    // {
    strcpy(pathname, header.filename);
    if(strncmp(header.protocol, "HTTP/1.1",8) !=0 && strncmp(header.protocol, "HTTP/1.0",8)!=0)
    {
         printf("HTTP/1.1 400 Bad Request\n\n");
	  sprintf(response_header,"HTTP/1.1 400 Bad Request\nConnection: close\nContent-length: %d\n\n",60);
          printf("Response header:\n%s\n",response_header); //Build the response header
	   n=sendto(create_socket,response_header,1024,0,(struct sockaddr *)&address,fromlen);
    }
    else
    {
        if(strncmp(pathname,"/\0" ,2)==0)//Try to find index.html if the request contains no filename.
        {
            strcpy(pathname,"index.html");
        }
        strcpy(file_path, strtok(pathname,"/"));
        printf("Finally, the file to be fetched is: %s\n",file_path);
        if((fd = fopen(file_path , "r"))!=NULL)
        {
            bzero(file_content,1024);
	    stat(file_path,&st);
		
	    printf("File size: %d\n",(int)st.st_size);
	    
	     sprintf(response_header,"HTTP/1.1 200 OK\nConnection: keep-alive\nContent-length: %d\nContent-Type: text/html\n\n",((int)st.st_size+84));
            printf("Response header:\n%s\n",response_header);
           
	    n=sendto(create_socket,response_header,1024,0,(struct sockaddr *)&address,fromlen);
	    bzero(buffer,1024);
	    printf("Content:\n");
            while ((fgets(buffer, 1024, fd)) != NULL )
            {
                //strcat(file_content,buffer);
                n=sendto(create_socket,buffer,1024,0,(struct sockaddr *)&address,fromlen);
		printf("%s",buffer);
            }
            bzero(buffer,1024);
	    strcpy(buffer,"\\c");
	    n=sendto(create_socket,buffer,1024,0,(struct sockaddr *)&address,fromlen);
            fclose(fd);
        }
        else //Send 404 error
        {
            strcpy(file_path , "not_found.html");
            fd = fopen(file_path,"r");
            bzero(file_content,1024);
            while ((fgets(buffer, 1024, fd)) != NULL )
            {
                strcat(file_content,buffer);
            }
            sprintf(response_header,"HTTP/1.0 404 Not Found\nConnection: keep-alive\nContent-length: %d\nContent-Type: text/html\n\n",((int)strlen(file_content)+91));
            printf("Response header:\n%s\n",response_header);
            strcat(response_header, file_content);
            n=sendto(create_socket,response_header,1024,0,(struct sockaddr *)&address,fromlen);
            if(n<0)
            {
                perror("Sendto:");
                exit(1);
            }
            bzero(buffer,1024);
	    strcpy(buffer,"\\c");
	    n=sendto(create_socket,buffer,1024,0,(struct sockaddr *)&address,fromlen);
            fclose(fd);
        }
    }
    //}
    
}


int main(int argc, char* argv[]) {
    
    
    if(argc>1)
    {
        strcpy(port, argv[1]); //look for command line arguments
    }
    else
        strcpy(port, "10000"); //else use a default 10000 port
    
    cur_dir = getenv("PWD"); //Using relative path. Not really needed.
    setupSocket();
    fromlen = sizeof(address);
    while (1) {
        printf("Waiting to receive\n");
        n = recvfrom(create_socket,buffer,1024,0,(struct sockaddr *)&address,&fromlen);
        printf("buffer: %s\n",buffer);
        if (n < 0)
        {
            perror("recvfrom");
            exit(1);
        }
        sendResponse();
    }
    close(create_socket);
    return 0;
}
