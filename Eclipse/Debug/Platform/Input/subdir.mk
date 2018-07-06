################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Platform/Input/AutoKeyRepeat.cpp \
/home/ionut/Divide/Source\ Code/Platform/Input/EffectManager.cpp \
/home/ionut/Divide/Source\ Code/Platform/Input/EventHandler.cpp \
/home/ionut/Divide/Source\ Code/Platform/Input/InputInterface.cpp 

OBJS += \
./Platform/Input/AutoKeyRepeat.o \
./Platform/Input/EffectManager.o \
./Platform/Input/EventHandler.o \
./Platform/Input/InputInterface.o 

CPP_DEPS += \
./Platform/Input/AutoKeyRepeat.d \
./Platform/Input/EffectManager.d \
./Platform/Input/EventHandler.d \
./Platform/Input/InputInterface.d 


# Each subdirectory must supply rules for building sources it contributes
Platform/Input/AutoKeyRepeat.o: /home/ionut/Divide/Source\ Code/Platform/Input/AutoKeyRepeat.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Input/EffectManager.o: /home/ionut/Divide/Source\ Code/Platform/Input/EffectManager.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Input/EventHandler.o: /home/ionut/Divide/Source\ Code/Platform/Input/EventHandler.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Platform/Input/InputInterface.o: /home/ionut/Divide/Source\ Code/Platform/Input/InputInterface.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Eclipse/../Source Code/" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Divide/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/DivideLibraries/boost/ -I/home/ionut/DivideLibraries/OpenAL/include/ -I/home/ionut/DivideLibraries/cegui/include/ -I/home/ionut/DivideLibraries/physx/ -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/physx/Samples/PxToolkit/include -I/home/ionut/DivideLibraries/physx/Source/foundation/include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/assimp/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/sdl/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


