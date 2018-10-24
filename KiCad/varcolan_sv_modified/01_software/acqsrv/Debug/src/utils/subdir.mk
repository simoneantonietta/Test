################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/utils/HashFunctions.cpp \
../src/utils/Serializer.cpp \
../src/utils/SimpleCfgFile.cpp \
../src/utils/TaggedBinFile.cpp \
../src/utils/TaggedTxtFile.cpp \
../src/utils/Trace.cpp \
../src/utils/Utils.cpp \
../src/utils/baseconv.cpp \
../src/utils/kbhit.cpp \
../src/utils/test.cpp 

C_SRCS += \
../src/utils/wc_strncmp.c 

OBJS += \
./src/utils/HashFunctions.o \
./src/utils/Serializer.o \
./src/utils/SimpleCfgFile.o \
./src/utils/TaggedBinFile.o \
./src/utils/TaggedTxtFile.o \
./src/utils/Trace.o \
./src/utils/Utils.o \
./src/utils/baseconv.o \
./src/utils/kbhit.o \
./src/utils/test.o \
./src/utils/wc_strncmp.o 

C_DEPS += \
./src/utils/wc_strncmp.d 

CPP_DEPS += \
./src/utils/HashFunctions.d \
./src/utils/Serializer.d \
./src/utils/SimpleCfgFile.d \
./src/utils/TaggedBinFile.d \
./src/utils/TaggedTxtFile.d \
./src/utils/Trace.d \
./src/utils/Utils.d \
./src/utils/baseconv.d \
./src/utils/kbhit.d \
./src/utils/test.d 


# Each subdirectory must supply rules for building sources it contributes
src/utils/%.o: ../src/utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DHPROT_ENABLE_RX_RAW_BUFFER -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/utils/%.o: ../src/utils/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DHPROT_ENABLE_RX_RAW_BUFFER -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


