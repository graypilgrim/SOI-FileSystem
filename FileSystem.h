/*  structure:
    FS SIZE         4B
    NODE TABLE      10*24B
    BITMAP          1 b -> 128 B
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
private:
    void ReadSuperblock();
    void WriteSuperblock();
    void WriteEmptyData();
    void Exist() const throw(std::string);
    void NotExist() const throw(std::string);
    uint32_t FindPlace(uint32_t fileSize);
    uint32_t BlocksNumber();
    uint32_t BlocksNumber(uint32_t fileSize);
    std::list<Node>::iterator FindFile(std::string &fileName);
    uint32_t SuperblockSize();
    int SemUp(uint32_t semId, uint32_t semNum);
    int SemDown(uint32_t semId, uint32_t semNum);
    int CreateSemaphore(std::string &fileName);

    bool exist;
    uint32_t size;
    uint32_t filesNo;
    std::fstream partition;
    std::list<Node> files;
    std::unique_ptr<bool[]> bitmap;
    std::vector<int32_t> semaphores;


public:
    FileSystem();
    ~FileSystem();
    uint32_t GetSize() const;

    void Create(uint32_t);
    void Destroy();
    void Upload(std::string &fileName);
    void Download(std::string &s);
    void DeleteFile(std::string &s);
    void ListFiles();
    void ListMemory();
    void ReadFile(std::string &fileName);

};

#endif
