################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Server/Scenes/NetworkScene/tcp_session_impl.cpp 

OBJS += \
./Scenes/NetworkScene/tcp_session_impl.o 

CPP_DEPS += \
./Scenes/NetworkScene/tcp_session_impl.d 


# Each subdirectory must supply rules for building sources it contributes
Scenes/NetworkScene/tcp_session_impl.o: /home/ionut/Custom/trunk/Source\ Code/Server/Scenes/NetworkScene/tcp_session_impl.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/ionut/Custom/trunk/EclipseData/Divide-Server/../../Source Code/" -I"/home/ionut/Custom/Libraries/boost/" -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


