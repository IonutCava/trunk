################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefines.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefinesApple.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefinesUnix.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefinesWindows.cpp 

OBJS += \
./Platform/Platform/PlatformDefines.o \
./Platform/Platform/PlatformDefinesApple.o \
./Platform/Platform/PlatformDefinesUnix.o \
./Platform/Platform/PlatformDefinesWindows.o 

CPP_DEPS += \
./Platform/Platform/PlatformDefines.d \
./Platform/Platform/PlatformDefinesApple.d \
./Platform/Platform/PlatformDefinesUnix.d \
./Platform/Platform/PlatformDefinesWindows.d 


# Each subdirectory must supply rules for building sources it contributes
Platform/Platform/PlatformDefines.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefines.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Platform/PlatformDefinesApple.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefinesApple.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Platform/PlatformDefinesUnix.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefinesUnix.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Platform/PlatformDefinesWindows.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Platform/Platform/PlatformDefinesWindows.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


