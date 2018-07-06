################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/fastlz.c 

OBJS += \
./Libs/src/ReCast/RecastContrib/fastlz/fastlz.o 

C_DEPS += \
./Libs/src/ReCast/RecastContrib/fastlz/fastlz.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/src/ReCast/RecastContrib/fastlz/fastlz.o: /fastlz.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


