/*  structure:
    FS SIZE         4B
    NODE TABLE      10*24B
    BITMAP          1 b -> 128 B
*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <memory>

#define NAME disk
#define BLOCK_SIZE 128
#define FILES_NO

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
    void Exist() const throw(std::string);
    void NotExist() const throw(std::string);
    size_t FindFile(std::string &s);
    uint32_t FindPlace(uint32_t size);


    bool exist;
    uint32_t size;
    fstream partition;
    std::unique_ptr<Node[FILES_NO]> files;
    std::unique_ptr<bool[]> bitmap;

public:
    FileSystem();
    uint32_t GetSize() const;

    uint32_t Create(uint32_t);
    bool Destroy();
    bool Upload(std::string s);
    bool Download(std::string s);
    bool DestroyFile(std::string s);
    void ListFiles();
    void ListMemory();

};

#endif
