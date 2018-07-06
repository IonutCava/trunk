################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Rendering/Camera/Camera.cpp \
/home/ionut/Divide/Source\ Code/Rendering/Camera/FirstPersonCamera.cpp \
/home/ionut/Divide/Source\ Code/Rendering/Camera/FreeFlyCamera.cpp \
/home/ionut/Divide/Source\ Code/Rendering/Camera/Frustum.cpp \
/home/ionut/Divide/Source\ Code/Rendering/Camera/OrbitCamera.cpp \
/home/ionut/Divide/Source\ Code/Rendering/Camera/ScriptedCamera.cpp \
/home/ionut/Divide/Source\ Code/Rendering/Camera/ThirdPersonCamera.cpp 

OBJS += \
./Rendering/Camera/Camera.o \
./Rendering/Camera/FirstPersonCamera.o \
./Rendering/Camera/FreeFlyCamera.o \
./Rendering/Camera/Frustum.o \
./Rendering/Camera/OrbitCamera.o \
./Rendering/Camera/ScriptedCamera.o \
./Rendering/Camera/ThirdPersonCamera.o 

CPP_DEPS += \
./Rendering/Camera/Camera.d \
./Rendering/Camera/FirstPersonCamera.d \
./Rendering/Camera/FreeFlyCamera.d \
./Rendering/Camera/Frustum.d \
./Rendering/Camera/OrbitCamera.d \
./Rendering/Camera/ScriptedCamera.d \
./Rendering/Camera/ThirdPersonCamera.d 


# Each subdirectory must supply rules for building sources it contributes
Rendering/Camera/Camera.o: /home/ionut/Divide/Source\ Code/Rendering/Camera/Camera.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/Camera/FirstPersonCamera.o: /home/ionut/Divide/Source\ Code/Rendering/Camera/FirstPersonCamera.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/Camera/FreeFlyCamera.o: /home/ionut/Divide/Source\ Code/Rendering/Camera/FreeFlyCamera.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/Camera/Frustum.o: /home/ionut/Divide/Source\ Code/Rendering/Camera/Frustum.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/Camera/OrbitCamera.o: /home/ionut/Divide/Source\ Code/Rendering/Camera/OrbitCamera.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/Camera/ScriptedCamera.o: /home/ionut/Divide/Source\ Code/Rendering/Camera/ScriptedCamera.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/Camera/ThirdPersonCamera.o: /home/ionut/Divide/Source\ Code/Rendering/Camera/ThirdPersonCamera.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


