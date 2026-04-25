@echo off
REM 1. 在后台启动 Renode 并运行你的 .resc 脚本
start /b D:\soft\Renode\renode.exe stm32f4_discovery.resc

REM 2. 等待 GDB Server (3333) 端口就绪
echo Waiting for GDB Server on port 3333...
:loop
netstat -an | findstr "3333" | findstr "LISTENING" > nul
if errorlevel 1 (
    timeout /t 1 /nobreak > nul
    goto loop
)
echo GDB Server is ready.
exit /b 0