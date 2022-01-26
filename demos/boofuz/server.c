#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/**

	This program implements a simple protocol for writing data to a file or reading it from a file as follows
	First Packet: 							<Username> + '\n'
	Second Packet:							r| or w| + <FileName> + '\n'	
	Third Packet if w method is used:  		<fileContents>

	If read method is used client will receive a 512 byte len packet containing the file.
	All comunication is ended by an b'End' message if no internal error has occured

*/

/**
	Function used to read untill '\n' from socket
*/
int recvln(int conn, char *buff, int buffsz)
{
	char *bp = buff;
	char c;
	int	n;

	while(bp - buff < buffsz && 
	      (n = recv(conn, bp, 1, 0)) > 0) {
		if (*bp++ == '\n')
			return (bp - buff);
	}
	if (n < 0)
		return -1;

	if (bp - buff == buffsz)
		while (recv(conn, &c, 1, 0) > 0 && c != '\n');

	return (bp - buff);
}


int main(int argc, char *argv[]) {

	int SERVER_PORT = 40000;

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	server_address.sin_port = htons(SERVER_PORT);

	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	int listen_sock;
	if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("could not create listen socket\n");
		return 1;
	}

	if ((bind(listen_sock, (struct sockaddr *)&server_address,
	          sizeof(server_address))) < 0) {
		printf("could not bind socket\n");
		return 1;
	}

	int wait_size = 16;  // Maximum number of waiting clients

	if (listen(listen_sock, wait_size) < 0) {
		printf("could not open socket for listening\n");
		return 1;
	}

	// Scoket for clients
	struct sockaddr_in client_address;
	int client_address_len = 0;

	// Run forever
	while (true) {
		// Open a new socket to transmit data per connection
		int sock;
		if ((sock =
		         accept(listen_sock, (struct sockaddr *)&client_address,
		                &client_address_len)) < 0) {
			printf("could not open a socket to accept data\n");
			return 1;
		}

		int n = 0;
		int len = 0;
		int maxlen = 100; // Buffer is of len 100, so easy to break

		char* fileBuffer = malloc(sizeof(char) * 512);
		char* realFileBuffer = calloc(512,1);
		strcat(realFileBuffer, "files/");
		
		char nameBuffer[maxlen];
		char fileNameBuffer[maxlen]; 

		printf("client connected with ip address: %s\n",
		       inet_ntoa(client_address.sin_addr));

		n = recvln(sock, &nameBuffer, maxlen);

		nameBuffer[n - 1] = 0; //escape thea new line
		printf("client is %s\n", nameBuffer);
	
		n = recvln(sock, &fileNameBuffer, maxlen);

		fileNameBuffer[n - 1] = 0; //escape thea new line
		strcat(realFileBuffer, fileNameBuffer + 2);

		FILE* filePTR = NULL;
		
		printf("command is %s\n", fileNameBuffer);

		if(fileNameBuffer[0] == 'r') {
			filePTR = fopen(realFileBuffer, "rb");

			if(filePTR != NULL) {

				printf("sending %s contents\n", fileNameBuffer + 2);

				fseek(filePTR, 0, SEEK_END);
				long fsize = ftell(filePTR);
				fseek(filePTR, 0, SEEK_SET); 
				fread(fileBuffer, fsize, 1, filePTR);

				fclose(filePTR);
				
				fileBuffer[fsize] = 0; 

				send(sock, fileBuffer, fsize, 0);

			}
			send(sock, "End", 3, 0); 
		}

		if(fileNameBuffer[0] == 'w') {

			filePTR = fopen(realFileBuffer, "wb");
			n = recv(sock, fileBuffer, 512, 0);

			if(filePTR != NULL) {

				printf("writing to %s\n", fileNameBuffer + 2);
				fwrite(fileBuffer, 512, 1, filePTR);

				fclose(filePTR);
			}

			send(sock, "End", 3, 0);
		} else{
			send(sock, "End", 3, 0);
		}

		free(fileBuffer);
		close(sock);
	}

	close(listen_sock);
	return 0;
}
