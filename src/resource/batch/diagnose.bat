@echo off
mode con: cols=100 lines=55
set /A errors=0
powershell write-host -back Red "`n"-------------------"`n"R5R DIAGNOSTIC TOOL""`n"-------------------`n"

Rem made by @Archi#0731 on discord, tell me if you have problems with this script, I'm not sure if it's perfect
echo pls don't run me as admin
cd ../
powershell write-host -fore Yellow "`n"Checking if Origin is installed with registry keys : 
@ECHO reg query HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Origin
if %errorlevel%==0 (powershell write-host -fore Green [Origin Installation found]"`n") else (SET /A errors+=1 && powershell write-host -fore Red [Origin Installation not found]"`n")

powershell write-host -fore Yellow Checking if the main executable exists : 
if exist r5apex.exe (powershell write-host -fore Green [Main executable detected]"`n") else (SET /A errors+=1 && powershell write-host -fore Red [Missing file !]"`n")

powershell write-host -fore Yellow Checking if the platform folder exists : 
if exist platform\ (powershell write-host -fore Green ["platform/" folder detected]"`n") else (SET /A errors+=1 && powershell write-host -fore Red [Missing file !]"`n")

powershell write-host -fore Yellow Checking if the scripts folder exists : 
if exist platform\scripts\ (powershell write-host -fore Green ["platform/scripts" folder detected]"`n") else (SET /A errors+=1 && powershell write-host -fore Red [Missing folder!]"`n" )

powershell write-host -fore Yellow Listing scripts folder: 
DIR platform\scripts\ /A:D /-N /b

powershell write-host -fore Yellow "`n"Checking if the language is set to english : 
findstr /r "english" "%userprofile%\Saved Games\Respawn\Apex_fnf\profile\profile.cfg"
if %errorlevel%==0 (powershell write-host -fore Green [Language set to English]"`n") else (SET /A errors+=1 && powershell write-host -fore Red [Not in English or file not found!]"`n")

powershell write-host -fore Yellow Listing folders : 
DIR /A:D /-N /b

powershell write-host "`n"
powershell write-host "RESULTS :"
if %errors% GTR 0 (powershell write-host  "Your install has some problems, please open a ticket on the discord server and send a screen shot of this." && PowerShell -Command "Add-Type -AssemblyName PresentationFramework;[System.Windows.MessageBox]::Show('Your install has some problems, please open a ticket on the discord server and send a screen shot of this.')") else (powershell write-host -fore Green "All good! You can start r5apex.exe. If you still have a problem, open a ticket on the discord server." && PowerShell -Command "Add-Type -AssemblyName PresentationFramework;[System.Windows.MessageBox]::Show('All good! You can start r5apex.exe. If you still have a problem, open a ticket on the discord server.')")
pause

