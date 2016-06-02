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
    if(!strcmp(argv[1], "-create"))
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
            fileSystem.~FileSystem();
        }

    }

    //DESTROY PARTITION
    if(!strcmp(argv[1], "-destroy"))
    {
        try
        {
            fileSystem.Destroy();
        }
        catch(std::string &e)
        {
            std::cout << e;
            fileSystem.~FileSystem();
        }
    }

    //LIST FILES
    if(!strcmp(argv[1], "-ls"))
    {
        try
        {
            fileSystem.ListFiles();
        }
        catch (std::string e)
        {
            std::cout << e;
        }
    }

    //LIST MEMORY
    if(!strcmp(argv[1], "-lm"))
    {
        try
        {
            fileSystem.ListMemory();
        }
        catch (std::string e)
        {
            std::cout << e;
        }
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
            fileSystem.~FileSystem();
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
            fileSystem.~FileSystem();
        }

    }

    if(!strcmp(argv[1], "-d"))
    {
        if (argc <= 2)
        {
            std::cout << "Type name of file to download" << std::endl;
            return 0;
        }

        std::stringstream ss;
        ss << argv[2];
        std::string fileName;
        ss >> fileName;

        try
        {
            fileSystem.Download(fileName);
        }
        catch (std::string &e)
        {
            std::cout << e;
            fileSystem.~FileSystem();
        }

    }

    if(!strcmp(argv[1], "-read"))
    {
        if (argc <= 2)
        {
            std::cout << "Type name of file to read" << std::endl;
            return 0;
        }

        std::stringstream ss;
        ss << argv[2];
        std::string fileName;
        ss >> fileName;

        try
        {
            fileSystem.ReadFile(fileName);
        }
        catch (std::string &e)
        {
            std::cout << e;
            fileSystem.~FileSystem();
        }

    }

    return 0;

}
