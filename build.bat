cls
del *.obj
del *.dll
cl /LD /EHsc /nologo DLLRavendawn.cpp User32.lib DbgHelp.lib
