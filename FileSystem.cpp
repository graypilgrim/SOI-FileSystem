#include "FileSystem.h"

FileSystem::FileSystem()
{
    partition = std::fstream(NAME, std::fstream::in | std::fstream::out | std::fstream::binary);

    if (partition.good())
    {
        exist = true;
    }
    else
    {
        exist = false;
    }

}

FileSystem::~FileSystem()
{

    partition.close();
/*
    for(auto id : semaphores)
    {
        std::cout << "checking" << std::endl;
        SemDown(id, MUTEX);
        std::cout << "in" << std::endl;
        if(semctl(id, ACCESS, GETVAL, 0) >= 0)
            semctl(id, 0, IPC_RMID, 0);
        SemUp(id, MUTEX);
    }
*/
}

void FileSystem::Create(uint32_t size) throw (std::string)
{
    NotExist();

    partition = std::fstream(NAME, std::fstream::out | std::fstream::binary);

    if(size < 0)
        throw std::string("Size must not be negative!\n");

    while((size % 128) != 0)
        ++size;

    this->size = size;

    uint blocksNo = BlocksNumber();

    bitmap.reset(new bool[BlocksNumber()]);
    files.reset(new Node[FILES_NO]);

    for(uint i = 0; i < FILES_NO; ++i)
        files[i].size = 0;

    for(uint i = 0; i < blocksNo; ++i)
        bitmap[i] = false;

    WriteSuperblock();
    WriteEmptyData();

    std::cout << "Partition has been created" << std::endl;
}

void FileSystem::Destroy() throw (std::string)
{
    Exist();

    remove(NAME);
}

void FileSystem::Upload(std::string &fileName) throw (std::string)
{
    Exist();
    ReadSuperblock();

    std::fstream newFile(fileName, std::fstream::in | std::fstream::out |std::fstream::binary);

    if(!newFile.good())
        throw std::string ("The file does not exist! Check file name\n");

    newFile.seekg(0, newFile.end);
    std::streampos end = newFile.tellg();
    newFile.seekg(0, newFile.beg);
    std::streampos begin = newFile.tellg();

    uint32_t newFileSize = end - begin;

    uint32_t dataBegin = FindPlace(newFileSize);

    int32_t index = -1;

    for(uint i = 0; i < FILES_NO; ++i)
    {
        if(files[i].name == fileName)
            throw std::string("File name must be unique!\n");

        if(index < 0 && files[i].size == 0)
            index = i;
    }

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

void FileSystem::Download(std::string &fileName) throw (std::string)
{
    Exist();
    ReadSuperblock();

    int semId = CreateSemaphore(fileName);

    if(semctl(semId, COUNTER, GETVAL, 0) == 0)
    {
        if(semctl(semId, MUTEX, GETVAL, 0) == 0)
            throw std::string("Can't get access to the file\n");
        else
            SemDown(semId, MUTEX);
    }

    SemUp(semId, COUNTER);

    int32_t index = FindFile(fileName);

    std::fstream file(fileName, std::fstream::out | std::fstream::binary);

    char *temp = new char[files[index].size];

    uint32_t begin = files[index].dataBegin;

    partition.seekp(SuperblockSize() + BLOCK_SIZE*begin, partition.beg);

    partition.read(reinterpret_cast<char *>(&temp), sizeof(temp));
    file.write(reinterpret_cast<const char *>(&temp), sizeof(temp));

    file.close();

    SemDown(semId, COUNTER);

    if(semctl(semId, COUNTER, GETVAL) == 0)
        SemUp(semId, MUTEX);
}

void FileSystem::DeleteFile(std::string &fileName) throw (std::string)
{
    Exist();
    ReadSuperblock();
    int32_t index = FindFile(fileName);

    uint32_t semId = CreateSemaphore(fileName);

    SemDown(semId, MUTEX);
    if(semctl(semId, ACCESS, GETVAL, 0) == 0)
    {
        SemUp(semId, MUTEX);
        SemDown(semId, ACCESS);
    }
    else
    {
        SemDown(semId, ACCESS);
        SemUp(semId, MUTEX);
    }

    uint32_t begin = files[index].dataBegin;
    uint32_t end = begin + BlocksNumber(files[index].size);

    for(uint i = begin; i < end; ++i)
        bitmap[i] = false;

    files[index].name = std::string();
    files[index].size = 0;

    WriteSuperblock();

    sleep(1);
    std::cout << "File: " << fileName << " removed" << std::endl;
    SemUp(semId, ACCESS);
}

void FileSystem::ListFiles() throw (std::string)
{
    Exist();
    ReadSuperblock();

    std::cout << "Name\t\t\tSize" << std::endl;

    for(uint i = 0; i < FILES_NO; ++i)
    {
        if(files[i].size != 0)
            std::cout << files[i].name << "\t" << files[i].size << std::endl;
    }
}

void FileSystem::ListMemory() throw (std::string)
{
    Exist();
    ReadSuperblock();

    std::cout << "\n\tSUPERBLOCK:" << std::endl;
    std::cout << "Partition size\t" << sizeof(size) << "\n" <<
                 "Nodes table\t" << sizeof(Node)*FILES_NO << "\n" <<
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

void FileSystem::ReadFile(std::string &fileName) throw (std::string)
{
    Exist();
    ReadSuperblock();
    FindFile(fileName);

    int semId = CreateSemaphore(fileName);

    SemDown(semId, MUTEX);
    if(semctl(semId, COUNTER, GETVAL, 0) == 0)
    {
        if(semctl(semId, ACCESS, GETVAL, 0) == 1)
        {
            SemDown(semId, ACCESS);
            SemUp(semId, MUTEX);
        }
        else
        {
            SemUp(semId, MUTEX);
            SemDown(semId, ACCESS);
        }
    }

    SemUp(semId, COUNTER);

    ReadSuperblock();
    try
    {
        FindFile(fileName);
    }
    catch (std::string e)
    {
        SemUp(semId, ACCESS);
        throw e;
    }

    std::cout << "Reading the file: " << fileName << std::endl;
    sleep(1);

    SemDown(semId, MUTEX);
    SemDown(semId, COUNTER);
    if(semctl(semId, COUNTER, GETVAL, 0) == 0)
        SemUp(semId, ACCESS);
    SemUp(semId, MUTEX);
}

void FileSystem::Exist() throw(std::string)
{
    if (!exist)
        throw std::string("Partition does not exist!\nUse -create.\n");
}

void FileSystem::NotExist() throw(std::string)
{
    if (exist)
        throw std::string("Partition already exists!\nEdit or destroy existing partition.\n");
}

void FileSystem::ReadSuperblock() throw (std::string)
{
    partition.seekg(0, partition.beg);
    partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));

    bitmap.reset(new bool[BlocksNumber()]);
    files.reset(new Node[FILES_NO]);

    for(uint i = 0; i < FILES_NO; ++i)
    {
        std::stringstream ss;
        char tempName[16] = "";
        partition.read(reinterpret_cast<char *>(&tempName), sizeof(tempName));
        partition.read(reinterpret_cast<char *>(&(files[i].size)), sizeof(uint32_t));
        partition.read(reinterpret_cast<char *>(&(files[i].dataBegin)), sizeof(uint32_t));

        ss << tempName;
        ss >> files[i].name;
    }

    int blocksNo = size/BLOCK_SIZE;

    for(int i = 0; i < blocksNo; ++i)
        partition.read(reinterpret_cast<char *>(&bitmap[i]), sizeof(bool));

}

void FileSystem::WriteSuperblock() throw (std::string)
{
    partition.seekp(0, partition.beg);
    partition.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));

    for(uint i = 0; i < FILES_NO; ++i)
    {
        std::stringstream ss;
        char tempName[16] = "";
        ss << files[i].name;
        ss >> tempName;
        partition.write(reinterpret_cast<const char *>(&tempName), sizeof(tempName));
        partition.write(reinterpret_cast<const char *>(&(files[i].size)), sizeof(uint32_t));
        partition.write(reinterpret_cast<const char *>(&(files[i].dataBegin)), sizeof(uint32_t));
    }

    int blocksNo = BlocksNumber();

    for(int i = 0; i < blocksNo; ++i)
    {
        partition.write(reinterpret_cast<const char *>(&bitmap[i]), sizeof(bool));
    }
}

void FileSystem::WriteEmptyData()
{
    int blocksNo = size/BLOCK_SIZE;

    char temp[BLOCK_SIZE] = "";
    for(int i = 0; i < blocksNo; ++i)
        partition.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
}

uint32_t FileSystem::FindPlace(uint32_t fileSize) throw (std::string)
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

int32_t FileSystem::FindFile(std::string &fileName) throw (std::string)
{
    for(int32_t i = 0; i < FILES_NO; ++i)
    {
        if(files[i].name == fileName)
            return i;
    }

    throw std::string("There is no file with this name!\n");
}

int FileSystem::CreateSemaphore(std::string &fileName) throw (std::string)
{
    uint32_t hash = 0;
    bool init = false;

    for(uint i = 0; i < fileName.size(); ++i)
        hash += fileName[i];

    int tempId = semget(hash, 3, IPC_CREAT|IPC_EXCL|0600);
    if (tempId == -1)
    {
        tempId = semget(hash, 2, 0600);
        init = true;

        if (tempId == -1)
            throw std::string("Can't create semaphores array\n");
    }

    if(!init)
    {
        SemUp(tempId, ACCESS);
        SemUp(tempId, MUTEX);
    }

    semaphores.push_back(tempId);
    return tempId;
}

int FileSystem::SemUp(uint32_t semId, uint32_t semNum)
{
    struct sembuf buf;
    buf.sem_num = semNum;
    buf.sem_op = 1;
    buf.sem_flg = 0;

    return semop(semId, &buf, 1);
}

int FileSystem::SemDown(uint32_t semId, uint32_t semNum)
{
    struct sembuf buf;
    buf.sem_num = semNum;
    buf.sem_op = -1;
    buf.sem_flg = 0;

    return semop(semId, &buf, 1);
}
