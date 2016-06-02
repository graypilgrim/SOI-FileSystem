/*  structure:
    FS SIZE             4 B
    FS FILES NUMBER     4 B
    NODE TABLE          10*24 B
    BITMAP              1 B -> 128 B
*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>
#include <memory>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <list>
#include <vector>

#define NAME        "disk.fs"
#define FILES_NO    30
#define BLOCK_SIZE  128
#define MUTEX       0
#define COUNTER     1

struct Node
{
    std::string name; //max 16B
    uint32_t size;
    uint32_t dataBegin;
};

class FileSystem
{
public:
    FileSystem();
    ~FileSystem();
    uint32_t GetSize();

    void Create(uint32_t) throw (std::string);
    void Destroy() throw (std::string);
    void Upload(std::string &fileName) throw (std::string);
    void Download(std::string &fileName) throw (std::string);
    void DeleteFile(std::string &fileName) throw (std::string);
    void ListFiles() throw (std::string);
    void ListMemory() throw (std::string);
    void ReadFile(std::string &fileName) throw (std::string);

private:
    void Exist() throw(std::string);
    void NotExist() throw(std::string);
    void ReadSuperblock() throw (std::string);
    void WriteSuperblock() throw (std::string);
    void WriteEmptyData();
    uint32_t FindPlace(uint32_t fileSize) throw (std::string);
    uint32_t BlocksNumber();
    uint32_t BlocksNumber(uint32_t fileSize);
    uint32_t SuperblockSize();
    int32_t FindFile(std::string &fileName) throw (std::string);
    int SemUp(uint32_t semId, uint32_t semNum);
    int SemDown(uint32_t semId, uint32_t semNum);
    int CreateSemaphore(std::string &fileName) throw (std::string);

    bool exist;
    uint32_t size;
    std::fstream partition;
    std::unique_ptr<Node[]> files;
    std::unique_ptr<bool[]> bitmap;
    std::vector<int32_t> semaphores;
};

#endif
