################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMesh.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMeshContext.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMeshDebugDraw.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMeshLoader.cpp 

OBJS += \
./AI/PathFinding/NavMeshes/NavMesh.o \
./AI/PathFinding/NavMeshes/NavMeshContext.o \
./AI/PathFinding/NavMeshes/NavMeshDebugDraw.o \
./AI/PathFinding/NavMeshes/NavMeshLoader.o 

CPP_DEPS += \
./AI/PathFinding/NavMeshes/NavMesh.d \
./AI/PathFinding/NavMeshes/NavMeshContext.d \
./AI/PathFinding/NavMeshes/NavMeshDebugDraw.d \
./AI/PathFinding/NavMeshes/NavMeshLoader.d 


# Each subdirectory must supply rules for building sources it contributes
AI/PathFinding/NavMeshes/NavMesh.o: /home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMesh.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

AI/PathFinding/NavMeshes/NavMeshContext.o: /home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMeshContext.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

AI/PathFinding/NavMeshes/NavMeshDebugDraw.o: /home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMeshDebugDraw.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

AI/PathFinding/NavMeshes/NavMeshLoader.o: /home/ionut/Custom/Divide/trunk/Source\ Code/AI/PathFinding/NavMeshes/NavMeshLoader.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


