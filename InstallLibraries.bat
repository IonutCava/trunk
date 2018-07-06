@echo off
echo Downloading third party libraries

if "%~1"=="" (
	set libraryLocation=%~dp0..\Libraries
) else (
	set libraryLocation=%1
)

mkdir %libraryLocation%
cd %libraryLocation%
svn cleanup
svn checkout https://xp-dev.com/svn/Divide-Dependencies %libraryLocation%
echo Setting $(EngineLibraries) system variable to %libraryLocation%
setx EngineLibraries %libraryLocation%
echo Downloading PhysX SDK (manually register and download version 3.2.4)
start explorer "http://supportcenteronline.com/ics/support/mylogin.asp?splash=1"
echo Copy the contents of the SDK to "$(EngineLibraries)/physx"
pause