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
#include<pthread.h>
#define BUF_SIZE 1024
int create_socket, new_socket;
socklen_t addrlen;
char buffer[BUF_SIZE], port[6], *cur_dir;
struct sockaddr_in address;
struct request_head //Structure to parse the contents of the request header
{
    char method[64];
    char protocol[128];
    char filename[128];
    char connection_type[256];
};
bool close_socket=true;

//Function to create and bind the socket to a port
void setupSocket()
{
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
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
    char con_type[64];
    strcpy(con_type,strtok(header.connection_type, " \t\n"));
    strcpy(con_type,strtok(NULL, " \t\n"));
    if(strncmp(con_type,"keep-alive",10)==0)
    {
        close_socket= false;
    }
    else
    {
        close_socket=true;
    }
    return header;
}



void *sendResponse()
{
    char *pch, file_path[500], pathname[500],file_content[1024],response_header[1024];
    int file_read,read_size;
    FILE *fd;
    struct request_head header;
    struct stat st;
    bzero(buffer,1024);
    while((read_size = recv(new_socket, buffer, BUF_SIZE, 0))>0)
    {
        printf("Request received:\n%s\n", buffer); //Print the contents of the request.
        header = getHeaderStructure(buffer);
        strcpy(pathname, header.filename);
        if(strncmp(header.protocol, "HTTP/1.1",8) !=0 && strncmp(header.protocol, "HTTP/1.0",8)!=0)
        {
		printf("HTTP/1.1 400 Bad Request\n\n"); //Implement Error 400 here.
		sprintf(response_header,"HTTP/1.1 400 Bad Request\nConnection: close\nContent-length: %d\n\n",60);
                printf("Response header:\n%s\n",response_header); //Build the response header
		send(new_socket, response_header,strlen(response_header),0); //response_header now contains both header and content. Send it to the client.
                
        }
        else
        {
            if(strncmp(pathname,"/\0" ,2)==0)//Try to find index.html if the request contains no filename.
            {
                strcpy(pathname,"index.html");
            }
            strcpy(file_path, strtok(pathname,"/"));//Remove the '/'
            printf("File to be fetched is: %s\n",file_path);
            if((fd = fopen(file_path , "r"))!=NULL) //Open the file. Returns null if file is not present
            {
		stat(file_path,&st);
		
		printf("File size: %d\n",(int)st.st_size);
                bzero(file_content,1024);
		sprintf(response_header,"HTTP/1.1 200 OK\nConnection: keep-alive\nContent-length: %d\nContent-Type: text/html\n\n",((int)st.st_size+84));
                printf("Response header:\n%s\n",response_header); //Build the response header
		send(new_socket, response_header,strlen(response_header),0); //response_header now contains both header and content. Send it to the client.
                
                while ((fgets(buffer, 1024, fd)) != NULL )
                {
                    send(new_socket, buffer,strlen(buffer),0); //response_header now contains both header and content. Send it to the client.
		    //strcat(file_content,buffer);
                }
                //strcat(response_header, file_content);
                
                //printf("Response:\n%s\n",response_header);
                fclose(fd);
            }
            else
            {
                strcpy(file_path , "not_found.html"); //File not found. Respond with appropriate message
                fd = fopen(file_path,"r"); //Open not_found.html
                bzero(file_content,1024);
                while ((fgets(buffer, 1024, fd)) != NULL )
                {
                    strcat(file_content,buffer);
                }
                sprintf(response_header,"HTTP/1.1 404 Not Found\nConnection: keep-alive\nContent-length: %d\nContent-Type: text/html\n\n",((int)strlen(file_content)+91));
                printf("Response header:\n%s\n",response_header); //sets the 404 header.
                send(new_socket, response_header,91,0);
                printf("Response:\n%s\n",file_content);
                send(new_socket, file_content, strlen(file_content),0);
                fclose(fd);
            }
        }
        //}
        
    }
    if(read_size == 0)
    {
        printf("Closing the socket\n"); //Close the socket as the client has finished sending requests. Comes here every time for persistent connections.
        close(new_socket);
    }
    else
    {
        printf("error in receive\n");
    }
}



int main(int argc, char* argv[]) {
    
    pthread_t threads[100];
    int thread_no=0,iret;
    if(argc>1)
    {
        strcpy(port, argv[1]); //look for command line arguments
    }
    else
        strcpy(port, "10000"); //else use a default 10000 port
    
    cur_dir = getenv("PWD");
    setupSocket();
    
    while (1) {
        if (listen(create_socket, 10) < 0) { //Listen for connections
            perror("server: listen");
            exit(1);
        }
        printf("Done Listening\n");
        printf("before accept\n");
        if ((new_socket = accept(create_socket, (struct sockaddr *) &address, &addrlen)) < 0) {
            perror("server: accept");
            exit(1);
        }
        printf("After accept\n");
        if (new_socket > 0){
            printf("A Client is connected...\n"); //Client accepted
        }
        thread_no++;
        thread_no = thread_no%100;
        iret = pthread_create(&threads[thread_no],NULL,sendResponse,NULL);//Assign a thread to handle the request
	
	printf("I'm still running\n");
	
	//pthread_join(threads[thread_no],NULL);
    }
    close(create_socket);
    return 0;
}
