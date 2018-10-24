################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/comm/SerCommTh.cpp 

OBJS += \
./src/comm/SerCommTh.o 

CPP_DEPS += \
./src/comm/SerCommTh.d 


# Each subdirectory must supply rules for building sources it contributes
src/comm/%.o: ../src/comm/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DHPROT_ENABLE_RX_RAW_BUFFER -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


