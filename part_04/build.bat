@echo off
REM del *.pdb > NUL 2> NUL
cl %* /nologo /Z7 /W4 /external:anglebrackets /external:W0 /wd4201 /wd4996 /wd4505 /link /incremental:no /opt:ref
del *.obj > NUL 2> NUL
del *.ilk > NUL 2> NUL
