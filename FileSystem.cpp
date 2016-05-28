#include "FileSystem.h"

FileSystem::FileSystem()
{
    parition = std::fstream(NAME);

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

void FileSystem::Exist() throw(std::string)
{
    if (!exist)
        throw std::string("Partition does not exist!\nUse -create.");
}

void FileSystem::NotExist() throw(std::string)
{
    if (exist)
        throw std::string("Partition already exists!\nEdit or destroy existing partition.\n");
}

uint32_t FileSystem::Create(uint32_t size)
{
    NotExist();

    if(size < 0)
        throw std::string("Size must not be negative!\n");

    while((size % 128) != 0)
        ++size;

    this.size = size;

    for(int i = 0; i < FILES_NO; ++i)
        files[i].size = 0;

    bitmap.reset(new bool[size / 128]);

    WriteSuperblock();
    WriteEmptyData();

    partition.close();

    return size;
}

bool FileSystem::Destroy()
{
    Exist();

    remove(NAME);
}

bool FileSystem::Upload(std::string &fileName)
{
    std::ifstream file(fileName);
    if(!file.good())
        throw std::string("The file does not exist!\n");

    if !possible
        throw exception;

}

bool FileSystem::Download()
{
    FindFile(std::string);
}

bool FileSystem::DestroyFile()
{
    FindFile(std::string);
}

void ListFiles()
{
    for each
        cout << << << <<
}

void ListMemory()
{
    for each
        cout << << < << <<
}

void WriteSuperblock()
{
    //ustawic pisanie na poczatek
    parition.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));

    std::stringstream ss;
    for(int i = 0; i < 10; ++i)
    {
        char tempName[16] = "";
        ss << files[i].name;
        ss >> tempName;
        partition.write(reinterpret_cast<const char *>(&tempName), sizeof(tempName));
        partition.write(reinterpret_cast<const char *>(&(files[i].size), sizeof(uint32_t)));
        partition.write(reinterpret_cast<const char *>(&(files[i].dataBegin), sizeof(uint32_t)));
    }

    for(int i = 0; i < size/BLOCK_SIZE; ++i)
    {
        partition.write(reinterpret_cast<const char *>(&bitmap[i]), sizeof(bool));
    }
}

void ReadSuperblock()
{
    //ustawic czytanie na poczatek
    partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));

    std::stringstream ss;
    for(int i = 0; i < 10; ++i)
    {
        char tempName[16] = "";
        partition.read(reinterpret_cast<char *>(&tempName), sizeof(tempName));
        partition.read(reinterpret_cast<char *>(&(files[i].size), sizeof(uint32_t)));
        partition.read(reinterpret_cast<char *>(&(files[i].dataBegin), sizeof(uint32_t)));

        ss << tempName;
        ss >> files[i].name;
    }

    for(int i = 0; i < size/BLOCK_SIZE; ++i)
    {
        partition.read(reinterpret_cast<char *>(&bitmap[i]), sizeof(bool));
    }

}
