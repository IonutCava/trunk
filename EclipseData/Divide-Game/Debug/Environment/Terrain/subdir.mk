################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Environment/Terrain/Terrain.cpp \
/home/ionut/Custom/trunk/Source\ Code/Environment/Terrain/TerrainChunk.cpp \
/home/ionut/Custom/trunk/Source\ Code/Environment/Terrain/TerrainLoader.cpp 

OBJS += \
./Environment/Terrain/Terrain.o \
./Environment/Terrain/TerrainChunk.o \
./Environment/Terrain/TerrainLoader.o 

CPP_DEPS += \
./Environment/Terrain/Terrain.d \
./Environment/Terrain/TerrainChunk.d \
./Environment/Terrain/TerrainLoader.d 


# Each subdirectory must supply rules for building sources it contributes
Environment/Terrain/Terrain.o: /home/ionut/Custom/trunk/Source\ Code/Environment/Terrain/Terrain.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Environment/Terrain/TerrainChunk.o: /home/ionut/Custom/trunk/Source\ Code/Environment/Terrain/TerrainChunk.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Environment/Terrain/TerrainLoader.o: /home/ionut/Custom/trunk/Source\ Code/Environment/Terrain/TerrainLoader.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


