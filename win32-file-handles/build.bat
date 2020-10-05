@echo off

if not exist "build\" mkdir build

set opts=-FC -GR- -EHa- -nologo -Zi
set code=%cd%
pushd build
cl %opts% %code%\win32_file_handles.c -DSAME_FILE_TWICE -Fesame_file_twice
popd
