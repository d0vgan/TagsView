@echo off

set ARC_EXE=7z.exe
set PROJECT_NAME=TagsView
set ZIP_FILE=%PROJECT_NAME%_dll.zip

if exist %ZIP_FILE% del %ZIP_FILE%
cd .\Release
%ARC_EXE% u -tzip ..\%ZIP_FILE% @..\%PROJECT_NAME%_zip_dll.files -mx5
cd ..
%ARC_EXE% a -tzip %ZIP_FILE% .\Plugin\ -mx5
%ARC_EXE% a -tzip %ZIP_FILE% .\TagsView_History.txt .\TagsView_Readme.txt -mx5
%ARC_EXE% t %ZIP_FILE%

pause