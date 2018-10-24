################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/aes.c \
../src/callbacks.c \
../src/interface.c \
../src/main.c \
../src/saet.c \
../src/support.c 

O_SRCS += \
../src/aes.o \
../src/callbacks.o \
../src/interface.o \
../src/main.o \
../src/saet.o \
../src/support.o 

OBJS += \
./src/aes.o \
./src/callbacks.o \
./src/interface.o \
./src/main.o \
./src/saet.o \
./src/support.o 

C_DEPS += \
./src/aes.d \
./src/callbacks.d \
./src/interface.d \
./src/main.d \
./src/saet.d \
./src/support.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


