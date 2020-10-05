@echo off

del /s /q build\test_data\*
rmdir /s /q build\test_data\
mkdir build\test_data
xcopy test_data build\test_data\ /E /H

pushd build

REM same_file_twice

popd
