@echo off

set libraryFolder=3rdParty
set libraryLocation=%~dp0%libraryFolder%
set physxLocation=%libraryLocation%\physx

IF NOT EXIST %libraryLocation% mkdir %libraryLocation%

echo Downloading third party libraries into folder: %libraryLocation%

svn >nul 2>nul
if %ERRORLEVEL% neq 9009 goto continue
echo Subversion "svn" command not found. Please install Subversion first..
pause
exit 1

:continue
svn cleanup
svn checkout https://xp-dev.com/svn/Divide-Dependencies %libraryFolder%

pause