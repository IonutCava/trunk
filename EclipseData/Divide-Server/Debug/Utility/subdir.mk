################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Server/Utility/Patch.cpp \
/home/ionut/Custom/trunk/Source\ Code/Server/Utility/XMLParser.cpp 

OBJS += \
./Utility/Patch.o \
./Utility/XMLParser.o 

CPP_DEPS += \
./Utility/Patch.d \
./Utility/XMLParser.d 


# Each subdirectory must supply rules for building sources it contributes
Utility/Patch.o: /home/ionut/Custom/trunk/Source\ Code/Server/Utility/Patch.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/ionut/Custom/trunk/EclipseData/Divide-Server/../../Source Code/" -I"/home/ionut/Custom/Libraries/boost/" -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Utility/XMLParser.o: /home/ionut/Custom/trunk/Source\ Code/Server/Utility/XMLParser.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/ionut/Custom/trunk/EclipseData/Divide-Server/../../Source Code/" -I"/home/ionut/Custom/Libraries/boost/" -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


