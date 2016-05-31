#include "FileSystem.h"

FileSystem::FileSystem()
{
    partition = std::fstream(NAME, std::fstream::in | std::fstream::out | std::fstream::binary);

    files.reset(new Node[FILES_NO]);
    bitmap.reset(new bool[size / 128]);

    if (partition.good())
    {
        exist = true;
        partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));

    }
    else
    {
        exist = false;
    }

}

FileSystem::~FileSystem()
{
    partition.close();

    semctl(semId, (int)0, IPC_RMID, (int)0);
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

    for(int i = 0; i < 10; ++i)
    {
        std::stringstream ss;
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
    bool init = true;

    semId = semget(SEM_ARRAY, FILES_NO, IPC_CREAT|IPC_EXCL|0600);
    if (semId == -1){
        semId = semget(SEM_ARRAY, FILES_NO, 0600);
        init = false;
        if (semId == -1){
            throw std::string("Can't create semaphores array\n");
        }
    }

    partition.seekg(0, partition.beg);
    partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));

    for(int i = 0; i < 10; ++i)
    {
        std::stringstream ss;
        char tempName[16] = "";
        partition.read(reinterpret_cast<char *>(&tempName), sizeof(tempName));
        partition.read(reinterpret_cast<char *>(&(files[i].size)), sizeof(uint32_t));
        partition.read(reinterpret_cast<char *>(&(files[i].dataBegin)), sizeof(uint32_t));

        ss << tempName;
        ss >> files[i].name;

        if(init)
            semctl(semId, i, SETVAL,(int)1);
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
            std::cout << files[i].name << "\t" << files[i].size << std::endl;
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

void FileSystem::Upload(std::string &fileName)
{
    ReadSuperblock();


    std::fstream newFile(fileName, std::fstream::in | std::fstream::out |std::fstream::binary);

    if(!newFile.good())
        throw std::string ("The file does not exist! Check file name\n");

    newFile.seekg(0, newFile.end);
    std::streampos end = newFile.tellg();
    newFile.seekg(0, newFile.beg);
    std::streampos begin = newFile.tellg();

    uint32_t newFileSize = end - begin;

    uint32_t dataBegin;
    try
    {
        dataBegin = FindPlace(newFileSize);
    }
    catch(std::string e)
    {
        throw e;
    }

    uint32_t index = 0;

    for(uint i = 0; i < FILES_NO; ++i)
    {
        if(files[i].name == fileName)
            throw std::string("File name must be unique!\n");

        if(files[i].size > 0)
            ++index;
    }

    if(index == FILES_NO)
        throw std::string ("The maximum number of files has been reached. Delete some file to upload new\n");

    files[index].name = fileName;
    files[index].size = newFileSize;
    files[index].dataBegin = dataBegin;


    char temp[BLOCK_SIZE];

    newFile.seekg(0, newFile.beg);
    partition.seekp(SuperblockSize() + 128*dataBegin, partition.beg);

    uint32_t blocksNo = BlocksNumber(newFileSize);

    for(uint i = 0; i < blocksNo; ++i)
    {
        newFile.read(reinterpret_cast<char *>(&temp), sizeof(temp));
        partition.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
        bitmap[dataBegin+i] = true;
    }
    newFile.close();

    WriteSuperblock();

    std::cout << "The file has been added" << std::endl;
}

uint32_t FileSystem::FindPlace(uint32_t fileSize)
{
    uint32_t x = BlocksNumber();
    uint32_t begin = 0;
    uint32_t counter = 0;
    for(uint i = 0; i < x; ++i)
    {

        if(bitmap[i] == false)
            counter += (i - begin + 1) * 128;
        else
        {
            counter = 0;
            begin = i + 1;
        }

        if(counter >= fileSize)
            return begin;
    }

    throw std::string("There is no enough place, remove some files and try again\n");
}

uint32_t FileSystem::BlocksNumber()
{
    return size / BLOCK_SIZE;
}

uint32_t FileSystem::BlocksNumber(uint32_t fileSize)
{
    return (fileSize % BLOCK_SIZE == 0 ? fileSize / BLOCK_SIZE : fileSize / BLOCK_SIZE + 1);
}

uint32_t FileSystem::SuperblockSize()
{
    return sizeof(size) + 10* sizeof(Node) + (size/BLOCK_SIZE) * sizeof(bool);
}

uint32_t FileSystem::FilesNumber()
{
    uint32_t counter = 0;

    for(uint i = 0; i < FILES_NO; ++i)
    {
        if(files[i].size > 0)
            ++counter;
    }

    return counter;
}

void FileSystem::DeleteFile(std::string &fileName)
{
    ReadSuperblock();

    uint32_t index = FindFile(fileName);

    uint32_t begin = files[index].dataBegin;
    uint32_t end = files[index].dataBegin + BlocksNumber(files[index].size);

    for(uint i = begin; i < end; ++i)
        bitmap[i] = false;

    files[index].name = std::string();
    files[index].size = 0;
    files[index].dataBegin = 0;

    WriteSuperblock();
}

uint32_t FileSystem::FindFile(std::string &fileName)
{
    for(uint i = 0; i < FILES_NO; ++i)
    {
        if(files[i].name == fileName)
        {
            return i;
        }
    }

    throw std::string("There is no file with this name!\n");
}

void FileSystem::Download(std::string &fileName)
{
    ReadSuperblock();

    uint32_t index;

    try
    {
        index = FindFile(fileName);
    }
    catch (std::string &e)
    {
        throw e;
    }

    std::fstream file(fileName, std::fstream::out | std::fstream::binary);

    char *temp = new char[files[index].size];

    uint32_t begin = files[index].dataBegin;

    partition.seekp(SuperblockSize() + BLOCK_SIZE*begin, partition.beg);

    partition.read(reinterpret_cast<char *>(&temp), sizeof(temp));
    file.write(reinterpret_cast<const char *>(&temp), sizeof(temp));

    file.close();
}

void FileSystem::ReadFile(std::string &fileName)
{
    ReadSuperblock();

    FindFile(fileName);

    while(1)
    {}
}
