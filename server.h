#include <string>
#include "utility.h"

#ifndef SER_TCP_H
#define SER_TCP_H

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
    void Create_END_Request(Msg& pdu);
	void Create_Error_Request(Msg& pdu, const std::string &errorString);
	void GetFileFromClient(std::fstream&);
public: 
	TcpThread(int clientsocket):cs(clientsocket)
	{}
	virtual void run();
    int msg_recv(int ,Msg * );
	int msg_send(int ,Msg * );
	unsigned long ResolveName(char name[]);
    static void err_sys(const char * fmt,...);
    int SendFileToClient(std::fstream &file_get);
};

#endif