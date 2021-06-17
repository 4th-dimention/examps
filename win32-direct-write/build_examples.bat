@echo off

if not exist "build\" (mkdir build)

set opts=-nologo -FC -Zi

pushd build
cl %opts% ..\example_texture_extraction.cpp dwrite.lib gdi32.lib /Feextract
cl %opts% ..\example_rasterizer.cpp dwrite.lib gdi32.lib user32.lib opengl32.lib /Ferasterize
popd