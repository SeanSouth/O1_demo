@echo OFF
SETLOCAL ENABLEDELAYEDEXPANSION
@echo .......................................................................................................................
@echo ..
@echo .. OTP PROGRAMMING
@echo ..
@echo .......................................................................................................................
@echo.

rem MODE=NORMAL: Programs OTP patch
rem MODE=TEST:   Tests script writting in addresses 0x0000-0x0039
set MODE=NORMAL
echo Executing in %MODE% mode!

set UARTBOOT=..\..\..\sdk\bsp\system\loaders\uartboot\Release\uartboot.bin
if NOT exist %UARTBOOT% (
@echo uartboot.bin not found!
@echo Please build uartboot project ^(RELEASE configuration^) and try again.
goto :Finished
)

@echo Please enter your COM port number and press enter.
@set /p comprtnr=-^> 
@echo.
@echo COMPORT=COM%comprtnr% 

rem ###############################################################
rem # Program qspi_loader otp function
rem ###############################################################

IF %MODE%==TEST (
set QSPI_PATCH_ADDR=0x0000
) ELSE (
set QSPI_PATCH_ADDR=0x1f03
)

set QSPI_PATCH_LEN=0x7D
set QSPI_PATCH=0x2010B5F8 
@echo Programming qspi_loader otp function (address=%QSPI_PATCH_ADDR%, length=%QSPI_PATCH_LEN%)...
CALL "..\..\..\binaries\cli_programmer.exe" -b %UARTBOOT% COM%comprtnr% write_otp %QSPI_PATCH_ADDR% %QSPI_PATCH_LEN% %QSPI_PATCH%

rem ###############################################################
rem # Program qspi_loader otp function starting address 
rem ###############################################################
IF %MODE%==TEST (
set QSPI_PATCH_ADDR=0x003F
) ELSE (
set QSPI_PATCH_ADDR=0x1f01
)
set QSPI_PATCH_LEN=2
set QSPI_PATCH=0x07F8F818  0x7D
@echo Programming qspi_loader otp function starting address (address=%QSPI_PATCH_ADDR%, length=%QSPI_PATCH_LEN%)...
CALL "..\..\..\binaries\cli_programmer.exe" -b %UARTBOOT% COM%comprtnr% write_otp %QSPI_PATCH_ADDR% %QSPI_PATCH_LEN% %QSPI_PATCH%


rem ###############################################################
rem # Program qspi_loader otp function enable  
rem ###############################################################
if %MODE%==TEST (
set QSPI_PATCH_ADDR=0x0040
) ELSE (
set QSPI_PATCH_ADDR=0x1d49
)
set QSPI_PATCH_LEN=2
set QSPI_PATCH=0x04  0x0
@echo Programming qspi_loader otp function enable (address=%QSPI_PATCH_ADDR%, length=%QSPI_PATCH_LEN%)...
CALL "..\..\..\binaries\cli_programmer.exe" -b %UARTBOOT% COM%comprtnr% write_otp %QSPI_PATCH_ADDR% %QSPI_PATCH_LEN% %QSPI_PATCH%


goto :Finished

:Finished
@echo.
@echo .......................................................................................................................
@echo ..
@echo .. FINISHED!
@echo ..
@echo .......................................................................................................................