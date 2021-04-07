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

using namespace std;


std::string current_working_directory( void )
{
    char* cwd = _getcwd( 0, 0 ) ; // **** microsoft specific ****
    std::string working_directory(cwd) ;
    std::free(cwd) ;
    return working_directory ;
}

vector<string> ParseListofFile(char * inputBuffer)
{
    vector<string> filelist;
    char* next_token;
    char *p = strtok_s(inputBuffer, ",", &next_token);
    while (p) {
        filelist.emplace_back(string(p));
        p = strtok_s(NULL, ",",&next_token);
    }
    return filelist;
}

vector<string> getFilesinDir(const char* folder)
{
    string search_path = string(folder) + "/*.*";
    vector<string> fileList;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile(search_path.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("FindFirstFile failed (%d)\n", GetLastError());
        return fileList;
    }

    do
    {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            fileList.emplace_back(FindFileData.cFileName);
    }
    while (FindNextFile(hFind, &FindFileData) != 0);
    FindClose(hFind);

    return fileList;
}

unsigned int getfileSize(std::fstream &file)
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

bool isDirectoryExist(const char * dirname)
{
    DWORD ftyp = GetFileAttributesA(dirname);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!
    return false;    // this is not a directory!
}