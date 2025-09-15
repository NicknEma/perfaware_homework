@echo off

del *.pdb > NUL 2> NUL
call build check_ranges_main.cpp    /Fecheck_ranges.exe
call build check_precision_main.cpp /Fecheck_precision.exe
