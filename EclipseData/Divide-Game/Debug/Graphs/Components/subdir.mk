################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Graphs/Components/AnimationComponent.cpp \
/home/ionut/Custom/trunk/Source\ Code/Graphs/Components/NavigationComponent.cpp \
/home/ionut/Custom/trunk/Source\ Code/Graphs/Components/PhysicsComponent.cpp \
/home/ionut/Custom/trunk/Source\ Code/Graphs/Components/RenderingComponent.cpp \
/home/ionut/Custom/trunk/Source\ Code/Graphs/Components/SGNComponent.cpp 

OBJS += \
./Graphs/Components/AnimationComponent.o \
./Graphs/Components/NavigationComponent.o \
./Graphs/Components/PhysicsComponent.o \
./Graphs/Components/RenderingComponent.o \
./Graphs/Components/SGNComponent.o 

CPP_DEPS += \
./Graphs/Components/AnimationComponent.d \
./Graphs/Components/NavigationComponent.d \
./Graphs/Components/PhysicsComponent.d \
./Graphs/Components/RenderingComponent.d \
./Graphs/Components/SGNComponent.d 


# Each subdirectory must supply rules for building sources it contributes
Graphs/Components/AnimationComponent.o: /home/ionut/Custom/trunk/Source\ Code/Graphs/Components/AnimationComponent.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Graphs/Components/NavigationComponent.o: /home/ionut/Custom/trunk/Source\ Code/Graphs/Components/NavigationComponent.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Graphs/Components/PhysicsComponent.o: /home/ionut/Custom/trunk/Source\ Code/Graphs/Components/PhysicsComponent.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Graphs/Components/RenderingComponent.o: /home/ionut/Custom/trunk/Source\ Code/Graphs/Components/RenderingComponent.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Graphs/Components/SGNComponent.o: /home/ionut/Custom/trunk/Source\ Code/Graphs/Components/SGNComponent.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


