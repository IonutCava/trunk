################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/Divide/trunk/Source\ Code/Physics/PhysX/PhysX.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Physics/PhysX/PhysXSceneInterface.cpp 

OBJS += \
./Physics/PhysX/PhysX.o \
./Physics/PhysX/PhysXSceneInterface.o 

CPP_DEPS += \
./Physics/PhysX/PhysX.d \
./Physics/PhysX/PhysXSceneInterface.d 


# Each subdirectory must supply rules for building sources it contributes
Physics/PhysX/PhysX.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Physics/PhysX/PhysX.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/usr/include/SDL2 -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Physics/PhysX/PhysXSceneInterface.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Physics/PhysX/PhysXSceneInterface.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Divide/Libraries/boost/ -I/home/ionut/Custom/Divide/Libraries/OpenAL/include/ -I/home/ionut/Custom/Divide/Libraries/cegui/include/ -I/home/ionut/Custom/Divide/Libraries/physx/ -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Divide/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/assimp/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/usr/include/SDL2 -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


