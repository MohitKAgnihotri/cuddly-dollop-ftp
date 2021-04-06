#include <vector>
#include <string>


#ifndef FTP_UTILITY_H
#define FTP_UTILITY_H

#define MAXPENDING 10
#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 2345
#define BUFFER_LENGTH 1024
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
    command type;
    int length; //length of effective bytes in the buffer
    int file_read_offset;
    int file_read_length;
    char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving

bool isDirectoryExist(const char * dirname);

unsigned int getfileSize(std::fstream &file);
std::vector<std::string> getFilesinDir(const char* folder);
std::string current_working_directory( void );
std::vector<std::string> ParseListofFile(char * inputBuffer);

#endif//FTP_UTILITY_H
