################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
E:/Black_Orca_SDK_BTLE_v_1.0.4.812/sdk/interfaces/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ble_functions.c \
E:/Black_Orca_SDK_BTLE_v_1.0.4.812/sdk/interfaces/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ripple.c 

OBJS += \
./sdk/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ble_functions.o \
./sdk/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ripple.o 

C_DEPS += \
./sdk/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ble_functions.d \
./sdk/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ripple.d 


# Each subdirectory must supply rules for building sources it contributes
sdk/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ble_functions.o: E:/Black_Orca_SDK_BTLE_v_1.0.4.812/sdk/interfaces/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ble_functions.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g3 -Ddg_configBLACK_ORCA_IC_REV=BLACK_ORCA_IC_REV_A -Ddg_configBLACK_ORCA_IC_STEP=BLACK_ORCA_IC_STEP_D -I"../../../../../sdk/interfaces/ble_stack/" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\adapters\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\config" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\include\adapter" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\include\manager" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\config" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att\attc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att\attm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att\atts" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gap" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gap\gapc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gap\gapm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gatt" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gatt\gattc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gatt\gattm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\l2c\l2cc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\l2c\l2cm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\smp" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\smp\smpc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\smp\smpm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\rwble_hl" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\em" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\llc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\lld" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\llm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\rwble" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\profiles" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ea\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\em\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\hci\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\hci\src" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\common\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\dbg\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\gtl\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\gtl\src" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\h4tl\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\ke\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\ke\src" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\nvds\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\rwip\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch\ll\armgcc_4_8" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch\boot\armgcc_4_8" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch\compiler\armgcc_4_8" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\build\ble-full\reg\fw" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\flash" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\reg" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\rf" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\rf\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble_services\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble_clients\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\config" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\system\sys_man\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\free_rtos\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\osal" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\peripherals\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\memory\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\config" -include"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\config\custom_config_qspi.h" -include"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\config\ble_config.h" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

sdk/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ripple.o: E:/Black_Orca_SDK_BTLE_v_1.0.4.812/sdk/interfaces/ble/src/stack/plf/black_orca/src/driver/rf/src/rf_ripple.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g3 -Ddg_configBLACK_ORCA_IC_REV=BLACK_ORCA_IC_REV_A -Ddg_configBLACK_ORCA_IC_STEP=BLACK_ORCA_IC_STEP_D -I"../../../../../sdk/interfaces/ble_stack/" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\adapters\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\config" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\include\adapter" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\include\manager" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\config" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att\attc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att\attm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\att\atts" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gap" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gap\gapc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gap\gapm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gatt" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gatt\gattc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\gatt\gattm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\l2c\l2cc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\l2c\l2cm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\smp" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\smp\smpc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\host\smp\smpm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\hl\src\rwble_hl" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\em" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\llc" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\lld" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\controller\llm" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\ll\src\rwble" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ble\profiles" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\ea\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\em\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\hci\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\ip\hci\src" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\common\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\dbg\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\gtl\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\gtl\src" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\h4tl\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\ke\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\ke\src" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\nvds\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\modules\rwip\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch\ll\armgcc_4_8" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch\boot\armgcc_4_8" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\arch\compiler\armgcc_4_8" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\build\ble-full\reg\fw" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\flash" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\reg" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\rf" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\src\stack\plf\black_orca\src\driver\rf\api" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble_services\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble_clients\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\config" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\system\sys_man\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\free_rtos\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\osal" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\peripherals\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\bsp\memory\include" -I"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\config" -include"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\projects\dk_apps\ble_profiles\ancs\config\custom_config_qspi.h" -include"E:\Black_Orca_SDK_BTLE_v_1.0.4.812\sdk\interfaces\ble\config\ble_config.h" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


