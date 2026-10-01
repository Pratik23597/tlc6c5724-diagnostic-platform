@echo off
title TLC6C5724 Demo Launcher

echo Starting TLC6C5724 MQTT broker...

start "Mosquitto Demo" "C:\Program Files\mosquitto\mosquitto.exe" -c "C:\Users\user\Desktop\mosquitto-demo.conf" -v

echo.
echo Starting Node-RED...

start "Node-RED" cmd /k node-red

echo.
echo Waiting for Node-RED to become ready...

:WAIT_FOR_NODE_RED
powershell -NoProfile -Command ^
  "try { $r = Invoke-WebRequest -UseBasicParsing http://127.0.0.1:1880 -TimeoutSec 2; exit 0 } catch { exit 1 }"

if errorlevel 1 (
    timeout /t 1 /nobreak >nul
    goto WAIT_FOR_NODE_RED
)

echo.
echo Node-RED is ready.

start "" "http://127.0.0.1:1880/dashboard/page1"

echo.
echo TLC6C5724 Diagnostics launched successfully.
timeout /t 3 /nobreak >nul
exit