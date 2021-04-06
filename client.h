
#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

class TcpClient {
    int sock; /* Socket descriptor */
    struct sockaddr_in ServAddr; /* server socket address */
    unsigned short ServPort; /* server port */
    Msg tpdu, rpdu; /* Transmit and Receive message */
    WSADATA wsadata;

    void create_get_message(Msg &, char const *filename);

    void create_put_message(Msg &, char const *filename);

    void create_getall_message(Msg &, char const *filename);

    void create_putall_message(Msg &, char const *filename);

    void TcpClient::create_end_message(Msg &pdu);

    bool getfile(const char *filename);

    bool putfile(const char *filename);

    std::string getFileName();

public:
    static BOOL fileExists(TCHAR *file);

    TcpClient() {
    }

    void run(int argc, char *argv[]);

    ~TcpClient();

    int msg_recv(int, Msg *);

    int msg_send(int, Msg *);

    unsigned long ResolveName(char name[]);

    void err_sys(const char *fmt, ...);
};

#endif//FTP_CLIENT_H
