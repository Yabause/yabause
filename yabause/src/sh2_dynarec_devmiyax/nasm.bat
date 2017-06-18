REM CMAKE does not support nasm. you need to run this command to generate dynalib.obj
REM You can obtain nasm.exe from http://www.nasm.us/

nasm.exe dynalib.asm -f win32 -O0 