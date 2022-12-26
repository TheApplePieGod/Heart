#include "hepch.h"
#include "Utils.h"

#import <Cocoa/Cocoa.h>

namespace MacOS
{
    void Utils::SetCorrectWorkingDirectory()
    {
        NSBundle* main = [NSBundle mainBundle];
        NSString* resourcePath = [main resourcePath];
        NSFileManager* fileManager = [NSFileManager defaultManager];
        [fileManager changeCurrentDirectoryPath: resourcePath];
    }
}