SETLOCAL ENABLEDELAYEDEXPANSION

set config_file=program_qspi_ini.cmd
set user_config_file=program_qspi_config.bat
if not exist %config_file% call %user_config_file% 
call %config_file% 

if not exist ..\..\..\binaries\bin2image.exe (
  echo.
  echo bin2mage.exe not found. Please make sure it is built in sdk\utilities\bin2image first
  exit /b 1
)

if %CHIP_REV% == AC (
@echo Preparing image for AC chip: %IMAGE%
CALL "..\..\..\binaries\bin2image.exe" qspi_cached %IMAGE% %IMAGE%.cached
goto :Finished
) 


@echo Preparing image for AD chip: %IMAGE%
set bin2image_params=qspi_cached %IMAGE% %IMAGE%.cached AD 
if %ENABLE_UART% == y (
@echo bin2image_params=%bin2image_params% enable_uart %RAM_SHUFFLING%
CALL "..\..\..\binaries\bin2image.exe" %bin2image_params% enable_uart %RAM_SHUFFLING%
goto :Finished
)
@echo bin2image_params=%bin2image_params% %RAM_SHUFFLING%
CALL "..\..\..\binaries\bin2image.exe" %bin2image_params% %RAM_SHUFFLING%
goto :Finished


:Finished

