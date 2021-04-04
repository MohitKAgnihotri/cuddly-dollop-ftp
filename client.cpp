#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <winsock2.h>
#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <windows.h>
#include <fstream>

#include <direct.h>

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 2345
#define BUFFER_LENGTH 1024
#define TRACE 0
#define MSGHDRSIZE 16 //Message Header Size

typedef enum
{
	GET = 1 << 0,
	PUT = 1 << 1,
	GETALL = 1 << 2,
	PUTALL = 1 << 3,
	DATA_SUCCESS = 1 << 4,
	DATA_ERROR = 1 << 5,
	END = 1 << 6
} command;

typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req; //request

typedef struct
{
	char response[RESP_LENGTH];
} Resp; //response

typedef struct
{
	command type;
	int length; //length of effective bytes in the buffer
	int file_read_offset;
	int file_read_length;
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


class TcpClient
{
	int sock; /* Socket descriptor */
	struct sockaddr_in ServAddr; /* server socket address */
	unsigned short ServPort; /* server port */
	Req* reqp; /* pointer to request */
	Resp* respp; /* pointer to response*/
	Msg tpdu, rpdu; /* Transmit and Receive message */
	WSADATA wsadata;
	void create_get_message(Msg&, char const* filename);
	void create_put_message(Msg&, char const* filename);
	void create_getall_message(Msg&);
	void create_putall_message(Msg&);
	bool getfile(const char* filename);
	bool putfile(const char* filename);
	std::string getFileName();
public:
	static BOOL fileExists(TCHAR* file);

	TcpClient()
	{
	}

	void run(int argc, char* argv[]);
	~TcpClient();
	int msg_recv(int, Msg*);
	int msg_send(int, Msg*);
	unsigned long ResolveName(char name[]);
	void err_sys(const char* fmt, ...);
};

using namespace std;

void TcpClient::create_get_message(Msg& pdu, char const* filename)
{
	memset(&pdu, 0x00, sizeof(Msg));
	pdu.type = GET;
	pdu.file_read_offset = 0;
	pdu.file_read_length = 0;
	strncpy(pdu.buffer, filename, strlen(filename));
	pdu.length = strlen(tpdu.buffer);
	pdu.buffer[tpdu.length + 1] = '\0';
}

void TcpClient::create_getall_message(Msg& pdu)
{
	memset(&pdu, 0x00, sizeof(Msg));
	pdu.type = GETALL;
}

void TcpClient::create_put_message(Msg& pdu, char const* filename)
{
	memset(&pdu, 0x00, sizeof(Msg));
	pdu.type = PUT;
	pdu.file_read_offset = 0;
	pdu.file_read_length = 0;
	strncpy(pdu.buffer, filename, strlen(filename));
	pdu.length = strlen(pdu.buffer);
	pdu.buffer[pdu.length + 1] = '\0';
}

void TcpClient::create_putall_message(Msg& pdu)
{
	memset(&pdu, 0x00, sizeof(Msg));
	pdu.type = PUTALL;
}


BOOL TcpClient::fileExists(TCHAR* file)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE handle = FindFirstFile(file, &FindFileData);
	int found = handle != INVALID_HANDLE_VALUE;
	if (found)
	{
		//FindClose(&handle); this will crash
		FindClose(handle);
	}
	return found;
}


bool TcpClient::getfile(const char* filename)
{
	bool isError = false;
	std::fstream file_get;
	// Open the file in the write mode.
	file_get.open(filename, std::fstream::out);
	if (!file_get.is_open())
		this->err_sys("Filed to open the file for writing");

	file_get.write(rpdu.buffer, rpdu.length);
	while (rpdu.length == BUFFER_LENGTH)
	{
		// if there is more data to write
		memset(&rpdu, 0x00, sizeof(rpdu));
		int bytes_read = msg_recv(sock, &rpdu);
		if (bytes_read < 0)
		{
			err_sys("Error: Read");
			closesocket(sock);
			break;
		}
		if (bytes_read == 0)
		{
			err_sys("Socket probably closed by the client");
			closesocket(sock);
			break;
		}
		if (rpdu.type == DATA_SUCCESS)
			file_get.write(rpdu.buffer, rpdu.length);
		else if (rpdu.type == DATA_ERROR)
		{
			printf("Received Failure Response from the server : [%s]\n", rpdu.buffer);
			isError = true;
			break;
		}
		else
		{
			printf("Unexpected Message from the server: MessageType = [%d], Message content = [%s]\n", rpdu.type,
			       rpdu.buffer);
			isError = true;
			break;
		}
	}
	file_get.close();
	if (!isError)
	cout << "Transfer sucessful." << endl;
	return isError;
}

int getfileSize(std::fstream &file)
{
	streampos begin, end;
	unsigned int bytes_to_read;

	//Calculate the file size.
	begin = file.tellg();
	file.seekg(0, ios::end);
	end = file.tellg();
	bytes_to_read = end - begin;

	//Reset the file stream pointer to the start.
	file.seekg(0, ios::beg);
	return bytes_to_read;
}

bool TcpClient::putfile(const char* filename)
{
	std::fstream file_put;
	file_put.open(filename, ios::in | ios::binary);
	if (!file_put.is_open())
		err_sys("Failed to open the file for reading");

	int bytes_to_read = getfileSize(file_put);

	/*Send the file */
	tpdu.type = DATA_SUCCESS;
	while (bytes_to_read > 0)
	{
		// send data packets
		file_put.read(tpdu.buffer, BUFFER_LENGTH);
		if (!file_put)
			tpdu.length = file_put.gcount();
		else
			tpdu.length = BUFFER_LENGTH;
		bytes_to_read -= tpdu.length;
		msg_send(sock, &tpdu);
	}
	cout << "File" << filename << "sent." << endl;
	file_put.close();
	return true;
}

string TcpClient::getFileName()
{
	string filename;
	memset(&rpdu, 0x00, sizeof(MSG));
	msg_recv(sock, &rpdu);
	if (rpdu.type == DATA_SUCCESS)
	{
		filename = rpdu.buffer;
	}
	else if (rpdu.type == END)
	{
		filename = "";
	}
	
	return filename;

}

void TcpClient::run(int argc, char* argv[])
{
	//	if (argc != 4)
	//err_sys("usage: client servername filename size/time");
	char* inputserverhostname = new char[HOSTNAME_LENGTH];
	char* inputfilname = new char[FILENAME_LENGTH];
	char* inputrequesttype = new char[4];


	std::fstream file_put;


	cout << "Enter the following information" << endl;
	cout << "Server hostname:";
	cin >> inputserverhostname;
	cout << endl;
	cout << "Requested file name:";
	cin >> inputfilname;
	cout << endl;
	cout << "Request type (GET/PUT):";
	cin >> inputrequesttype;
	cout << endl;

	//initilize winsocket
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		WSACleanup();
		err_sys("Error in starting WSAStartup()\n");
	}

	//Create the socket
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket 
		err_sys("Socket Creating Error");

	//connect to the server
	ServPort = REQUEST_PORT;
	memset(&ServAddr, 0, sizeof(ServAddr)); /* Zero out structure */
	ServAddr.sin_family = AF_INET; /* Internet address family */
	ServAddr.sin_addr.s_addr = ResolveName(inputserverhostname); /* Server IP address */
	ServAddr.sin_port = htons(ServPort); /* Server port */
	if (connect(sock, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) < 0)
		err_sys("Socket Creating Error");


	command c = (inputrequesttype[0] == 'G') ? GET : PUT;
	switch (c)
	{
	case GET:

		create_get_message(tpdu, inputfilname);
		/*Send Data to the server */
		msg_send(sock, &tpdu);

		memset(&rpdu, 0x00, sizeof(rpdu));

		/*Receive Ready from the server*/
		msg_recv(sock, &rpdu);

		if (rpdu.type == DATA_SUCCESS)
		{
			getfile(inputfilname);
		}
		else if (rpdu.type == DATA_ERROR)
		{
			err_sys("Received Failure Response from the server : [%s]\n", rpdu.buffer);
		}
		else
		{
			err_sys("Unexpected Message from the server: MessageType = [%d], Message content = [%s]\n", rpdu.type,
			        rpdu.buffer);
		}
		break;

	case PUT:

		create_put_message(tpdu, inputfilname);
		msg_send(sock, &tpdu); // make upload request

		//wait for the server response
		msg_recv(sock, &rpdu); // recieve ready signal

		if (rpdu.type == DATA_SUCCESS)
		{
			putfile(inputfilname);
		}
		else
		{
			fprintf(stderr, "Error: Server not ready.\n");
		}
		break;
		
	case GETALL:
		// Send the message to the server and await ready response.
		create_getall_message(tpdu);
		
		/*Send Request to the server */
		msg_send(sock, &tpdu);

		/*Get message from the server*/
		msg_recv(sock, &rpdu);

		if (rpdu.type == DATA_SUCCESS)
		{
			bool moreFileToSend = true;
			while(moreFileToSend)
			{
				const string filename = getFileName();
				if (!filename.empty())
					getfile(filename.c_str());
				else
					moreFileToSend = false;
			}
			
		}

		
		break;

	}
	closesocket(sock);
}

TcpClient::~TcpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
}


void TcpClient::err_sys(const char* fmt, ...) //from Richard Stevens's source code
{
	perror(nullptr);
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
}

unsigned long TcpClient::ResolveName(char name[])
{
	struct hostent* host; /* Structure containing host information */

	if ((host = gethostbyname(name)) == nullptr)
		err_sys("gethostbyname() failed");

	/* Return the binary, network byte ordered address */
	return *((unsigned long*)host->h_addr_list[0]);
}


#if 1
/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpClient::msg_recv(int sock, Msg* msg_ptr)
{
	int rbytes, n = 0;

	for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
		if ((n = recv(sock, (char*)msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
			err_sys("Recv MSGHDR Error");

	for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
		if ((n = recv(sock, static_cast<char*>(msg_ptr->buffer) + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
			err_sys("Recevier Buffer Error");

	return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 */
int TcpClient::msg_send(int sock, Msg* msg_ptr)
{
	int n;
	if ((n = send(sock, (char*)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n - MSGHDRSIZE);
}

#else
/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpClient::msg_recv(int sock, Msg* msg_ptr)
{
	int n;
	n = recv(sock, (char*)msg_ptr->buffer, BUFFER_LENGTH, 0);
	if (n <= 0)
	{
		err_sys("Error in receive");
	}
	return n;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 */
int TcpClient::msg_send(int sock, Msg* msg_ptr)
{
	int n = send(sock, (char*)msg_ptr, msg_ptr->length, 0);
	if (n <= 0)
		err_sys("Send MSGHDRSIZE+length Error");
	return n;

}
#endif

int main(int argc, char* argv[]) //argv[1]=servername argv[2]=filename argv[3]=time/size
{
	TcpClient* tc = new TcpClient();
	tc->run(argc, argv);
	return 0;
}
