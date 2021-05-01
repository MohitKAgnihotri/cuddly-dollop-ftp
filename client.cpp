#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <windows.h>
#include <fstream>

#include <direct.h>
#include <vector>
#include "utility.h"
#include "client.h"

using namespace std;


bool compare_func(const filename_with_modTime_t s1, const filename_with_modTime_t s2)
{
    if ((s1.modTime == s2.modTime)
            && (s1.filename == s2.filename))
        return true;
    else
        return false;
}

void
TcpClient::create_get_message(Msg &pdu, char const *filename)
{
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = GET;
    pdu.file_read_offset = 0;
    pdu.file_read_length = 0;
    strncpy(pdu.buffer, filename, strlen(filename));
    pdu.length = strlen(tpdu.buffer);
    pdu.buffer[pdu.length + 1] = '\0';
}

void
TcpClient::create_getall_message(Msg &pdu, char const *foldername)
{
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = GETALL;
    strncpy(pdu.buffer, foldername, strlen(foldername));
    pdu.length = strlen(tpdu.buffer);
    pdu.buffer[pdu.length + 1] = '\0';
}

void
TcpClient::create_sync_message(Msg &pdu, char const *foldername)
{
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = SYNC;
    strncpy(pdu.buffer, foldername, strlen(foldername));
    pdu.length = strlen(tpdu.buffer);
    pdu.buffer[pdu.length + 1] = '\0';
}

void
TcpClient::create_put_message(Msg &pdu, char const *filename)
{
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = PUT;
    pdu.file_read_offset = 0;
    pdu.file_read_length = 0;
    strncpy(pdu.buffer, filename, strlen(filename));
    pdu.length = strlen(pdu.buffer);
    pdu.buffer[pdu.length + 1] = '\0';
}

void
TcpClient::create_end_message(Msg &pdu)
{
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = END;
}

void
TcpClient::create_putall_message(Msg &pdu, char const *foldername)
{
    memset(&pdu, 0x00, sizeof(Msg));
    pdu.type = PUTALL;
    strncpy(pdu.buffer, foldername, strlen(foldername));
    pdu.length = strlen(tpdu.buffer);
    pdu.buffer[pdu.length + 1] = '\0';
}

BOOL
TcpClient::fileExists(TCHAR *file)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE handle = FindFirstFile(file, &FindFileData);
    int found = handle != INVALID_HANDLE_VALUE;
    if (found)
    {
        FindClose(handle);
    }
    return found;
}

bool
TcpClient::getfile(const char *filename)
{
    bool isError = false;
    std::fstream file_get;
    // Open the file in the write mode.
    file_get.open(string(filename), std::fstream::out | ios::binary);
    if (!file_get.is_open())
        this->err_sys("Filed to open the file for writing");

    file_get.write(rpdu.buffer, rpdu.length);
    while (rpdu.length == BUFFER_LENGTH)
    {
        // if there is more data to write
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

bool
TcpClient::putfile(const char *filename)
{
    std::fstream file_put;
    file_put.open(filename, ios::in | ios::binary);
    if (!file_put.is_open())
        err_sys("Failed to open the file for reading");

    unsigned int bytes_to_read = getfileSize(file_put);

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

string
TcpClient::getFileName()
{
    string filename;
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

void
TcpClient::run(int argc, char *argv[])
{
    //	if (argc != 4)
    //err_sys("usage: client servername filename size/time");
    char *inputserverhostname = new char[HOSTNAME_LENGTH];
    char *inputfilname = new char[FILENAME_LENGTH];
    char *inputfoldername = new char[FILENAME_LENGTH];
    int inputrequesttype;

    std::fstream file_put;

    cout << "Enter the following information" << endl;
    cout << "Server hostname:";
    cin >> inputserverhostname;
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
    if (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
        err_sys("Socket Creating Error");

    inputrequesttype = getUserInput(inputfilname, inputfoldername);
    while (inputrequesttype != EXIT)
    {
        switch (inputrequesttype)
        {
            case GET:create_get_message(tpdu, inputfilname);
                /*Send Data to the server */
                msg_send(sock, &tpdu);

                /*Receive nReady from the server*/
                msg_recv(sock, &rpdu);

                if (rpdu.type == DATA_SUCCESS)
                {
                    getfile(inputfilname);
                }
                else if (rpdu.type == DATA_ERROR)
                {
                    cout << "Received Failure Response from the server" << rpdu.buffer << endl;
                }
                else
                {
                    cout << "Unexpected Message from the server: MessageType" << rpdu.type << "Message content = "
                         << rpdu.buffer << endl;
                }
                break;

            case PUT:file_put.open(inputfilname, ios::in | ios::binary);
                if (!file_put.is_open())
                {
                    cout << "Failed to open the file for reading";
                    break;
                }

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
                //Try to create folder with the name in the Request
                if (CreateDirectory(string("Client_" + string(inputfoldername)).c_str(), NULL))
                {
                    std::cout << "Directory" << string("Client_" + string(inputfoldername)).c_str()
                              << " Successfully Created." << std::endl;
                    SetCurrentDirectory(string("Client_" + string(inputfoldername)).c_str());
                }
                else if (ERROR_ALREADY_EXISTS == GetLastError())
                {
                    std::cout << "Directory" << string("Client_" + string(inputfoldername)).c_str()
                              << " Already Exist. Can't GETALL." << std::endl;
                    break;
                }
                else
                {
                    std::cout << "Directory" << string("Client_" + string(inputfoldername)).c_str()
                              << " Already Exist. Can't put." << std::endl;
                    break;
                }

                // Send the message to the server and await ready response.
                create_getall_message(tpdu, inputfoldername);

                /*Send Request to the server */
                msg_send(sock, &tpdu);

                /*Get message from the server*/
                msg_recv(sock, &rpdu);

                if (rpdu.type == DATA_SUCCESS)
                {
                    vector<string> fileList = ParseListofFile(rpdu.buffer);
                    for (auto &file : fileList)
                    {
                        create_get_message(tpdu, file.c_str());
                        /*Send Data to the server */
                        msg_send(sock, &tpdu);

                        /*Receive Ready from the server*/
                        msg_recv(sock, &rpdu);

                        if (rpdu.type == DATA_SUCCESS)
                        {
                            getfile(file.c_str());
                        }
                        else if (rpdu.type == DATA_ERROR)
                        {
                            cout << "Received Failure Response from the server" << rpdu.buffer << endl;
                        }
                        else
                        {
                            cout << "Unexpected Message from the server: MessageType" << rpdu.type
                                 << "Message content = " << rpdu.buffer << endl;
                        }
                    }
                    create_end_message(tpdu);
                    msg_send(sock, &tpdu);
                }
                else if (rpdu.type == DATA_ERROR)
                {
                    cout << "Error @Server" << endl;
                    cout << rpdu.buffer << endl;
                }
                else
                {
                    cout << "Incorrect Response from the Server " << rpdu.type << endl;
                }
                SetCurrentDirectory("../");
                break;
            case SYNC:
            {
                //Try to create folder with the name in the Request
                vector<filename_with_modTime_t*> clientfileList = getFilesinDirWithModTime(inputfoldername);
                if (CreateDirectory(inputfoldername, NULL))
                {
                    std::cout << "Directory" << inputfoldername
                              << " Successfully Created." << std::endl;
                    SetCurrentDirectory(inputfoldername);
                }

                    // Enter the Folder as it already exist
                else if (ERROR_ALREADY_EXISTS == GetLastError())
                {
                    SetCurrentDirectory(inputfoldername);
                }

                // Send the message to the server and await ready response.
                create_sync_message(tpdu, inputfoldername);

                /*Send Request to the server */
                msg_send(sock, &tpdu);

                /*Get message from the server*/
                msg_recv(sock, &rpdu);

                if (rpdu.type == DATA_SUCCESS)
                {
                    vector<filename_with_modTime_t*> serverfileList = ParseListofFileWithModTime(rpdu.buffer);

                    // populate put list
                    vector <string> put_list;
                    bool found = false;
                    for (int i =0; i < clientfileList.size(); i++)
                    {
                        found = false;
                        for (int j = 0; j < serverfileList.size(); j++)
                        {
                            if (clientfileList[i]->filename == serverfileList[j]->filename)
                            {
                                found = true;
                                if (clientfileList[i]->modTime > serverfileList[j]->modTime)
                                {
                                    put_list.emplace_back(clientfileList[i]->filename);
                                }

                            }
                        }
                        if (!found)
                        {
                            put_list.emplace_back(clientfileList[i]->filename);
                        }
                    }

                    // populate put list
                    vector <string> get_list;
                    found = false;
                    for (int i =0; i < serverfileList.size(); i++)
                    {
                        found = false;
                        for (int j = 0; j < clientfileList.size(); j++)
                        {
                            if (serverfileList[i]->filename == clientfileList[j]->filename)
                            {
                                found = true;
                                if (serverfileList[i]->modTime > clientfileList[j]->modTime)
                                {
                                    get_list.emplace_back(serverfileList[i]->filename);
                                }

                            }
                        }
                        if (!found)
                        {
                            get_list.emplace_back(serverfileList[i]->filename);
                        }
                    }

                    /* Get all the files */
                    for (auto &file : get_list)
                    {
                            create_get_message(tpdu, file.c_str());
                            /*Send Data to the server */
                            msg_send(sock, &tpdu);

                            /*Receive Ready from the server*/
                            msg_recv(sock, &rpdu);

                            if (rpdu.type == DATA_SUCCESS)
                            {
                                getfile(file.c_str());
                            }
                            else if (rpdu.type == DATA_ERROR)
                            {
                                cout << "Received Failure Response from the server" << rpdu.buffer << endl;
                            }
                            else
                            {
                                cout << "Unexpected Message from the server: MessageType" << rpdu.type
                                     << "Message content = " << rpdu.buffer << endl;
                            }
                    }

                    /* Pull all the files */
                    for (auto &file : put_list)
                    {
                        create_put_message(tpdu, file.c_str());
                        /*Send Data to the server */
                        msg_send(sock, &tpdu);

                        /*Receive Ready from the server*/
                        msg_recv(sock, &rpdu);

                        if (rpdu.type == DATA_SUCCESS)
                        {
                            putfile(file.c_str());
                        }
                        else if (rpdu.type == DATA_ERROR)
                        {
                            cout << "Received Failure Response from the server" << rpdu.buffer << endl;
                        }
                        else
                        {
                            cout << "Unexpected Message from the server: MessageType" << rpdu.type
                                 << "Message content = " << rpdu.buffer << endl;
                        }
                    }

                    create_end_message(tpdu);
                    msg_send(sock, &tpdu);
                    cout << "Sync completed for " << inputfoldername << endl;
                }
                else if (rpdu.type == DATA_ERROR)
                {
                    cout << "Error @Server" << endl;
                    cout << rpdu.buffer << endl;
                }
                else
                {
                    cout << "Incorrect Response from the Server " << rpdu.type << endl;
                }
                SetCurrentDirectory("../");
            }
                break;

            case PUTALL:vector<string> fileList = getFilesinDir(inputfoldername);
                if (fileList.empty())
                {
                    cout << "No files in the folder" << endl;
                }
                else
                {
                    cout << "Starting transmission" << endl;
                    create_putall_message(tpdu, inputfoldername);
                    msg_send(sock, &tpdu);

                    // Wait for the message from the server
                    msg_recv(sock, &rpdu);

                    if (rpdu.type == DATA_SUCCESS)
                    {
                        SetCurrentDirectory(inputfoldername);
                        //server is ready for the reception
                        for (auto const &value : fileList)
                        {
                            create_put_message(tpdu, value.c_str());
                            msg_send(sock, &tpdu); // make upload request

                            /*Get message from the server*/
                            msg_recv(sock, &rpdu);
                            if (rpdu.type == DATA_SUCCESS)
                            {
                                putfile(value.c_str());
                            }
                            else
                            {
                                fprintf(stderr, "Error: Server not ready.\n");
                            }
                        }

                        SetCurrentDirectory("../");

                        //send END packet
                        memset(&tpdu, 0x00, sizeof(Msg));
                        tpdu.type = END;
                        msg_send(sock, &tpdu);
                    }
                    else
                    {
                        std::cout << "Error received from the server" << std::endl;
                        std::cout << rpdu.buffer << std::endl;
                    }
                }
        }
        inputrequesttype = getUserInput(inputfilname, inputfoldername);
    }
    closesocket(sock);
}

int
TcpClient::getUserInput(char *inputfilname, char *inputfoldername)
{
    int inputrequesttype;
    cout << "Request type " << endl;
    cout << "1. GET" << endl;
    cout << "2. PUT " << endl;
    cout << "3. GETALL" << endl;
    cout << "4. PUTALL " << endl;
    cout << "5. SYNC" << endl;
    cout << "6. Exit" << endl;
    cin >> inputrequesttype;

    if (inputrequesttype == 1 || inputrequesttype == 2)
    {
        cout << "Requested file name:";
        cin >> inputfilname;
        cout << endl;
    }
    else if (inputrequesttype == 3 || inputrequesttype == 4 || inputrequesttype == 5)
    {
        cout << "Requested folder name:";
        cin >> inputfoldername;
        cout << endl;
    }
    cout << endl;
    return inputrequesttype;
}

TcpClient::~TcpClient()
{
    /* When done uninstall winsock.dll (WSACleanup()) and exit */
    WSACleanup();
}

void
TcpClient::err_sys(const char *fmt, ...) //from Richard Stevens's source code
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

unsigned long
TcpClient::ResolveName(char name[])
{
    struct hostent *host; /* Structure containing host information */

    if ((host = gethostbyname(name)) == nullptr)
        err_sys("gethostbyname() failed");

    /* Return the binary, network byte ordered address */
    return *((unsigned long *) host->h_addr_list[0]);
}

#if 1
/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int
TcpClient::msg_recv(int sock, Msg *msg_ptr)
{
    int rbytes, n = 0;
    memset(msg_ptr, 0x00, sizeof(Msg));

    for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
        if ((n = recv(sock, (char *) msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
            err_sys("Recv MSGHDR Error");

    for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
        if ((n = recv(sock, static_cast<char *>(msg_ptr->buffer) + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
            err_sys("Recevier Buffer Error");

    return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 */
int
TcpClient::msg_send(int sock, Msg *msg_ptr)
{
    int n;
    if ((n = send(sock, (char *) msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
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

int
main(int argc, char *argv[]) //argv[1]=servername argv[2]=filename argv[3]=time/size
{
    TcpClient *tc = new TcpClient();
    tc->run(argc, argv);
    return 0;
}
