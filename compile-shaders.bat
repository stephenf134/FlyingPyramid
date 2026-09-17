@echo off
setlocal

set "PROJ_DIR=%~dp0"
if not defined SHADERC set "SHADERC=%LOCALAPPDATA%\Programs\bgfx\bin\shaderc.exe"
if not defined BGFX_SRC_DIR set "BGFX_SRC_DIR=%USERPROFILE%\msvc-pkg-dev\bgfx\src"
set "SHADER_DIR=%PROJ_DIR%src\shaders"
set "OUTPUT_DIR=%PROJ_DIR%cmake-build-debug\shaders"

if not exist "%SHADERC%" (
    echo ERROR: shaderc was not found at "%SHADERC%". Set SHADERC to its executable path. 1>&2
    exit /b 1
)
if not exist "%BGFX_SRC_DIR%\bgfx_shader.sh" (
    echo ERROR: bgfx_shader.sh was not found in "%BGFX_SRC_DIR%". Set BGFX_SRC_DIR to the BGFX src directory. 1>&2
    exit /b 1
)
if not exist "%OUTPUT_DIR%\" mkdir "%OUTPUT_DIR%"
if not exist "%OUTPUT_DIR%\" (
    echo ERROR: Could not create "%OUTPUT_DIR%". 1>&2
    exit /b 1
)

@REM clean the shader-bin dir
del /s /f /q "%PROJ_DIR%\shaders-bin\*.*"
@REM in case there are subdirs (possible if changing project architecture in future)
for /d %%i in ("%PROJ_DIR%\shaders-bin\*") do rmdir /s /q "%%i""

echo Compiling vertex shader...
"%SHADERC%" -f "%SHADER_DIR%\vs.sc" -o "%OUTPUT_DIR%\vs.bin" ^
    --type vertex --platform windows --profile s_5_0 -i "%BGFX_SRC_DIR%" ^
    --varyingdef "%SHADER_DIR%\varying.def.sc"
if errorlevel 1 goto :failed

echo Compiling fragment shader...
"%SHADERC%" -f "%SHADER_DIR%\fs.sc" -o "%OUTPUT_DIR%\fs.bin" ^
    --type fragment --platform windows --profile s_5_0 -i "%BGFX_SRC_DIR%" ^
    --varyingdef "%SHADER_DIR%\varying.def.sc"
if errorlevel 1 goto :failed

echo Shaders compiled successfully to "%OUTPUT_DIR%".
exit /b 0

:failed
echo ERROR: Shader compilation failed. 1>&2
exit /b 1
