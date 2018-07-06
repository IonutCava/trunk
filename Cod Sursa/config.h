/**************************************************************************************************************\
*                             Welcome to <ToDo> Engine's config settings!                                     *
* From here you can decide how you want to build your release of the code.
* - Configure if you want to you a PhysX generated terrain or an openGl generated one
* - Decide how many threads your application can use (maximum upper limit).
* - Show the debug ms-dos window that prints application information
* - Define what the console output log file should be named
* - 
* - 
***************************************************************************************************************/

//Edit the maximum number of concurrent threads that this application may start.
//Default 2: Rendering + PhysX
#define THREAD_LIMIT 2

//Comment this out to show the debug console
#define HIDE_DEBUG_CONSOLE

//Please enter the desired log file name
#define OUTPUT_LOG_FILE "console.log"

