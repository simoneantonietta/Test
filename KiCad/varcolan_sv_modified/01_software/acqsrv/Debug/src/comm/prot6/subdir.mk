################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/comm/prot6/hprot.c 

OBJS += \
./src/comm/prot6/hprot.o 

C_DEPS += \
./src/comm/prot6/hprot.d 


# Each subdirectory must supply rules for building sources it contributes
src/comm/prot6/%.o: ../src/comm/prot6/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DHPROT_ENABLE_RX_RAW_BUFFER -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


