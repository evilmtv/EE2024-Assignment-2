################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: demo_GPIO.axf

# Tool invocations
demo_GPIO.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"D:\NUS\Year 2 AY2016-2017\EE2024\Lib_CMSISv1p30_LPC17xx\Debug" -L"D:\NUS\Year 2 AY2016-2017\EE2024\Lib_EaBaseBoard\Debug" -L"D:\NUS\Year 2 AY2016-2017\EE2024\Lib_MCU\Debug" -Xlinker --gc-sections -Xlinker -Map=demo_GPIO.map -mcpu=cortex-m3 -mthumb -T "demo_GPIO_Debug.ld" -o "demo_GPIO.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) demo_GPIO.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size demo_GPIO.axf; # arm-none-eabi-objdump -h -S demo_GPIO.axf >demo_GPIO.lss
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
