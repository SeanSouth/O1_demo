@echo OFF
SETLOCAL ENABLEDELAYEDEXPANSION
@echo .......................................................................................................................
@echo ..
@echo .. OTP PROGRAMMING
@echo ..
@echo .......................................................................................................................
@echo.

rem MODE=NORMAL: Programs OTP patch
rem MODE=TEST:   Tests script writting in addresses 0x1000-0x1046
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
rem # Program find_qspi otp function
rem ###############################################################

IF %MODE%==TEST (
set QSPI_PATCH_ADDR=0x1000
) ELSE (
set QSPI_PATCH_ADDR=0x1f4f
)

set QSPI_PATCH_LEN=0x8A
set QSPI_PATCH=0x2400B510  0xF000E015  0x2200F896  0x20034611  0xF839F000  0xD0012800  0xBD102001  0x21012200  0xF000200B  0x2800F830  0x2001D001  0x1C60E7F5  0x2C02B2C4  0x2400DBE7  0xF000E015  0x2201F87C  0x20032100  0xF81FF000  0xD0012800  0xE7E42001  0x46112201  0xF000200B  0x2800F816  0x2001D001  0x1C60E7DB  0x2C02B2C4  0x2000DBE7  0x2010E7D5  0x06892103  0x20086008  0x47706008  0x21032010  0x60080689  0xB5F84770  0x460D4607  0xF7FF4616  0x4638FFEE  0xF843F000  0xF0002000  0x2000F840  0xF83DF000  0xD0022E00  0xF0002000  0x2000F838  0xF835F000  0xE0042400  0xF0002000  0x1C60F830  0x42ACB284  0xF000DBF8  0x4669F81B  0x78087008  0xD0032871  0xFFD2F7FF  0xBDF82000  0xF810F000  0x70484669  0x28517848  0xF7FFD003  0x2000FFC7  0x2001E7F3  0x2008E7F1  0x06892103  0x47706008  0x06892103  0xBF007F09  0x06892103  0x07C98A89  0x29000FC9  0x2103D1F8  0x7A080689  0x21034770  0x76080689  0x2103BF00  0x8A890689  0x0FC907C9  0xD1F82900  0xB5104770  0xFF99F7FF  0x21032004  0x60080689  0xF7FF20F5  0xF7FFFFEA  0x20FFFF90  0xFFE5F7FF  0xFF8BF7FF  0x21032001  0x60080689  0xF7FF20FF  0xF7FFFFDC  0x20FFFF82  0x06892103  0x20047608  0x20FF6008  0x20017608  0xF7FF6008  0x20F0FF76  0xFFCBF7FF  0xF7FF20D0  0xF7FFFFC8  0x2400FF75  0x1C60E001  0x2019B284  0x42840140  0xF7FFDBF9  0x2004FFA8  0x06892103  0x20666008  0xFFB5F7FF  0xFF5BF7FF  0xF7FF2099  0xF7FFFFB0  0x2400FF5D  0x1C60E001  0x2019B284  0x42840140  0xF7FFDBF9  0x2001FF90  0x06892103  0x20666008  0xFF9DF7FF  0xFF43F7FF  0xF7FF2099  0xF7FFFF98  0x2400FF45  0x1C60E001  0x2019B284  0x42840140  0xF7FFDBF9  0x20ABFF78  0xFF89F7FF  0xFF36F7FF  0xE0012400  0xB2841C60  0x01402019  0xDBF94284  0xFF69F7FF  0xBD102001 

@echo Programming find_qspi otp function (address=%QSPI_PATCH_ADDR%, length=%QSPI_PATCH_LEN%)...
CALL "..\..\..\binaries\cli_programmer.exe" -b %UARTBOOT% COM%comprtnr% write_otp %QSPI_PATCH_ADDR% %QSPI_PATCH_LEN% %QSPI_PATCH%

rem ###############################################################
rem # Program find_qspi otp function starting address 
rem ###############################################################
IF %MODE%==TEST (
set QSPI_PATCH_ADDR=0x1045
) ELSE (
set QSPI_PATCH_ADDR=0x1f00
)
set QSPI_PATCH_LEN=2
set QSPI_PATCH=0x07F8FA78  0x8a
@echo Programming find_qspi otp function starting address (address=%QSPI_PATCH_ADDR%, length=%QSPI_PATCH_LEN%)...
CALL "..\..\..\binaries\cli_programmer.exe" -b %UARTBOOT% COM%comprtnr% write_otp %QSPI_PATCH_ADDR% %QSPI_PATCH_LEN% %QSPI_PATCH%


rem ###############################################################
rem # Program find_qspi otp function enable  
rem ###############################################################
if %MODE%==TEST (
set QSPI_PATCH_ADDR=0x1047
) ELSE (
set QSPI_PATCH_ADDR=0x1d49
)
set QSPI_PATCH_LEN=2
set QSPI_PATCH=0x02  0x0
@echo Programming find_qspi otp function enable (address=%QSPI_PATCH_ADDR%, length=%QSPI_PATCH_LEN%)...
CALL "..\..\..\binaries\cli_programmer.exe" -b %UARTBOOT% COM%comprtnr% write_otp %QSPI_PATCH_ADDR% %QSPI_PATCH_LEN% %QSPI_PATCH%


goto :Finished

:Finished
@echo.
@echo .......................................................................................................................
@echo ..
@echo .. FINISHED!
@echo ..
@echo .......................................................................................................................
