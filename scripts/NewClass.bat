set FOLDER_NAME=%1
set CLASS_NAME=%2
setlocal ENABLEDELAYEDEXPANSION

set FINAL_NAME="%FOLDER_NAME%/%CLASS_NAME%.h"
if not exist %FINAL_NAME% (
    @REM Default header file
    set LAST_LINE=
    for /f "delims=" %%i in (./scripts/DefaultHeader) do (
        if "!LAST_LINE!" == "    " (
            echo.>> %FINAL_NAME%
        ) else (
            if NOT "!LAST_LINE!" == "" (
                set STRING=!LAST_LINE!
                set MODIFIED_LINE=!STRING:_CLASS_=%CLASS_NAME%!
                echo !MODIFIED_LINE!>> %FINAL_NAME%
            )
        )
        set LAST_LINE=%%i
    )
    echo|set /p="!LAST_LINE!">> %FINAL_NAME%
)

set FINAL_NAME="%FOLDER_NAME%/%CLASS_NAME%.cpp"
if not exist %FINAL_NAME% (
    @REM Default CPP file
    set LAST_LINE=
    for /f "delims=" %%i in (./scripts/DefaultCpp) do (
        if "!LAST_LINE!" == "    " (
            echo.>> %FINAL_NAME%
        ) else (
            if NOT "!LAST_LINE!" == "" (
                set STRING=!LAST_LINE!
                set MODIFIED_LINE=!STRING:_CLASS_=%CLASS_NAME%!
                echo !MODIFIED_LINE!>> %FINAL_NAME%
            )
        )
        set LAST_LINE=%%i
    )
    echo|set /p="!LAST_LINE!">> %FINAL_NAME%
)