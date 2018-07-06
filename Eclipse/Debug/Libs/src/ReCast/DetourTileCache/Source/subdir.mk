################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.cpp \
/home/ionut/Divide/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.cpp 

OBJS += \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.o \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.o 

CPP_DEPS += \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.d \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.o: /home/ionut/Divide/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


