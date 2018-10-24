################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/wthread/WCondition.cpp \
../src/wthread/WLock.cpp \
../src/wthread/WThread.cpp 

OBJS += \
./src/wthread/WCondition.o \
./src/wthread/WLock.o \
./src/wthread/WThread.o 

CPP_DEPS += \
./src/wthread/WCondition.d \
./src/wthread/WLock.d \
./src/wthread/WThread.d 


# Each subdirectory must supply rules for building sources it contributes
src/wthread/%.o: ../src/wthread/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DHPROT_ENABLE_RX_RAW_BUFFER -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


