@echo off
REM Gera o executavel da GUI (Windows) em dist\GIF-Display.exe
REM Requisitos: pip install -r requirements.txt pyinstaller
cd /d "%~dp0"
pyinstaller --noconfirm --onefile --windowed --name "GIF-Display" --paths . gui_launcher.py
echo.
echo Pronto: dist\GIF-Display.exe
