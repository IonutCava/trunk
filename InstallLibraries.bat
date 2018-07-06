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
echo Downloading PhysX SDK
echo ToDo
pause