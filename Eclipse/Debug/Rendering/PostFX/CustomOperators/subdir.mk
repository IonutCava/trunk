################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/BloomPreRenderOperator.cpp \
/home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/DoFPreRenderOperator.cpp \
/home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/FXAAPreRenderOperator.cpp \
/home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/SSAOPreRenderOperator.cpp 

OBJS += \
./Rendering/PostFX/CustomOperators/BloomPreRenderOperator.o \
./Rendering/PostFX/CustomOperators/DoFPreRenderOperator.o \
./Rendering/PostFX/CustomOperators/FXAAPreRenderOperator.o \
./Rendering/PostFX/CustomOperators/SSAOPreRenderOperator.o 

CPP_DEPS += \
./Rendering/PostFX/CustomOperators/BloomPreRenderOperator.d \
./Rendering/PostFX/CustomOperators/DoFPreRenderOperator.d \
./Rendering/PostFX/CustomOperators/FXAAPreRenderOperator.d \
./Rendering/PostFX/CustomOperators/SSAOPreRenderOperator.d 


# Each subdirectory must supply rules for building sources it contributes
Rendering/PostFX/CustomOperators/BloomPreRenderOperator.o: /home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/BloomPreRenderOperator.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/PostFX/CustomOperators/DoFPreRenderOperator.o: /home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/DoFPreRenderOperator.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/PostFX/CustomOperators/FXAAPreRenderOperator.o: /home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/FXAAPreRenderOperator.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Rendering/PostFX/CustomOperators/SSAOPreRenderOperator.o: /home/ionut/Divide/Source\ Code/Rendering/PostFX/CustomOperators/SSAOPreRenderOperator.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


