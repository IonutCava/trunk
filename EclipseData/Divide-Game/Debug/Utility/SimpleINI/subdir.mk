################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/Libs/SimpleINI/src/ConvertUTF.c 

OBJS += \
./Utility/SimpleINI/ConvertUTF.o 

C_DEPS += \
./Utility/SimpleINI/ConvertUTF.d 


# Each subdirectory must supply rules for building sources it contributes
Utility/SimpleINI/ConvertUTF.o: /home/ionut/Custom/trunk/Source\ Code/Libs/SimpleINI/src/ConvertUTF.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


