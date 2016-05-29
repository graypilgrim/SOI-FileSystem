#include "FileSystem.h"

FileSystem::FileSystem()
{
    partition = std::fstream(NAME, std::fstream::in | std::fstream::out | std::ios::binary);

    if (partition.good())
    {
        exist = true;
        ReadSuperblock();
        //tablica semaforow
    }
    else
    {
        exist = false;
    }

}

void FileSystem::Exist() const throw(std::string)
{
    if (!exist)
        throw std::string("Partition does not exist!\nUse -create.\n");
}

void FileSystem::NotExist() const throw(std::string)
{
    if (exist)
        throw std::string("Partition already exists!\nEdit or destroy existing partition.\n");
}

void FileSystem::Create(uint32_t size)
{
    NotExist();

    partition = std::fstream(NAME, std::fstream::out | std::fstream::binary);

    if(size < 0)
        throw std::string("Size must not be negative!\n");

    while((size % 128) != 0)
        ++size;

    this->size = size;

    files.reset(new Node[FILES_NO]);
    bitmap.reset(new bool[size / 128]);

    for(int i = 0; i < FILES_NO; ++i)
        files[i].size = 0;

    for(uint i = 0; i < size / 128; ++i)
        bitmap[i] = false;

    WriteSuperblock();
    WriteEmptyData();

    partition.close();
}

void FileSystem::Destroy()
{
    Exist();

    remove(NAME);
}

void FileSystem::WriteSuperblock()
{
    partition.seekp(0, partition.beg);
    partition.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));

    std::stringstream ss;
    for(int i = 0; i < 10; ++i)
    {
        char tempName[16] = "";
        ss << files[i].name;
        ss >> tempName;
        partition.write(reinterpret_cast<const char *>(&tempName), sizeof(tempName));
        partition.write(reinterpret_cast<const char *>(&(files[i].size)), sizeof(uint32_t));
        partition.write(reinterpret_cast<const char *>(&(files[i].dataBegin)), sizeof(uint32_t));
    }

    int blocksNo = size/BLOCK_SIZE;

    for(int i = 0; i < blocksNo; ++i)
    {
        partition.write(reinterpret_cast<const char *>(&bitmap[i]), sizeof(bool));
    }
}

void FileSystem::ReadSuperblock()
{
    partition.seekg(0, partition.beg);
    partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));

    std::stringstream ss;
    for(int i = 0; i < 10; ++i)
    {
        char tempName[16] = "";
        partition.read(reinterpret_cast<char *>(&tempName), sizeof(tempName));
        partition.read(reinterpret_cast<char *>(&(files[i].size)), sizeof(uint32_t));
        partition.read(reinterpret_cast<char *>(&(files[i].dataBegin)), sizeof(uint32_t));

        ss << tempName;
        ss >> files[i].name;
    }

    int blocksNo = size/BLOCK_SIZE;

    for(int i = 0; i < blocksNo; ++i)
    {
        partition.read(reinterpret_cast<char *>(&bitmap[i]), sizeof(bool));
    }

}

void FileSystem::WriteEmptyData()
{
    int blocksNo = size/BLOCK_SIZE;

    char temp[BLOCK_SIZE] = "";
    for(int i = 0; i < blocksNo; ++i)
        partition.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
}
