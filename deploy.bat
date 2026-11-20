@echo off
setlocal

:: Load variables from .env
if exist .env (
    for /f "usebackq eol=# tokens=1,* delims==" %%a in (".env") do (
        set "%%a=%%b"
    )
) else (
    echo [ERROR] .env file not found.
    exit /b 1
)

:: Check that build folder exists
if not exist "%BUILD_DIR%" (
    echo Build directory missing: %BUILD_DIR%
    exit /b 1
)

:: Transfer files to raspberry pi
echo Target: %RPI_USER%@%RPI_IP_ADDR%

scp -O ^
    -o MACs=hmac-sha2-256 ^
    "%BUILD_DIR%\alarm-manager" ^
    "%BUILD_DIR%\pulse-oximeter" ^
    "%RPI_USER%@%RPI_IP_ADDR%:/home/qnxuser/"
    
endlocal