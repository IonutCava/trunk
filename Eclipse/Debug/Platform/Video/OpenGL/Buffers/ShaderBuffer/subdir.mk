################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Divide/Source\ Code/Platform/Video/OpenGL/Buffers/ShaderBuffer/glUniformBuffer.cpp 

OBJS += \
./Platform/Video/OpenGL/Buffers/ShaderBuffer/glUniformBuffer.o 

CPP_DEPS += \
./Platform/Video/OpenGL/Buffers/ShaderBuffer/glUniformBuffer.d 


# Each subdirectory must supply rules for building sources it contributes
Platform/Video/OpenGL/Buffers/ShaderBuffer/glUniformBuffer.o: /home/ionut/Divide/Source\ Code/Platform/Video/OpenGL/Buffers/ShaderBuffer/glUniformBuffer.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Divide/Source Code" -I/home/ionut/DivideLibraries/physx/Include -I/home/ionut/DivideLibraries/OIS/include -I/home/ionut/DivideLibraries/Threadpool/include -I/home/ionut/DivideLibraries/SimpleINI/include -I/home/ionut/DivideLibraries/glbinding/include -I/home/ionut/DivideLibraries/cegui-deps/include -I/home/ionut/DivideLibraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


