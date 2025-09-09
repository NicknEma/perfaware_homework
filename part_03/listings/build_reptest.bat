@echo off
nasm -f win64 -o haversine_repetition_tester_loops.obj ../haversine_repetition_tester_loops.asm
lib /nologo haversine_repetition_tester_loops.obj
call ..\build.bat caseys_repetition_tester_main.cpp /Fe:caseys_tester.exe /O1
