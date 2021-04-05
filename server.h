#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 2345
#define BUFFER_LENGTH 1024 
#define MAXPENDING 10
#define MSGHDRSIZE 16 //Message Header Size


typedef enum
{
	GET = 1,
	PUT = 2,
	GETALL = 3,
	PUTALL = 4,
	DATA_SUCCESS = 5,
	DATA_ERROR = 6,
	END = 7
} command;

typedef struct  
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req;  //request

typedef struct  
{
	char response[RESP_LENGTH];
} Resp; //response


typedef struct
{
	command type;
	int  length; //length of effective bytes in the buffer
	int file_read_offset;
	int file_read_length;
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


class TcpServer
{
	int serverSock,clientSock;     /* Socket descriptor for server and client*/
	struct sockaddr_in ClientAddr; /* Client address */
	struct sockaddr_in ServerAddr; /* Server address */
	unsigned short ServerPort;     /* Server port */
	int clientLen;            /* Length of Server address data structure */
	char servername[HOSTNAME_LENGTH];


public:
		TcpServer();
		~TcpServer();
		void start();
};

class TcpThread :public Thread
{

	int cs;
	void Create_Ready_Request(Msg& pdu);
	void Create_Error_Request(Msg& pdu);
	void GetFileFromClient(std::fstream&);
public: 
	TcpThread(int clientsocket):cs(clientsocket)
	{}
	virtual void run();
    int msg_recv(int ,Msg * );
	int msg_send(int ,Msg * );
	unsigned long ResolveName(char name[]);
    static void err_sys(const char * fmt,...);
};

#endif