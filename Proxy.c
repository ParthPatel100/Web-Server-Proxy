/*
Parth Patel
UCID : 30096473
CPSC 441 Assignment 1
*/

/*
To Run this program first type <gcc -pthread A1.c -o A1> then <./A1> 
To Run the Config/Filter port telnet to the IP_ADDR(defined below) and the Port number = PORT_NUM(defined below) + 1 (so it will be 2589)
To Block a page, type BLOCK <Keyword> and to Unblock type UNBLOCK <Keyword>

During my testing of the test web pages, the loading, blocking and unblocking of pages worked as expected
For large files, like the image files, since I am transferring 1 byte at a time, the pictures take slightly more time to fully load 
*/

#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <netdb.h>
#include <stdlib.h>
#include <pthread.h>

#define PORT_NUM 2588
#define IP_ADDR "136.159.5.27"
#define MAX_BLOCK_PAGES 5

char blockedList[MAX_BLOCK_PAGES][30];
int blockedPages;

//Function used by the filter/config port to add the word to the list of blocked pages
void blockPage(char *word){
	int j= 0;

	//check if the number of blockedPages has reached the maximum
	if (blockedPages == MAX_BLOCK_PAGES){
		printf("Sorry, reached Maximum blocking limit\n");
		for(int i = 0; i < 5; i++){
			printf("Blocked : %s\n", blockedList[i]);
		}	
		return;
	}

	//Add the keyword to the blocked list if the blocked list is empty
	for(int i = 0; i < 5; i++){
		if(strcmp(blockedList[i], word) == 0){
			break;
		}
		else if (strcmp(blockedList[i], "") == 0){
			strcpy(blockedList[i], word);
			blockedPages++;
			break;
		}
	}

	//print all the blocked pages
	for(int i = 0; i < 5; i++){
			printf("Blocked : %s\n", blockedList[i]);
	}	
	printf("\n");
}

//Function to unblock a certain URL containing the unblocked keyword 
void unblockPage(char *word){
	int j = 0;
	//Look for the word and remove it from the blocked list and decrement the blockedPages
	for(int i = 0; i < 5; i++){
		if(strcmp(blockedList[i], word) == 0){
			strcpy(blockedList[i], "");
			blockedPages--;
			break;
		}

		//else if the keyword is not in the list, print out the message indicating the keyword is not in the list
		else if(i == 4){
			printf("Not in the list\n");
		}
	}
	printf("\n");

	//Print all the blocked pages
	for(int i = 0; i < 5; i++){
		printf("Blocked : %s\n", blockedList[i]);
	}
}

//Thread function to receive tthe BLOCK and UNBLOCK request
void* getFilter(){
	int filter_sock, server_sock;
	struct sockaddr_in server;
	char filter_request[100];
	int recvStatus;

	//create a filter socket
	filter_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(filter_sock == -1){ 
		printf("Could not create config socket\n");
	}
	printf("Config Socket created\n");

	//This helped me re-use the address and the port getting rif of the address already in use error
    if (setsockopt(filter_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

	//Create a server with the config address and port different from proxy port
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(IP_ADDR);
	server.sin_port = htons(PORT_NUM + 1);

	//bind the socket to the server
	int bindStatus = bind(filter_sock, (struct sockaddr *)&server, sizeof(server));
	if( bindStatus == -1){
		perror("config Binding failed");
		exit(1);
	}
	printf("config Binding done.\n");

	listen(filter_sock, 3);
	
	printf("Waiting for config client...\n");

	//If the config gets an unidentified request, it sends a message back to the config port indicating how to BLOCK and UNBLOCK
	char unrecog[200];
	strcpy(unrecog, "Server replied: --Could not process request, please try again--\n--To Block a page, type: BLOCK <Keyword> and to Unblock, type: UNBLOCK <Keyword>--\n");

	//Get the request, identify the type of request and attempt to execute it
	while(1){
		server_sock = accept(filter_sock, NULL, NULL);
		if (server_sock < 0){
			perror("Connection failed");
			exit(1);
		}

		//Recieve the message sent through the config port
		recvStatus = recv(server_sock, filter_request, 100, 0);
		while(recvStatus > 0){
			char webPage[100];
			if (recvStatus > 0){
				printf("Config server sent: \n%s", filter_request);
			}
			char keyWord[30];
			//Check if the request was block and parse the keyword from it
			if(sscanf(filter_request, "BLOCK %s\n", keyWord)){
				printf("Block Word: %s, len: %d\n", keyWord, strlen(keyWord));
				blockPage(keyWord);
			}	
			//check if the request was to unblock and extract the keyword from it
			else if (sscanf(filter_request, "UNBLOCK %s\n", keyWord)){
				unblockPage(keyWord);
			}

			//otherwise send a message instructing how to block or unblock
			else{
				send(server_sock, unrecog, strlen(unrecog), 0);
			}
			memset(webPage, 0, sizeof(webPage));
			memset(filter_request, 0, sizeof(filter_request));
			recvStatus = recv(server_sock, filter_request, 100, 0);
		}

	}
	

}
//Block the URL by changing and sending a different URL that leads to the error.html page
void block(char *firstReq, char *URL, char *HOST){
	sprintf(URL, "pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/error.html");
	sprintf(firstReq, "GET http://%s HTTP/1.1\r\nHost: %s\r\n\r\n", URL,HOST);
	printf("---------------BLOCKED-------------!!!\n");
}

//Check if the URL needs to be blocked by searching for the keyword in the URL
int checkIfBlock(char *URL){
    char page[100];
    for(int i = 0; i < 5; i++){
        strcpy(page, blockedList[i]);
        if(strlen(blockedList[i]) != 0 && strlen(page) != 0 && strstr(URL, page) != NULL){
			return 1;
        }
    }
	return 0;
}

//Function to connect to the web and send the request to the client
void* connectToWeb(){
	int proxy_sock, server_sock, pclient_sock;
	struct sockaddr_in server, client, pclient, pserver, webServer;
	char client_message[5000];

	//Proxy socket
	proxy_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (proxy_sock == -1)
	{
		printf("Could not create socket\n");
	}
	puts("Socket created\n");

	//This helped me re-use the address and the port getting rif of the address already in use error
	if (setsockopt(proxy_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

	//webserver socket
	server_sock = socket(AF_INET, SOCK_STREAM, 0);

	//Prepare the sockaddr_in structure pserver
	pserver.sin_family = AF_INET;
	pserver.sin_addr.s_addr = inet_addr(IP_ADDR);
	pserver.sin_port = htons(PORT_NUM);

	//Bind
    int bindStatus = bind(proxy_sock, (struct sockaddr *)&pserver, sizeof(pserver));
	if( bindStatus == -1){
		//print the error message
		perror("Binding failed");
		exit(1);
	}
	printf("Binding done.\n");
	

	
	//Listen
	listen(proxy_sock , 3);
	
	//Accept and incoming connection
	printf("Waiting for clients...\n");
	
	//accept connection from an incoming client
	
	//Receive a message from client
    int recvStatus;
    struct hostent *hp;
	char hostname[1000];
	int blocked = 0;

	//Get requests from browser and send the response back to the web browser
	while(1){
		server_sock = accept(proxy_sock, NULL, NULL);
		if (server_sock < 0){
			perror("Connection failed");
			exit(1);
		}
		//Keep on receiving the web browser requests and check if the URL needs to be blocked
		while(recvStatus = recv(server_sock, client_message, 5000, 0) > 0){

			if (recvStatus > 0){
				//Ignore any POST requests
				if(client_message[0] == 'P' && client_message[1] == 'O' && client_message[2] == 'S' && client_message[3] == 'T'){
					break;
				}

				//Parse requests from the GET requests
				else if (client_message[0] == 'G' && client_message[1] == 'E' && client_message[2] == 'T') {
					char *pathname = strtok(client_message, "\r\n");
					
					//Extrace the URL and the Host from the URL
					printf("Found HTTP request: %s\n", pathname);
					char URL[100], HOST[100];
					if(sscanf(pathname, "GET http://%s", URL) == 1)
					{
						printf("URL = %s\n", URL);

					}
					
					for (int i = 0; i < strlen(URL); i++)
					{
						if(URL[i] == '/')
						{
							strncpy(HOST, URL, i);
							HOST[i] = '\0';
							break;
						}
					}

					//Obtain the ip address from the host name
					printf("HOST: %s\n", HOST);
					hp = gethostbyname(HOST);
					if(hp == NULL){
						fprintf(stderr, "%s: unknown host\n", hostname);
						exit(1);
					}
					//copy IP address so you don't need hostname resolver
					//Set the webServer address = ip extracted from host name
					bcopy(hp->h_addr, &webServer.sin_addr.s_addr, hp -> h_length);
					pclient_sock = socket(AF_INET, SOCK_STREAM, 0);
					webServer.sin_family = AF_INET;
					webServer.sin_port = htons(80);

					//Connec pclient_sock with the webServer
					if (connect(pclient_sock , (struct sockaddr *)&webServer , sizeof(webServer)) < 0)
					{
                        perror("Sorry, connect failed. Error");
                        exit(1);
					}
					else{
                        printf("Connected \n");
                        char firstReq[500];
                        sprintf(firstReq, "GET http://%s HTTP/1.1\r\nHost: %s\r\n\r\n", URL,HOST);
                        printf("%s", firstReq);
                        printf("URL:%s\n", URL);
					
					//Print all the blocked pages
					for(int i = 0 ; i < 5; i++){
						printf("Blocked: %s\n", blockedList[i]);
					}
					
					//Check if there are any blocked keywords in the URL
                    if(blocked = checkIfBlock(URL)){
                        block(firstReq, URL, HOST);
                    }
                    					
					printf("Got request for: %s\n", URL);
					
					//Send the GET request to the webServer
					if (send(pclient_sock, firstReq, strlen(firstReq), 0) > 0){
						printf("Successfully sent request!\n");
					}
					
					//Receive the first 1024 bytes of information from the web page and send it to the web browser
					int sent = 0;
					int ret = 0;
					int totalReceived = 0;
					int contentLength = 0;
					char receivedMessage[1024];
                    
					ret = recv(pclient_sock, receivedMessage, 1024, 0);
					send(server_sock, receivedMessage, 1024, 0);

					//Clear the receivedMessage array
					memset(receivedMessage, 0 , sizeof(receivedMessage));

					//Receive 1 byte at a time for a web page
					//I used 1 byte as for bytes > 100, when loading the image, I got messages like "cannot load the image because it contains errors"
					char receivedPage[1];

					//Check if all the bytes were received
					if(ret == 1024){

						//while all the bytes havent been received, receive and send those bytes to the web browser from the webserver
						while(ret > 0){
							ret = recv(pclient_sock, receivedPage, 1, 0);
							if(ret < 0){
								printf("Failed\n");
							}	
							sent = send(server_sock, receivedPage, 1, 0);
							if (sent < 0){
								printf("Failed\n");
							}
							memset(receivedPage, 0 , sizeof(receivedPage));

						}						
					}


					}
				}

			}
		}
	}
	
}

int main(int argc, char *argv[]){
	//Pthread for the connection between web browser to the proxy and the web server/page
	pthread_t webThread;

	//Pthread to connect to the config/filter port to obtain the BLOCK and UNBLOCK request
	pthread_t filterThread;

	//Create both threads 
	int status = pthread_create(&webThread, NULL, connectToWeb, NULL);
	int status2 = pthread_create(&filterThread, NULL, getFilter, NULL);

	//Wait for both the threads to complete their tasks
	pthread_join(webThread, NULL);
	pthread_join(filterThread, NULL);
	return 0;
}