REM CMAKE does not support nasm. you need to run this command to generate dynalib.obj
REM You can obtain nasm.exe from http://www.nasm.us/

REM nasm.exe dynalib.asm -f win32 -O0 
nasm.exe dynalib_x86_64.asm -f win64 -O0 