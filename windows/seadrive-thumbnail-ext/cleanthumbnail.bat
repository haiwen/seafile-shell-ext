@echo off

title Clean thumbnail cached

taskkill /f /im explorer.exe

choice /t 3 /d y /n >nul

del /f /s /q "%userprofile%\AppData\Local\Microsoft\Windows\Explorer\*.tmp"

rename "%userprofile%\AppData\Local\Microsoft\Windows\Explorer\*.db" *.db.tmp

choice /t 3 /d y /n >nul

del /f /s /q "%userprofile%\AppData\Local\Microsoft\Windows\Explorer\*.tmp"

explorer
