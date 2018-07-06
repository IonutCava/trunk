#include "Headers/OpenCLInterface.h"
#include "Core/Headers/Console.h"

//ref: https://github.com/lilleyse/Mass-Occlusion-Culling
namespace Divide {

OpenCLInterface::OpenCLInterface() : _clPlatform(nullptr),
                                     _clGPUContext(nullptr),
                                     _clDevice(nullptr),
                                     _clCommandQueue(nullptr),
                                     _localWorkSize(256)
{
}

OpenCLInterface::~OpenCLInterface()
{
}

bool OpenCLInterface::init() {
    bool state = true;
    //Get an OpenCL platform
    cl_int clError = clGetPlatformIDs(1, &_clPlatform, NULL);

    if (clError != CL_SUCCESS) {
        Console::errorfn("Could not create platform");
        state = false;
    } else {
        //Get the device - for now just assume that the device supports sharing with OpenGL
        clError = clGetDeviceIDs(_clPlatform, CL_DEVICE_TYPE_GPU, 1, &_clDevice, NULL);

        if (clError != CL_SUCCESS) {
            Console::errorfn("Could not get a GPU device on the platform");
            state = false;
        } else {

            //Create the context, with support for sharing with OpenGL 
            cl_context_properties props[] =
            {
                CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
                CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
                CL_CONTEXT_PLATFORM, (cl_context_properties)_clPlatform,
                0
            };
            _clGPUContext = clCreateContext(props, 1, &_clDevice, NULL, NULL, &clError);

            if (clError != CL_SUCCESS) {
                Console::errorfn("Could not create a context");
                state = false;
            } else {
                // Create a command-queue
                _clCommandQueue = clCreateCommandQueue(_clGPUContext, _clDevice, 0, &clError);
                if (clError != CL_SUCCESS) {
                    Console::errorfn("Could not create command queue");
                    state = false;
                }
            }
        }
    }

    if (!state) {
        return deinit();
    }

    return state;
}

bool OpenCLInterface::deinit() {
    if (_clGPUContext) {
        clReleaseContext(_clGPUContext);
    }
        
    return true;
}

bool OpenCLInterface::loadProgram(const stringImpl& programPath) {
    size_t programLength;
    cl_program clProgram = nullptr;
    cl_kernel clKernel = nullptr;
    bool state = true;
    char* cSourceCL = loadProgramSource(programPath.c_str(), &programLength);
    if (cSourceCL == NULL) {
        Console::errorfn("Could not load program source");
        state = false;
    } else {

        // create the program
        cl_int clError;
        clProgram = clCreateProgramWithSource(_clGPUContext, 1, (const char **)&cSourceCL, &programLength, &clError);
        if (clError != CL_SUCCESS) {
            Console::errorfn("Could not create program");
            state = false;
        } else {

            // build the program
            clError = clBuildProgram(clProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
            if (clError != CL_SUCCESS)
            {
                Console::errorfn("Could not build program");
                char cBuildLog[10240];
                clGetProgramBuildInfo(clProgram, _clDevice, CL_PROGRAM_BUILD_LOG, sizeof(cBuildLog), cBuildLog, NULL);
                Console::errorfn("%s",  cBuildLog);
                state = false;
            } else {

                // create the kernel
                clKernel = clCreateKernel(clProgram, "pass_along", &clError);
                if (clError != CL_SUCCESS) {
                    Console::errorfn("Could not create kernel");
                    state = false;
                }
            }
        }
    }

    return state;
}

//from the Nvidia OpenCL utils
char* OpenCLInterface::loadProgramSource(const char* cFilename, size_t* szFinalLength)
{
    // locals 
    FILE* pFileStream = NULL;
    size_t szSourceLength;

    if (fopen_s(&pFileStream, cFilename, "rb") != 0)
    {
        return NULL;
    }

    // get the length of the source code
    fseek(pFileStream, 0, SEEK_END);
    szSourceLength = ftell(pFileStream);
    fseek(pFileStream, 0, SEEK_SET);

    // allocate a buffer for the source code string and read it in
    char* cSourceString = (char *)malloc(szSourceLength + 1);
    if (fread((cSourceString), szSourceLength, 1, pFileStream) != 1)
    {
        fclose(pFileStream);
        free(cSourceString);
        return 0;
    }

    // close the file and return the total length of the string
    fclose(pFileStream);
    if (szFinalLength != 0)
    {
        *szFinalLength = szSourceLength;
    }
    cSourceString[szSourceLength] = '\0';

    return cSourceString;
}

}; //namespace Divide