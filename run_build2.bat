@echo off
set MSYSTEM=
set IDF_PATH=C:\Users\patij212\Downloads\matter-over-wifi\esp-idf
set ESP_MATTER_PATH=C:\Users\patij212\Downloads\matter-over-wifi\esp-matter-v2
set PATH=%ESP_MATTER_PATH%\tools\bin;%PATH%
call %IDF_PATH%\export.bat
echo === IDF ENV LOADED ===
cd C:\Users\patij212\Downloads\matter-over-wifi\firmware

REM First pass - let CMake configure and generate args.gn, then run GN gen
REM This will fail at chip_gn-build but that's expected
idf.py build 2>&1

REM Now fix the regeneration issue: delete build.ninja.d so ninja won't try to regenerate
if exist "build\esp-idf\chip\build.ninja.d" (
    echo === Patching ninja regeneration issue ===
    del "build\esp-idf\chip\build.ninja.d"
    REM Run the ninja build directly
    cd build\esp-idf\chip
    ninja esp32
    cd C:\Users\patij212\Downloads\matter-over-wifi\firmware
    REM Touch the stamp file so CMake thinks chip_gn is built
    mkdir "build\esp-idf\chip\chip_gn-prefix\src\chip_gn-stamp" 2>/dev/null
    type nul > "build\esp-idf\chip\chip_gn-prefix\src\chip_gn-stamp\chip_gn-build"
    REM Run the final link step
    idf.py build 2>&1
)
