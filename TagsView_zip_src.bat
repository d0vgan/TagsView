@echo off

set ARC_EXE=7z.exe
set PROJECT_NAME=TagsView
set ZIP_FILE=%PROJECT_NAME%_src.zip

if exist %ZIP_FILE% del %ZIP_FILE%
%ARC_EXE% u -tzip %ZIP_FILE% @%PROJECT_NAME%_zip_src.files -mx5
%ARC_EXE% t %ZIP_FILE%

pause