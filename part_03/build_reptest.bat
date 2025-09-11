@echo off

nasm -f win64 -o repetition_tester_cpu_loops.obj        repetition_tester_cpu_loops.asm
nasm -f win64 -o repetition_tester_read_write_ports.obj repetition_tester_read_write_ports.asm
nasm -f win64 -o repetition_tester_read_bandwidth.obj   repetition_tester_read_bandwidth.asm
nasm -f win64 -o repetition_tester_cache_loops.obj      repetition_tester_cache_loops.asm

lib /nologo repetition_tester_cpu_loops.obj
lib /nologo repetition_tester_read_write_ports.obj
lib /nologo repetition_tester_read_bandwidth.obj
lib /nologo repetition_tester_cache_loops.obj

call build.bat repetition_tester_read_functions_main.cpp   /Fereptest_read_functions.exe
call build.bat repetition_tester_cpu_loops_main.cpp        /Fereptest_cpu_loops.exe
call build.bat repetition_tester_read_write_ports_main.cpp /Fereptest_read_write_ports.exe
call build.bat repetition_tester_read_bandwidth_main.cpp   /Fereptest_read_bandwidth.exe
call build.bat repetition_tester_cache_loops_main.cpp      /Fereptest_cache_loops.exe

del repetition_tester_cpu_loops.obj        > NUL 2> NUL
del repetition_tester_read_write_ports.obj > NUL 2> NUL
del repetition_tester_read_bandwidth.obj   > NUL 2> NUL
del repetition_tester_cache_loops.obj      > NUL 2> NUL

del repetition_tester_cpu_loops.lib        > NUL 2> NUL
del repetition_tester_read_write_ports.lib > NUL 2> NUL
del repetition_tester_read_bandwidth.lib   > NUL 2> NUL
del repetition_tester_cache_loops.lib      > NUL 2> NUL
