################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Core/Resources/Resource.cpp \
/home/ionut/Custom/trunk/Source\ Code/Core/Resources/ResourceCache.cpp \
/home/ionut/Custom/trunk/Source\ Code/Core/Resources/ResourceDescriptor.cpp 

OBJS += \
./Core/Resources/Resource.o \
./Core/Resources/ResourceCache.o \
./Core/Resources/ResourceDescriptor.o 

CPP_DEPS += \
./Core/Resources/Resource.d \
./Core/Resources/ResourceCache.d \
./Core/Resources/ResourceDescriptor.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Resources/Resource.o: /home/ionut/Custom/trunk/Source\ Code/Core/Resources/Resource.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost/ -I/home/ionut/Custom/Libraries/OpenAL/include/ -I/home/ionut/Custom/Libraries/cegui/include/ -I/home/ionut/Custom/Libraries/physx/ -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/Threadpool/include -I/home/ionut/Custom/Libraries/SimpleINI/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Core/Resources/ResourceCache.o: /home/ionut/Custom/trunk/Source\ Code/Core/Resources/ResourceCache.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost/ -I/home/ionut/Custom/Libraries/OpenAL/include/ -I/home/ionut/Custom/Libraries/cegui/include/ -I/home/ionut/Custom/Libraries/physx/ -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/Threadpool/include -I/home/ionut/Custom/Libraries/SimpleINI/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Core/Resources/ResourceDescriptor.o: /home/ionut/Custom/trunk/Source\ Code/Core/Resources/ResourceDescriptor.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost/ -I/home/ionut/Custom/Libraries/OpenAL/include/ -I/home/ionut/Custom/Libraries/cegui/include/ -I/home/ionut/Custom/Libraries/physx/ -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/Threadpool/include -I/home/ionut/Custom/Libraries/SimpleINI/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

