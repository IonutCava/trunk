################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Scenes/Scene.cpp \
/home/ionut/Divide/Source\ Code/Scenes/SceneInput.cpp \
/home/ionut/Divide/Source\ Code/Scenes/SceneState.cpp 

OBJS += \
./Scenes/Scene.o \
./Scenes/SceneInput.o \
./Scenes/SceneState.o 

CPP_DEPS += \
./Scenes/Scene.d \
./Scenes/SceneInput.d \
./Scenes/SceneState.d 


# Each subdirectory must supply rules for building sources it contributes
Scenes/Scene.o: /home/ionut/Divide/Source\ Code/Scenes/Scene.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Scenes/SceneInput.o: /home/ionut/Divide/Source\ Code/Scenes/SceneInput.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Scenes/SceneState.o: /home/ionut/Divide/Source\ Code/Scenes/SceneState.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


