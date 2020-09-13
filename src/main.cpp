//  Author:  Noah Himed
//  Date:    23 March 2020
//  Summary: Uses the Encoder class to either compress
//           or decompress a user-supplied file path.

#include "encoder.h"
#include "constants.h"

#include <iostream>
#include <string>
#include <exception>

using namespace std;

int main(int argc, const char* argv[])
{
    try
    {
        if(argc != 2)
            throw std::invalid_argument("Incorrect number of arguments, only enter one valid path");
        
        std::string file_path = argv[1];
        
        Encoder fileManipulator(file_path);
        
        if(fileManipulator.GetFileExt() == ORIGINAL_FILE_EXT)
            fileManipulator.Compress();
        else
            fileManipulator.Decompress();
    }
    catch (exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}

