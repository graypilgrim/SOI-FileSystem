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

    semId = semget(ID, 1, IPC_CREAT|IPC_EXCL|0600);
    if (semId == -1)
    {
        semId = semget(ID, FILES_NO, 0600);

        if (semId == -1)
            throw std::string("Can't create semaphores array\n");
    }

    SemUp(ID, FILEACCESS);

    WriteSuperblock();
    WriteEmptyData();

    std::cout << "Partition has been created" << std::endl;
}

void FileSystem::Destroy() throw (std::string)
{
    Exist();
    ReadSuperblock();

    semctl(semId, 0, IPC_RMID, 0);
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

    std::cout << "dataBegin " << dataBegin << std::endl;

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
    files[index].dataBegin = begin;

    char temp[BLOCK_SIZE];

    newFile.seekg(0, newFile.beg);

    uint32_t blocksNo = BlocksNumber(newFileSize);

    std::cout << "superb: " << SuperblockSize() << std::endl;

    partition.seekp(SuperblockSize() + BLOCK_SIZE*dataBegin, partition.beg);
    std::streampos partitionIter = partition.tellp();
    std::cout << "partIt: " << partitionIter << std::endl;
    newFile.seekg(0, newFile.beg);

    SemDown(ID, FILEACCESS);
    for(uint i = 0; i < blocksNo; ++i)
    {
        newFile.read(reinterpret_cast<char *>(temp), BLOCK_SIZE*sizeof(char));
        partition.write(reinterpret_cast<const char *>(temp), BLOCK_SIZE*sizeof(char));
        bitmap[dataBegin+i] = true;
    }
    SemUp(ID, FILEACCESS);

    newFile.close();

    WriteSuperblock();

    std::cout << "The file has been added" << std::endl;
}

void FileSystem::Download(std::string &fileName) throw (std::string)
{
    Exist();
    ReadSuperblock();

    int32_t index = FindFile(fileName);

    std::fstream file(fileName, std::fstream::out | std::fstream::binary);

    char *temp = new char[files[index].size];

    uint32_t dataBegin = files[index].dataBegin;

    partition.seekg(SuperblockSize() + BLOCK_SIZE*dataBegin, partition.beg);

    SemDown(ID, FILEACCESS);
    partition.read(reinterpret_cast<char *>(temp), files[index].size*sizeof(char));
    file.write(reinterpret_cast<const char *>(temp), files[index].size*sizeof(char));
    SemUp(ID, FILEACCESS);

    delete[] temp;

    file.close();
}

void FileSystem::DeleteFile(std::string &fileName) throw (std::string)
{
    Exist();
    ReadSuperblock();

    int32_t index = FindFile(fileName);

    uint32_t begin = files[index].dataBegin;
    uint32_t end = begin + BlocksNumber(files[index].size);

    for(uint i = begin; i < end; ++i)
        bitmap[i] = false;

    files[index].name = std::string();
    files[index].size = 0;

    WriteSuperblock();

    sleep(1);
    std::cout << "File: " << fileName << " removed" << std::endl;
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
    std::cout << "Partition size\t" << sizeof(size) << " B\n" <<
                 "Semaphore id\t" << sizeof(semId) << " B\n" <<
                 "Nodes table\t" << sizeof(Node)*FILES_NO << " B\n" <<
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

    std::cout << "Reading the file: " << fileName << std::endl;
    sleep(1);
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
    SemDown(ID, FILEACCESS);
    partition.seekg(0, partition.beg);
    partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));
    partition.read(reinterpret_cast<char *>(&semId), sizeof(uint32_t));

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

    SemUp(ID, FILEACCESS);
}

void FileSystem::WriteSuperblock() throw (std::string)
{
    SemDown(ID, FILEACCESS);
    partition.seekp(0, partition.beg);
    partition.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    partition.write(reinterpret_cast<const char *>(&semId), sizeof(uint32_t));

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
    SemUp(ID, FILEACCESS);
}

void FileSystem::WriteEmptyData()
{
    SemDown(ID, FILEACCESS);
    int blocksNo = size/BLOCK_SIZE;

    char temp[BLOCK_SIZE] = "";
    for(int i = 0; i < blocksNo; ++i)
        partition.write(reinterpret_cast<const char *>(&temp), sizeof(temp));

    SemUp(ID, FILEACCESS);
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
    uint32_t result = 0;
    result += sizeof(size);
    result += sizeof(semId);
    result += FILES_NO*(16*sizeof(char) + 2*sizeof(uint32_t));
    result += BlocksNumber()*sizeof(bool);
    return result;
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
