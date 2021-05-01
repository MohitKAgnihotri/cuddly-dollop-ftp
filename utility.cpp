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




vector<filename_with_modTime_t*>  getFilesinDirWithModTime(const char* folder)
{
    vector<filename_with_modTime_t*> filelist_with_time;
    vector<string> filesinDir = getFilesinDir(folder);
    vector<int64_t> modificationTime = getLastModifiedTime(folder, filesinDir);

    for ( int i = 0; i < filesinDir.size(); i++)
    {
        filename_with_modTime_t *temp = new filename_with_modTime_t;
        temp->filename = filesinDir[i];
        temp->modTime = modificationTime[i];
        filelist_with_time.emplace_back(temp);
    }
    return filelist_with_time;
}


vector<int64_t> getLastModifiedTime(const char* folder, vector<string> fileList)
{
    vector<int64_t> mod_time;
    struct _stat result;

    SetCurrentDirectory(folder);
    for (auto &file : fileList)
    {
        if (_stat(file.c_str(), &result) == 0)
        {
            mod_time.emplace_back(result.st_mtime);
        }
    }
    SetCurrentDirectory("../");
    return mod_time;
}

std::string current_working_directory( void )
{
    char* cwd = _getcwd( 0, 0 ) ; // **** microsoft specific ****
    std::string working_directory(cwd) ;
    std::free(cwd) ;
    return working_directory;
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

vector<filename_with_modTime_t*> ParseListofFileWithModTime(char * inputBuffer)
{
    vector<filename_with_modTime_t*> filelist_with_time;
    char* next_token;
    char *p = strtok_s(inputBuffer, ",", &next_token);
    while (p) {
        string filename = string(p);
        p = strtok_s(NULL, ",",&next_token);
        string modtime = string(p);

        auto *temp = new filename_with_modTime_t;
        temp->filename = filename;
        temp->modTime = stoll(modtime);

        filelist_with_time.emplace_back(temp);
        p = strtok_s(NULL, ",",&next_token);
    }
    return filelist_with_time;
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