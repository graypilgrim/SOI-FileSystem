#include "FileSystem.h"
#include <string.h>

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        std::cout << "Command need an argument" << std::endl;
        return 0;
    }

    FileSystem fileSystem;

    //CREATE PARTITION
    if(!strcmp(argv[1], "-c"))
    {
        if (argc <= 2)
        {
            std::cout << "Define partition size" << std::endl;
            return 0;
        }


        int32_t size = atoi(argv[2]);

        try
        {
            fileSystem.Create(size);
        }
        catch(std::string &e)
        {
            std::cout << e;
        }

    }

    //DESTROY PARTITION
    if(!strcmp(argv[1], "-d"))
    {
        try
        {
            fileSystem.Destroy();
        }
        catch(std::string &e)
        {
            std::cout << e;
        }
    }

    //LIST FILES
    if(!strcmp(argv[1], "-ls"))
    {
        fileSystem.ListFiles();
    }

    //LIST MEMORY
    if(!strcmp(argv[1], "-lm"))
    {
        fileSystem.ListMemory();
    }

    //UPLOAD FILE
    if(!strcmp(argv[1], "-u"))
    {
        if (argc <= 2)
        {
            std::cout << "Type name of file to upload" << std::endl;
            return 0;
        }

        std::stringstream ss;
        ss << argv[2];
        std::string fileName;
        ss >> fileName;

        try
        {
            fileSystem.Upload(fileName);
        }
        catch (std::string &e)
        {
            std::cout << e;
        }

    }

    //DELETE FILE
    if(!strcmp(argv[1], "-rm"))
    {
        if (argc <= 2)
        {
            std::cout << "Type name of file to delete" << std::endl;
            return 0;
        }

        std::stringstream ss;
        ss << argv[2];
        std::string fileName;
        ss >> fileName;

        try
        {
            fileSystem.DeleteFile(fileName);
        }
        catch (std::string &e)
        {
            std::cout << e;
        }

    }

    return 0;

}
