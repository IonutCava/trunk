################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/ASIO.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/ByteBuffer.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/Client.cpp \
/home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/tcp_session_tpl.cpp 

OBJS += \
./Libs/src/DivideNetworking/ASIO.o \
./Libs/src/DivideNetworking/ByteBuffer.o \
./Libs/src/DivideNetworking/Client.o \
./Libs/src/DivideNetworking/tcp_session_tpl.o 

CPP_DEPS += \
./Libs/src/DivideNetworking/ASIO.d \
./Libs/src/DivideNetworking/ByteBuffer.d \
./Libs/src/DivideNetworking/Client.d \
./Libs/src/DivideNetworking/tcp_session_tpl.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/src/DivideNetworking/ASIO.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/ASIO.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code" -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/DivideNetworking/ByteBuffer.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/ByteBuffer.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code" -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/DivideNetworking/Client.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/Client.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code" -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libs/src/DivideNetworking/tcp_session_tpl.o: /home/ionut/Custom/Divide/trunk/Source\ Code/Libs/src/DivideNetworking/tcp_session_tpl.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/Divide/trunk/Eclipse/../Source Code" -I/home/ionut/Custom/Divide/Libraries/physx/Include -I/home/ionut/Custom/Divide/Libraries/OIS/include -I/home/ionut/Custom/Divide/Libraries/Threadpool/include -I/home/ionut/Custom/Divide/Libraries/SimpleINI/include -I/home/ionut/Custom/Divide/Libraries/glbinding/include -I/home/ionut/Custom/Divide/Libraries/cegui-deps/include -I/home/ionut/Custom/Divide/Libraries/boost -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


