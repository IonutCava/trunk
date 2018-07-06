@echo off
echo Downloading third party libraries

if "%~1"=="" (
	set libraryLocation=%~dp0..\Libraries
) else (
	set libraryLocation=%1
)

IF NOT EXIST %libraryLocation% mkdir %libraryLocation%
cd %libraryLocation%

@echo off
svn >nul 2>nul
if %ERRORLEVEL% neq 9009 goto continue
echo Subversion "svn" command not found. Please install Subversion first..
pause
exit 1

:continue
svn cleanup
svn checkout https://xp-dev.com/svn/Divide-Dependencies %libraryLocation%
echo Setting $(EngineLibraries) system variable to %libraryLocation%
setx EngineLibraries %libraryLocation%
IF NOT EXIST physx mkdir physx
echo Downloading PhysX SDK (manually register and download the newest version)
echo Build all of the projects in DEBUG, CHECKED and RELEASE mode
echo Remember to update the PhysX DLL in the working directory with the newly built variants
start explorer "https://developer.nvidia.com/physx-source-github"
echo Copy the contents of the SDK to "$(EngineLibraries)/physx"
echo Make sure you have the OpenAL runtime installed as well: https://www.openal.org/downloads/
pause