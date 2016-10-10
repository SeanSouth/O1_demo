################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include sdk/peripherals/src/subdir.mk
-include sdk/osal/subdir.mk
-include sdk/memory/src/subdir.mk
-include sdk/cpm/subdir.mk
-include sdk/ble_services/src/subdir.mk
-include sdk/ble_clients/src/subdir.mk
-include sdk/ble/src/util/subdir.mk
-include sdk/ble/src/stack/plf/black_orca/src/driver/rf/src/subdir.mk
-include sdk/ble/src/stack/plf/black_orca/src/arch/main/ble/subdir.mk
-include sdk/ble/src/stack/modules/rwip/src/subdir.mk
-include sdk/ble/src/stack/modules/nvds/src/subdir.mk
-include sdk/ble/src/stack/ip/ble/profiles/subdir.mk
-include sdk/ble/src/stack/ip/ble/ll/src/rwble/subdir.mk
-include sdk/ble/src/manager/subdir.mk
-include sdk/ble/src/adapter/subdir.mk
-include sdk/ble/src/subdir.mk
-include sdk/adapters/src/subdir.mk
-include sdk/FreeRTOS/portable/MemMang/subdir.mk
-include sdk/FreeRTOS/portable/GCC/ARM_CM0/subdir.mk
-include sdk/FreeRTOS/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
ancs.bin \

SECONDARY_SIZE += \
ancs.siz \


# All Target
all: pre-build main-build

# Main-build Target
main-build: ancs.elf secondary-outputs

# Tool invocations
ancs.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g3 -T "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\ldscripts/mem.ld" -T "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\ldscripts/sections.ld" -Xlinker --gc-sections -L"../../../../../sdk/interfaces/ble_stack//DA14681-00-Debug" -L"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\misc" -Wl,-Map,"ancs.map" --specs=nano.specs --specs=nosys.specs -o "ancs.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

ancs.bin: ancs.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O binary "ancs.elf"  "ancs.bin"
	@echo 'Finished building: $@'
	@echo ' '

ancs.siz: ancs.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "ancs.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(SECONDARY_FLASH)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_UPPER_DEPS)$(C_DEPS) ancs.elf
	-@echo ' '

pre-build:
	-@echo 'Generate mem.ld for selected configuration (QSPI).'
	-arm-none-eabi-gcc -I "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\config" -include "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\config\custom_config_qspi.h" -E -P -c "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\ldscripts/mem.ld.h" -o "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\ldscripts/mem.ld"; arm-none-eabi-gcc -I "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\config" -include "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\config\custom_config_qspi.h" -Ddg_configBLACK_ORCA_IC_REV=BLACK_ORCA_IC_REV_A -Ddg_configBLACK_ORCA_IC_STEP=BLACK_ORCA_IC_STEP_D -E -P -c "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\ldscripts/sections.ld.h" -o "E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\ldscripts/sections.ld"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: main-build pre-build

-include ../makefile.targets
