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
    PUT,
    GETALL,
    PUTALL,
    SYNC,
    EXIT,
    DATA_SUCCESS,
    DATA_ERROR,
    END
} command;

typedef struct filename_with_modTime
{
    std::string filename;
    int64_t modTime;
}filename_with_modTime_t;

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
std::vector<int64_t> getLastModifiedTime(const char* folder, std::vector<std::string> fileList);
std::vector<filename_with_modTime_t*> ParseListofFileWithModTime(char * inputBuffer);
std::vector<filename_with_modTime_t*>  getFilesinDirWithModTime(const char* folder);

#endif//FTP_UTILITY_H
