#include "FileSystem.h"

FileSystem::FileSystem()
{
    partition = std::fstream(NAME, std::fstream::in | std::fstream::out | std::fstream::binary);

    bitmap.reset(new bool[size / 128]);

    if (partition.good())
    {
        exist = true;
        //partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));
        //partition.read(reinterpret_cast<char *>(&filesNo), sizeof(uint32_t));
    }
    else
    {
        exist = false;
    }

}

FileSystem::~FileSystem()
{
    partition.close();
    files.clear();

//    semctl(semId, (int)0, IPC_RMID, (int)0);
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

    this->filesNo = 0;

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
    partition.write(reinterpret_cast<const char *>(&filesNo), sizeof(uint32_t));

    for(auto it : files)
    {
        std::stringstream ss;
        char tempName[16] = "";
        ss << it.name;
        ss >> tempName;
        partition.write(reinterpret_cast<const char *>(&tempName), sizeof(tempName));
        partition.write(reinterpret_cast<const char *>(&(it.size)), sizeof(uint32_t));
        partition.write(reinterpret_cast<const char *>(&(it.dataBegin)), sizeof(uint32_t));
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
/*
    semId = semget(SEM_ARRAY, FILES_NO, IPC_CREAT|IPC_EXCL|0600);
    if (semId == -1){
        semId = semget(SEM_ARRAY, FILES_NO, 0600);
        init = false;
        if (semId == -1){
            throw std::string("Can't create semaphores array\n");
        }
    }
*/
    partition.seekg(0, partition.beg);
    partition.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));
    partition.read(reinterpret_cast<char *>(&filesNo), sizeof(uint32_t));

    for(uint i = 0; i < filesNo; ++i)
    {
        Node node;
        std::stringstream ss;
        char tempName[16] = "";
        partition.read(reinterpret_cast<char *>(&tempName), sizeof(tempName));
        partition.read(reinterpret_cast<char *>(&(node.size)), sizeof(uint32_t));
        partition.read(reinterpret_cast<char *>(&(node.dataBegin)), sizeof(uint32_t));

        ss << tempName;
        ss >> node.name;

        files.push_back(node);

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

    for(auto node : files)
    {
        std::cout << node.name << "\t" << node.size << std::endl;
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

    for(auto node : files)
    {
        if(node.name == fileName)
            throw std::string("File name must be unique!\n");

    }

    Node node;


    node.name = fileName;
    node.size = newFileSize;
    node.dataBegin = dataBegin;

    files.push_back(node);
    ++filesNo;

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

void FileSystem::DeleteFile(std::string &fileName)
{
    ReadSuperblock();

    std::list<Node>::iterator it = FindFile(fileName);

    //SemDown(semId, index);

    uint32_t begin = it->dataBegin;
    uint32_t end = it->dataBegin + BlocksNumber(it->size);

    for(uint i = begin; i < end; ++i)
        bitmap[i] = false;

    files.erase(it);
    --filesNo;

    WriteSuperblock();

    //SemUp(semId, index);
}

std::list<Node>::iterator FileSystem::FindFile(std::string &fileName)
{
    for(auto it = files.begin(); it != files.end(); ++it)
    {
        if(it->name == fileName)
            return it;
    }

    throw std::string("There is no file with this name!\n");
}

void FileSystem::Download(std::string &fileName)
{
    ReadSuperblock();

    std::list<Node>::iterator it;

    try
    {
        it = FindFile(fileName);
    }
    catch (std::string &e)
    {
        throw e;
    }

    //SemDown(semId, index);

    std::fstream file(fileName, std::fstream::out | std::fstream::binary);

    char *temp = new char[it->size];

    uint32_t begin = it->dataBegin;

    partition.seekp(SuperblockSize() + BLOCK_SIZE*begin, partition.beg);

    partition.read(reinterpret_cast<char *>(&temp), sizeof(temp));
    file.write(reinterpret_cast<const char *>(&temp), sizeof(temp));

    file.close();

    //SemUp(semId, index);
}

void FileSystem::ReadFile(std::string &fileName)
{
    ReadSuperblock();

    std::list<Node>::iterator it = FindFile(fileName);

    //SemDown(semId, index);
    //sleep(20);
    //SemUp(semId, index);

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
