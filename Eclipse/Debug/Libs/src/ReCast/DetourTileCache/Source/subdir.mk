################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.cpp 

OBJS += \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.o \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.o 

CPP_DEPS += \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.d \
./Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCache.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code" -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/ReCast/DetourTileCache/Source/DetourTileCacheBuilder.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code" -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


