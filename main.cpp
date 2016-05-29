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
        catch(std::string e)
        {
            std::cout << e;
        }

    }

    if(!strcmp(argv[1], "-destroy"))
    {
        try
        {
            fileSystem.Destroy();
        }
        catch(std::string e)
        {
            std::cout << e;
        }
    }

    return 0;

}
