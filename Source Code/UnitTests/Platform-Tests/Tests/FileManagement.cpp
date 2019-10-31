#include "stdafx.h"

#include "Headers/Defines.h"

#include "Core/Headers/StringHelper.h"
#include "Platform/File/Headers/FileManagement.h"

TEST(FileExistanceCheck)
{
    const char* invalidFileName = "abc.cba";

    if (PreparePlatform()) {
        const Divide::SysInfo& systemInfo = Divide::const_sysInfo();
        const char* exeName = systemInfo._pathAndFilename._fileName.c_str();

        CHECK_TRUE(Divide::fileExists(exeName));
        CHECK_FALSE(Divide::fileExists(invalidFileName));
    }
}

TEST(PathExistanceCheck)
{
    const char* invalidPath = "abccba";

    if (PreparePlatform()) {
        CHECK_TRUE(Divide::pathExists((Divide::Paths::g_exePath + Divide::Paths::g_assetsLocation).c_str()));
        CHECK_FALSE(Divide::pathExists(invalidPath));
    }
}

TEST(ExtensionCheck)
{
    const char* file1 = "temp.xyz";
    const char* file2 = "folder/temp.st";
    const char* file3 = "folder/temp";
    const char* ext1 = "xyz";
    const char* ext2 = "st";

    if (PreparePlatform()) {
        CHECK_TRUE(Divide::hasExtension(file1, ext1));
        CHECK_TRUE(Divide::hasExtension(file2, ext2));
        CHECK_FALSE(Divide::hasExtension(file1, ext2));
        CHECK_FALSE(Divide::hasExtension(file3, ext2));
    }
}