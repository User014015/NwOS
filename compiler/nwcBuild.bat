@echo off

echo ================================
echo        NwC Compiler Build
echo ================================
echo.

echo [1/3] Compiling lexer...
clang compiler/lexer/lexer.c -c -o compiler/builded/lexer.o

if errorlevel 1 goto error

echo [2/3] Compiling parser...
clang compiler/parser/parser.c -c -o compiler/builded/parser.o

if errorlevel 1 goto error

echo [3/4] Compiling codegen...
clang compiler/codegen/codegen.c -c -o compiler/builded/codegen.o

if errorlevel 1 goto error

echo [4/4] Linking NwC...
clang compiler/compilerMain.c compiler/builded/lexer.o compiler/builded/parser.o compiler/builded/codegen.o -o compiler/nwc.exe
if errorlevel 1 goto error
degen.o -o compiler/nwc.exe nw/test.nw

echo.
echo ================================
echo        BUILD SUCCESS
echo ================================
echo.

pause
exit /b 0


:error

echo.
echo ================================
echo        BUILD FAILED
echo ================================
echo.

pause
exit /b 1