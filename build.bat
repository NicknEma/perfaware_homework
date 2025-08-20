@echo off
del *.pdb > NUL 2> NUL
cl %* /nologo /Z7 /W4 /external:anglebrackets /external:W0 /link /incremental:no /opt:ref
del *.obj > NUL 2> NUL
del *.ilk > NUL 2> NUL
