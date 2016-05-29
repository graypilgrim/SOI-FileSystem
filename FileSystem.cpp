#include "FileSystem.h"

FileSystem::FileSystem()
{
    partition = std::fstream(NAME, std::fstream::in | std::fstream::out | std::fstream::binary);

    files.reset(new Node[FILES_NO]);
    bitmap.reset(new bool[size / 128]);

    if (partition.good())
    {
        exist = true;
        //tablica semaforow
    }
    else
    {
        exist = false;
    }

}

FileSystem::~FileSystem()
{
    partition.close();
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

    for(int i = 0; i < FILES_NO; ++i)
        files[i].size = 0;

    for(uint i = 0; i < (size / 128); ++i)
        bitmap[i] = false;

    WriteSuperblock();
    WriteEmptyData();

    std::cout << "Partition has been created" << std::endl;
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

void FileSystem::ListFiles()
{
    ReadSuperblock();

    std::cout << "Name\t\tSize" << std::endl;

    for(int i = 0; i < FILES_NO; ++i)
    {
        if(files[i].size > 0)
            std::cout << files[i].name << "\t\t\t" << files[i].size << std::endl;
    }
}

void FileSystem::ListMemory()
{
    ReadSuperblock();

    std::cout << "\n\tSUPERBLOCK:" << std::endl;
    std::cout << "Partition size\t" << sizeof(size) << "\n" <<
                 "Nodes table\t" << 10* sizeof(Node) << "\n" <<
                 "Bitmap\t\t" << (size/BLOCK_SIZE) * sizeof(bool) << std::endl;

    std::cout << "\n\tDATA BLOCKS:" << std::endl;

    uint blocks = size/BLOCK_SIZE;
    uint cols = 8;
    uint rows = (blocks % cols > 0) ? (blocks / cols) + 1 : (blocks / cols);

    std::cout << "Block size\t" << BLOCK_SIZE << "\n" <<
                 "Blocks number\t" << blocks << std::endl;

    std::cout << "\n\tMemory map" << std::endl;

    std::cout << " \t";
    for(uint i = 0 ; i < cols; ++i)
        std::cout << i << "  ";
    std::cout << std::endl;

    for(uint i = 0; i < rows; ++i)
    {
        std::cout << i << "\t";

        for(uint j = 0; j < cols; ++j)
        {
            if(i*cols + j  >= blocks)
                break;

            if(bitmap[i*cols + j])
                std::cout << "#  ";
            else
                std::cout << "0  ";

        }
        std::cout << std::endl;
    }
}
