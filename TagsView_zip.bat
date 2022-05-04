@echo off

set ARC_EXE=7z.exe
set PROJECT_NAME=TagsView

%ARC_EXE% u -tzip %PROJECT_NAME%.zip @%PROJECT_NAME%_zip.files -mx5
%ARC_EXE% t %PROJECT_NAME%.zip

pause