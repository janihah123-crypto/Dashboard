@echo off
echo Starting TV Dashboard...
cd /d "c:\Users\Janib\OneDrive\Desktop\VScode-variante"
setlocal enabledelayedexpansion
echo Stopping any existing Node.js processes...
taskkill /f /im node.exe 2>nul
echo Starting the server...
start "dashboard-node" /b node server\server.js
echo Waiting for server to initialize...
timeout /t 3 /nobreak > nul
echo Detecting local IP address...
set LOCAL_IP=
rem try Get-NetIPAddress (preferred on modern Windows)
for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "try{ (Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue | Where-Object { $_.IPAddress -ne '127.0.0.1' -and $_.IPAddress -notlike '169.*' } | Select-Object -First 1 -ExpandProperty IPAddress) } catch{}"`) do set LOCAL_IP=%%I

rem fallback: try DNS lookup method
if "%LOCAL_IP%"=="" (
	for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "try{ ($ips = [System.Net.Dns]::GetHostEntry($env:COMPUTERNAME).AddressList | Where-Object { $_.AddressFamily -eq 'InterNetwork' -and $_.IPAddressToString -ne '127.0.0.1' }; if($ips){ $ips[0].IPAddressToString } } catch{}"`) do set LOCAL_IP=%%I
)

rem final fallback: parse ipconfig output (handles German/English Windows)
if "%LOCAL_IP%"=="" (
	echo Primary methods failed, trying ipconfig parsing...
	for /f "tokens=2 delims::" %%A in ('ipconfig ^| findstr /R /C:"IPv4" /C:"IPv4-Adresse"') do (
		set IPRAW=%%A
		set IPRAW=!IPRAW: =!
		set LOCAL_IP=!IPRAW!
		goto :gotip
	)
)
:gotip
if "%LOCAL_IP%"=="" set LOCAL_IP=localhost
echo Local IP: %LOCAL_IP%
echo Opening dashboard in fullscreen mode (localhost and LAN IP)...
powershell -NoProfile -Command "Start-Process 'msedge' @('--start-fullscreen','http://localhost:3000/dashboard.html','http://%LOCAL_IP%:3000/dashboard.html'); Start-Sleep -s 5; Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.SendKeys]::SendWait('{F11}')"
echo If other devices can't reach the dashboard, allow node.exe through Windows Firewall or open port 3000 for inbound TCP traffic.
echo Dashboard started successfully!