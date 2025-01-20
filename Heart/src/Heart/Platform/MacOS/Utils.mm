#include "hepch.h"
#include "Utils.h"

#import <Cocoa/Cocoa.h>

namespace MacOS
{
    bool Utils::IsAppPackaged()
    {
        NSBundle* main = [NSBundle mainBundle];
        if (main == nullptr)
            return false;

        NSString* execPath = [main executablePath];

        NSArray<NSString*>* pathComponents = execPath.pathComponents;

        // Check for ".app/Contents/MacOS"
        NSUInteger count = pathComponents.count;
        return (
            count >= 4 &&
            [pathComponents[count - 4] hasSuffix:@".app"] &&
            [pathComponents[count - 3] isEqualToString:@"Contents"] &&
            [pathComponents[count - 2] isEqualToString:@"MacOS"]
        );
    }

    void Utils::SetCorrectWorkingDirectory()
    {
        NSBundle* main = [NSBundle mainBundle];
        NSString* resourcePath = [main resourcePath];
        NSFileManager* fileManager = [NSFileManager defaultManager];
        [fileManager changeCurrentDirectoryPath: resourcePath];
    }

    std::string Utils::GetApplicationSupportDirectory()
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSString* dir = [paths firstObject];
        return std::string([dir UTF8String]);
    }
}
