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
set QSPI_PATCH=0x2010B5F8 0x06892103 0x20086008 0x209F6008 0xBF007608 0x2103BF00 0x8A890689 0x0FC907C9 0xD1F82900 0x2400BF00 0xBF00E013 0x06892103 0xBF007F09 0x2103BF00 0x8A890689 0x0FC907C9 0xD1F82900 0x06892103 0x46697A08 0x1C605508 0x2C03B2C4 0x2010DBE9 0x06892103 0x46686008 0x28EF7800 0x4668D003 0x28C87800 0xBF00D157 0x2003BF00 0x68400680 0x4308213C 0x06892103 0xBF006048 0x20012602 0x20066008 0x20087608 0x20316008 0x46087608 0x20107606 0xF0006008 0xBF00F895 0x27022600 0x21032001 0x60080689 0x76082006 0x60082008 0x76082001 0x76064608 0x20107607 0xF0006008 0xBF00F881 0x2003BF00 0x68400680 0x4388210C 0x60480609 0xBF00BF00 0x60082001 0x76082006 0x60082008 0x760820A3 0x49322000 0x70087008 0x21032010 0x60080689 0xBF00BF00 0x60C8482E 0x61082066 0x68404608 0x00400840 0x60481C40 0x2501BF00 0x4668E04C 0x28C27800 0xBF00D148 0x2003BF00 0x68400680 0x4308213C 0x06892103 0xBF006048 0x2001BF00 0x20106008 0x20086008 0x20066008 0x20117608 0x20096008 0x20016008 0x20407608 0x20107608 0xBF006008 0x60082009 0x76082005 0xBF00BF00 0xBF00BF00 0x06802003 0x07C07F00 0x28000FC0 0x2010D1F8 0x06892103 0xBF006008 0xBF00BF00 0x68404608 0x4388210C 0x60480609 0xBF00BF00 0x60C84808 0x61084808 0x68404608 0x00400840 0x60481C40 0x2501BF00 0xBDF84628 0x0C000020 0xA8A000EB 0xA80000EB 0x00001026 0x21032009 0x60080689 0x76082005 0x2003BF00 0x7F000680 0x0FC007C0 0xD1F82800 0x21032010 0x60080689 0x00004770 
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
