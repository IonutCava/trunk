################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/Recast.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastAlloc.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastArea.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastContour.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastFilter.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastLayers.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastMesh.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastMeshDetail.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastRasterization.cpp \
/home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastRegion.cpp 

OBJS += \
./Libs/ReCast/ReCast/Source/Recast.o \
./Libs/ReCast/ReCast/Source/RecastAlloc.o \
./Libs/ReCast/ReCast/Source/RecastArea.o \
./Libs/ReCast/ReCast/Source/RecastContour.o \
./Libs/ReCast/ReCast/Source/RecastFilter.o \
./Libs/ReCast/ReCast/Source/RecastLayers.o \
./Libs/ReCast/ReCast/Source/RecastMesh.o \
./Libs/ReCast/ReCast/Source/RecastMeshDetail.o \
./Libs/ReCast/ReCast/Source/RecastRasterization.o \
./Libs/ReCast/ReCast/Source/RecastRegion.o 

CPP_DEPS += \
./Libs/ReCast/ReCast/Source/Recast.d \
./Libs/ReCast/ReCast/Source/RecastAlloc.d \
./Libs/ReCast/ReCast/Source/RecastArea.d \
./Libs/ReCast/ReCast/Source/RecastContour.d \
./Libs/ReCast/ReCast/Source/RecastFilter.d \
./Libs/ReCast/ReCast/Source/RecastLayers.d \
./Libs/ReCast/ReCast/Source/RecastMesh.d \
./Libs/ReCast/ReCast/Source/RecastMeshDetail.d \
./Libs/ReCast/ReCast/Source/RecastRasterization.d \
./Libs/ReCast/ReCast/Source/RecastRegion.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/ReCast/ReCast/Source/Recast.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/Recast.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastAlloc.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastAlloc.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastArea.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastArea.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastContour.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastContour.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastFilter.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastFilter.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastLayers.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastLayers.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastMesh.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastMesh.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastMeshDetail.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastMeshDetail.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastRasterization.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastRasterization.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/ReCast/ReCast/Source/RecastRegion.o: /home/ionut/Custom/trunk/Source\ Code/Libs/ReCast/ReCast/Source/RecastRegion.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


