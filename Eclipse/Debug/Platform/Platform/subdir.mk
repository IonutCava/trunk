################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefines.cpp \
/home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefinesApple.cpp \
/home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefinesUnix.cpp \
/home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefinesWindows.cpp 

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
Platform/Platform/PlatformDefines.o: /home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefines.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Platform/PlatformDefinesApple.o: /home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefinesApple.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Platform/PlatformDefinesUnix.o: /home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefinesUnix.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Platform/PlatformDefinesWindows.o: /home/ionut/Divide/Source\ Code/Platform/Platform/PlatformDefinesWindows.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

