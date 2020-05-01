@echo off

set libs=User32.lib Dwmapi.lib Gdi32.lib UxTheme.lib

set opts=-FC -GR- -EHa- -nologo -Zi
set code=%cd%

if not exist build ( mkdir build )
pushd build
cl %opts% %code%\win32_custom_window.c %libs% -Fecustom_window
popd
