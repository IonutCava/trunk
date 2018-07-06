################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourAlloc.cpp \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourCommon.cpp \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNavMesh.cpp \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNavMeshBuilder.cpp \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNavMeshQuery.cpp \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNode.cpp 

OBJS += \
./Libs/src/ReCast/Detour/Source/DetourAlloc.o \
./Libs/src/ReCast/Detour/Source/DetourCommon.o \
./Libs/src/ReCast/Detour/Source/DetourNavMesh.o \
./Libs/src/ReCast/Detour/Source/DetourNavMeshBuilder.o \
./Libs/src/ReCast/Detour/Source/DetourNavMeshQuery.o \
./Libs/src/ReCast/Detour/Source/DetourNode.o 

CPP_DEPS += \
./Libs/src/ReCast/Detour/Source/DetourAlloc.d \
./Libs/src/ReCast/Detour/Source/DetourCommon.d \
./Libs/src/ReCast/Detour/Source/DetourNavMesh.d \
./Libs/src/ReCast/Detour/Source/DetourNavMeshBuilder.d \
./Libs/src/ReCast/Detour/Source/DetourNavMeshQuery.d \
./Libs/src/ReCast/Detour/Source/DetourNode.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/src/ReCast/Detour/Source/DetourAlloc.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourAlloc.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/ReCast/Detour/Source/DetourCommon.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourCommon.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/ReCast/Detour/Source/DetourNavMesh.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNavMesh.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/ReCast/Detour/Source/DetourNavMeshBuilder.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNavMeshBuilder.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/ReCast/Detour/Source/DetourNavMeshQuery.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNavMeshQuery.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/ReCast/Detour/Source/DetourNode.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/Detour/Source/DetourNode.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


