@echo off
set MSYSTEM=
set IDF_PATH=C:\Users\patij212\Downloads\matter-over-wifi\esp-idf
set ESP_MATTER_PATH=C:\Users\patij212\Downloads\matter-over-wifi\esp-matter-v2
set PATH=%ESP_MATTER_PATH%\tools\bin;%PATH%
call %IDF_PATH%\export.bat
echo === IDF ENV LOADED ===
gn --version
cd C:\Users\patij212\Downloads\matter-over-wifi\firmware
idf.py build 2>&1
