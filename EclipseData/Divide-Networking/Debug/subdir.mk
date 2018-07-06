################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Networking/ASIO.cpp \
/home/ionut/Custom/trunk/Source\ Code/Networking/ByteBuffer.cpp \
/home/ionut/Custom/trunk/Source\ Code/Networking/Client.cpp \
/home/ionut/Custom/trunk/Source\ Code/Networking/tcp_session_tpl.cpp 

OBJS += \
./ASIO.o \
./ByteBuffer.o \
./Client.o \
./tcp_session_tpl.o 

CPP_DEPS += \
./ASIO.d \
./ByteBuffer.d \
./Client.d \
./tcp_session_tpl.d 


# Each subdirectory must supply rules for building sources it contributes
ASIO.o: /home/ionut/Custom/trunk/Source\ Code/Networking/ASIO.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/ionut/Custom/trunk/EclipseData/Divide-Networking/../../Source Code/" -I/home/ionut/Custom/Libraries/boost/ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

ByteBuffer.o: /home/ionut/Custom/trunk/Source\ Code/Networking/ByteBuffer.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/ionut/Custom/trunk/EclipseData/Divide-Networking/../../Source Code/" -I/home/ionut/Custom/Libraries/boost/ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Client.o: /home/ionut/Custom/trunk/Source\ Code/Networking/Client.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/ionut/Custom/trunk/EclipseData/Divide-Networking/../../Source Code/" -I/home/ionut/Custom/Libraries/boost/ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

tcp_session_tpl.o: /home/ionut/Custom/trunk/Source\ Code/Networking/tcp_session_tpl.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/ionut/Custom/trunk/EclipseData/Divide-Networking/../../Source Code/" -I/home/ionut/Custom/Libraries/boost/ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


