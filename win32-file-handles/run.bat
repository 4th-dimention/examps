@echo off

del /s /q build\test_data\*
rmdir /s /q build\test_data\
mkdir build\test_data
xcopy test_data build\test_data\ /E /H
attrib +R build\test_data\text_file_protected.txt

pushd build

same_file_twice

popd
