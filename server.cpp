#pragma once
#pragma comment(lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include "server.h"
#include "Thread.h"
#include "utility.h"
#include <direct.h>
#include <fstream>
#include <iostream>
#include <process.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <winsock2.h>

using namespace std;

TcpServer::TcpServer() {
    WSADATA wsadata;
    if (WSAStartup(0x0202, &wsadata) != 0)
        TcpThread::err_sys("Starting WSAStartup() error\n");

    // Display name of local host
    if (gethostname(servername, HOSTNAME_LENGTH) != 0) // get the hostname
        TcpThread::err_sys("Get the host name error,exit");

    printf("Server: %s waiting to be contacted for time/size request...\n",
           servername);

    // Create the server socket
    if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        TcpThread::err_sys("Create socket error,exit");

    // Fill-in Server Port and Address info.
    ServerPort = REQUEST_PORT;
    memset(&ServerAddr, 0, sizeof(ServerAddr));     /* Zero out structure */
    ServerAddr.sin_family = AF_INET;                /* Internet address family */
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServerAddr.sin_port = htons(ServerPort);        /* Local port */

    // Bind the server socket
    if (bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
        TcpThread::err_sys("Bind socket error,exit");

    // Successfull bind, now listen for Server requests.
    if (listen(serverSock, MAXPENDING) < 0)
        TcpThread::err_sys("Listen socket error,exit");
}

TcpServer::~TcpServer() { WSACleanup(); }

void TcpServer::start() {
    for (;;) /* Run forever */
    {
        /* Set the size of the result-value parameter */
        clientLen = sizeof(ClientAddr);

        /* Wait for a Server to connect */
        if ((clientSock = accept(serverSock, (struct sockaddr *) &ClientAddr,
                                 &clientLen)) < 0)
            TcpThread::err_sys("Accept Failed ,exit");

        /* Create a Thread for this new connection and run*/
        TcpThread *pt = new TcpThread(clientSock);
        pt->start();
    }
}

//////////////////////////////TcpThread Class
/////////////////////////////////////////////
void TcpThread::err_sys(const char *fmt, ...) {
    perror(NULL);
    va_list args;
            va_start(args, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
            va_end(args);
    exit(1);
}

unsigned long TcpThread::ResolveName(char name[]) {
    struct hostent *host; /* Structure containing host information */

    if ((host = gethostbyname(name)) == NULL)
        err_sys("gethostbyname() failed");

    /* Return the binary, network byte ordered address */
    return *((unsigned long *) host->h_addr_list[0]);
}

#if 1

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been
recevied successfully.
*/
int TcpThread::msg_recv(int sock, Msg *msg_ptr) {
    int rbytes, n;

    for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
        if ((n = recv(sock, (char *) msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
            err_sys("Recv MSGHDR Error");

    for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
        if ((n = recv(sock, (char *) msg_ptr->buffer + rbytes,
                      msg_ptr->length - rbytes, 0)) <= 0)
            err_sys("Recevier Buffer Error");

    return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent
 * out successfully
 */
int TcpThread::msg_send(int sock, Msg *msg_ptr) {
    int n;
    if ((n = send(sock, (char *) msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) !=
        (MSGHDRSIZE + msg_ptr->length))
        err_sys("Send MSGHDRSIZE+length Error");
    return (n - MSGHDRSIZE);
}

#else
int TcpThread::msg_recv(int sock, Msg *msg_ptr) {
  int n;

  n = recv(sock, (char *)msg_ptr->buffer, BUFFER_LENGTH, 0);
  if (n <= 0)
    err_sys("Recevier Buffer Error");

  return n;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent
 * out successfully
 */
int TcpThread::msg_send(int sock, Msg *msg_ptr) {
  int n;
  n = send(sock, (char *)msg_ptr, msg_ptr->length, 0);
  if (n <= 0)
    err_sys("Send Buffer Error");
  return (n);
}
#endif

void TcpThread::Create_Ready_Request(Msg &pdu) {
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = DATA_SUCCESS;
}

void TcpThread::Create_END_Request(Msg &pdu) {
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = END;
}

void TcpThread::Create_Error_Request(Msg &pdu, const std::string &errorString) {
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = DATA_ERROR;
    memcpy(pdu.buffer, errorString.c_str(), errorString.length());
    pdu.length = errorString.length();
}

void TcpThread::GetFileFromClient(std::fstream &fileToGet) {
    Msg rpdu;
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
        fileToGet.write(rpdu.buffer, rpdu.length); // write data to file
    } while (rpdu.length == BUFFER_LENGTH);
}

void TcpThread::run() // cs: Server socket
{
    Msg rpdu, spdu; // send_message receive_message
    unsigned int bytes_to_read = 0;
    std::fstream file_get;
    std::fstream file_put;

    memset(&rpdu, 0x00, sizeof(rpdu));
    msg_recv(cs, &rpdu); // Receive a message from the client

    switch (rpdu.type) {
        /* Download request */
        case GET:
            cout << "Starting transfer of" << rpdu.buffer << endl;

            file_get.open(rpdu.buffer, std::fstream::in | std::fstream::out);
            if (!file_get.is_open()) {
                Create_Error_Request(
                        spdu, string("Error: File " + string(rpdu.buffer) + " Not Found"));
                msg_send(cs, &spdu);
                cout << "Error: File " << rpdu.buffer << " Not Found" << endl;
            } else {
                (void) SendFileToClient(file_get);
            }
            file_get.close();
            closesocket(cs); // close the connection
            printf("Transfer sucessful.\n");
            break;

            /* Upload request */
        case PUT:
            Create_Ready_Request(spdu);
            msg_send(cs, &spdu); // tell client ready

            file_put.open("server" + std::string(rpdu.buffer), std::fstream::out);
            if (!file_put.is_open())
                err_sys("failed to open the file");
            printf("Starting transfer of \"%s\".\n", rpdu.buffer);
            GetFileFromClient(file_put);
            file_put.close();
            printf("Transfer sucessful.\n");
            break;

        case GETALL:
            // Check if the directory Exist
            if (isDirectoryExist(rpdu.buffer)) {
                std::vector<std::string> filesinDir = getFilesinDir(rpdu.buffer);

                if (!filesinDir.empty()) {

                    // Send Ready  to the client
                    Create_Ready_Request(spdu);

                    // Start GET ALL for the all the files
                    for (auto &file : filesinDir) {
                        strncat(spdu.buffer, file.c_str(), file.length());
                        strncat(spdu.buffer, ",", 1);
                    }

                    spdu.length = strlen(spdu.buffer) > BUFFER_LENGTH ? BUFFER_LENGTH
                                                                      : strlen(spdu.buffer);
                    msg_send(cs, &spdu);
                    // set current directory
                    SetCurrentDirectory(rpdu.buffer);

                    while (true) {
                        memset(&rpdu, 0x00, sizeof(Msg));
                        msg_recv(cs, &rpdu);
                        if (rpdu.type == GET) {
                            cout << "Starting transfer of" << rpdu.buffer << endl;

                            file_get.open(rpdu.buffer, std::fstream::in | std::fstream::out);
                            if (!file_get.is_open()) {
                                Create_Error_Request(
                                        spdu,
                                        string("Error: File " + string(rpdu.buffer) + " Not Found"));
                                msg_send(cs, &spdu);
                                cout << "Error: File " << rpdu.buffer << " Not Found" << endl;
                            } else {
                                (void) SendFileToClient(file_get);
                            }
                            file_get.close();
                        } else if (rpdu.type == END) {
                            cout << "Transfer Finished" << endl;
                            break;
                        } else {
                            cout << "Unknown Packet from the Client" << endl;
                        }
                    }
                    SetCurrentDirectory("../");
                }
                Create_END_Request(spdu);
                msg_send(cs, &spdu);
            } else {
                std::cout << "Directory" << rpdu.buffer << "Doesn't Exist. Can't GETALL."
                          << std::endl;
                std::cout << "Current working directory = "
                          << current_working_directory();
                Create_Error_Request(
                        spdu, std::string("Directory Doesn't Exist. Can't GETALL."));
                msg_send(cs, &spdu);
            }
            break;
        case PUTALL:
            // Try to create folder with the name in the Request
            if (CreateDirectory(rpdu.buffer, NULL)) {
                std::cout << "Directory" << rpdu.buffer << " Sucessfully Created."
                          << std::endl;
                Create_Ready_Request(spdu);
                msg_send(cs, &spdu); // tell client ready
                SetCurrentDirectory(rpdu.buffer);
            } else if (ERROR_ALREADY_EXISTS == GetLastError()) {
                std::cout << "Directory" << rpdu.buffer << " Already Exist. Can't put."
                          << std::endl;
                Create_Error_Request(
                        spdu, std::string("Directory Already Exist. Can't Overwrite"));
                msg_send(cs, &spdu);
            } else {
                std::cout << "Directory" << rpdu.buffer << " Already Exist. Can't put."
                          << std::endl;
                Create_Error_Request(spdu,
                                     std::string("Can't Create Directory at the Server"));
                msg_send(cs, &spdu);
            }

            while (1) {
                // Client will do multiple PUT
                // Receive PUT message for the 1st file
                msg_recv(cs, &rpdu); // Receive a message from the client
                if (rpdu.type == PUT) {
                    file_put.open("server" + std::string(rpdu.buffer), std::fstream::out);
                    if (!file_put.is_open())
                        err_sys("failed to open the file");

                    printf("Starting transfer of \"%s\".\n", rpdu.buffer);

                    Create_Ready_Request(spdu);
                    msg_send(cs, &spdu); // tell client ready

                    GetFileFromClient(file_put);
                    file_put.close();
                    std::cout << "Transfer sucessful for file" << rpdu.buffer << std::endl;
                } else if (rpdu.type == END) {
                    std::cout << "Finished getting all the files." << std::endl;
                    SetCurrentDirectory("../");
                    break;
                }
            }
            break;
    }

    closesocket(cs);
}

int TcpThread::SendFileToClient(fstream &file_get) {
    Msg spdu;
    unsigned int bytes_to_read = getfileSize(file_get);
    while (bytes_to_read > 0) {
        memset(&spdu, 0x00, sizeof(spdu));
        file_get.read(spdu.buffer, BUFFER_LENGTH);
        if (!file_get)
            spdu.length = file_get.gcount();
        else
            spdu.length = BUFFER_LENGTH;

        bytes_to_read -= spdu.length;
        spdu.type = DATA_SUCCESS;
        msg_send(cs, &spdu);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////

int main(void) {

    TcpServer ts;
    ts.start();

    return 0;
}
