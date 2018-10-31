################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/comm/serial/serial.c 

OBJS += \
./src/comm/serial/serial.o 

C_DEPS += \
./src/comm/serial/serial.d 


# Each subdirectory must supply rules for building sources it contributes
src/comm/serial/%.o: ../src/comm/serial/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DHPROT_ENABLE_RX_RAW_BUFFER -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


