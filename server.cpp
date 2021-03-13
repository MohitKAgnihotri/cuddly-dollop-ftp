#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include <fstream>
#include <direct.h>
#include "Thread.h"
#include "server.h"

TcpServer::TcpServer()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202,&wsadata)!=0)
		TcpThread::err_sys("Starting WSAStartup() error\n");
	
	//Display name of local host
	if(gethostname(servername,HOSTNAME_LENGTH)!=0) //get the hostname
		TcpThread::err_sys("Get the host name error,exit");
	
	printf("Server: %s waiting to be contacted for time/size request...\n",servername);
	
	
	//Create the server socket
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		TcpThread::err_sys("Create socket error,exit");
	
	//Fill-in Server Port and Address info.
	ServerPort=REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */
	
	//Bind the server socket
    if (bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
		TcpThread::err_sys("Bind socket error,exit");
	
	//Successfull bind, now listen for Server requests.
	if (listen(serverSock, MAXPENDING) < 0)
		TcpThread::err_sys("Listen socket error,exit");
}

TcpServer::~TcpServer()
{
	WSACleanup();
}


void TcpServer::start()
{
	for (;;) /* Run forever */
	{
		/* Set the size of the result-value parameter */
		clientLen = sizeof(ClientAddr);
		
		/* Wait for a Server to connect */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &ClientAddr, 
			&clientLen)) < 0)
			TcpThread::err_sys("Accept Failed ,exit");
		
        /* Create a Thread for this new connection and run*/
		TcpThread * pt=new TcpThread(clientSock);
		pt->start();
	}
}

//////////////////////////////TcpThread Class //////////////////////////////////////////
void TcpThread::err_sys(const char * fmt,...)
{     
	perror(NULL);
	va_list args;
	va_start(args,fmt);
	fprintf(stderr,"error: ");
	vfprintf(stderr,fmt,args);
	fprintf(stderr,"\n");
	va_end(args);
	exit(1);
}
unsigned long TcpThread::ResolveName(char name[])
{
	struct hostent *host;            /* Structure containing host information */
	
	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");
	
	/* Return the binary, network byte ordered address */
	return *((unsigned long *) host->h_addr_list[0]);
}

#if 1
/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpThread::msg_recv(int sock,Msg * msg_ptr)
{
	int rbytes,n;
	
	for(rbytes=0;rbytes<MSGHDRSIZE;rbytes+=n)
		if((n=recv(sock,(char *)msg_ptr+rbytes,MSGHDRSIZE-rbytes,0))<=0)
			err_sys("Recv MSGHDR Error");
		
		for(rbytes=0;rbytes<msg_ptr->length;rbytes+=n)
			if((n=recv(sock,(char *)msg_ptr->buffer+rbytes,msg_ptr->length-rbytes,0))<=0)
				err_sys( "Recevier Buffer Error");
			
			return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/
int TcpThread::msg_send(int sock,Msg * msg_ptr)
{
	int n;
	if((n=send(sock,(char *)msg_ptr,MSGHDRSIZE+msg_ptr->length,0))!=(MSGHDRSIZE+msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n-MSGHDRSIZE);
	
}
#else
int TcpThread::msg_recv(int sock, Msg* msg_ptr)
{
	int n;

	n = recv(sock, (char*)msg_ptr->buffer, BUFFER_LENGTH, 0);
	if (n<=0)
		err_sys("Recevier Buffer Error");

	return n;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/
int TcpThread::msg_send(int sock, Msg* msg_ptr)
{
	int n;
	n = send(sock, (char*)msg_ptr, msg_ptr->length, 0);
	if (n<=0)
		err_sys("Send Buffer Error");
	return (n);

}
#endif

void TcpThread::run() //cs: Server socket
{
	Resp * respp;//a pointer to response
	Req * reqp; //a pointer to the Request Packet
	Msg rpdu,spdu; //send_message receive_message
	struct _stat stat_buf;
    int result;
	int start_byte = 0;
	int file_download_len = 0;
	int bytes_to_read = 0;
	std::fstream file_get;
	std::fstream file_put;

	memset(&rpdu, 0x00, sizeof(rpdu));
		
	msg_recv(cs,&rpdu); // Receive a message from the client
	
	switch (rpdu.type)
	{
		/* Download request */
	case GET:
		start_byte = rpdu.file_read_offset;
		file_download_len = rpdu.file_read_length;
		file_get.open(rpdu.buffer, std::fstream::in | std::fstream::out);
		if (!file_get.is_open())
		{
			spdu.type = DATA; // data unit type as error
			sprintf(spdu.buffer, "Error: File \"%s\" not found.\n", rpdu.buffer);
			spdu.length = strlen(spdu.buffer);
			msg_send(cs, &spdu);
			fprintf(stderr, "Error: File \"%s\" not found.\n", rpdu.buffer);
		}
		else
		{
			int total_bytes_to_be_sent = 0;
			spdu.type = DATA;
			
			//Calculate the file size.
			std::streampos begin = file_get.tellg();
			file_get.seekg(0, std::ios::end);
			std::streampos end = file_get.tellg();
			bytes_to_read = end - begin;

			//reset the file pointer to the start
			file_get.seekg(0, std::ios::beg);
			
			total_bytes_to_be_sent = bytes_to_read;
			while (bytes_to_read > 0)
			{
				memset(&spdu, 0x00, sizeof(spdu));
				file_get.read(spdu.buffer, BUFFER_LENGTH);
				spdu.length = file_get.gcount();
				bytes_to_read -= spdu.length;
				spdu.type = DATA;
				msg_send(cs, &spdu);
			}
		}
		file_get.close();
		closesocket(cs); // close the connection
		break;

		/* Upload request */
	case PUT:
		memset(&spdu, 0x00, sizeof(spdu));
		spdu.type = DATA;
		spdu.length = 0;
		msg_send(cs, &spdu); // tell client ready
		file_put.open("server"+ std::string(rpdu.buffer), std::fstream::out);
		if (!file_put.is_open())
			err_sys("failed to open the file");

		printf("Starting transfer of \"%s\".\n", rpdu.buffer);
		do {
			memset(&rpdu, 0x00, sizeof(rpdu));
			int bytes_read = msg_recv(cs, &rpdu);
			if (bytes_read < 0) {
				perror("Error: Read");
				closesocket(cs);
				break;
			}
			if (bytes_read == 0) {
				printf("Socket probably closed by the client\n");
				closesocket(cs);
				break;
			}
			file_put.write(rpdu.buffer, rpdu.length); // write data to file
		} while (rpdu.length == BUFFER_LENGTH);
		file_put.close();
		char buff[FILENAME_MAX]; //create string buffer to hold path
		_getcwd(buff, FILENAME_MAX);
		std::string current_working_dir(buff);
		std::cout << current_working_dir;
		printf("Transfer sucessful.\n");
		break;
	}

	closesocket(cs);
	
}



////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	
	TcpServer ts;
	ts.start();
	
	return 0;
}


