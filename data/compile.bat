@echo off

SetLocal EnableDelayedExpansion

for /r %%i in (shaders\*) do (
	set LAST=%%i
	set LAST=!LAST:~-3!
	set SPV=spv
	if !LAST! neq !SPV! (
		echo %%i
		glslc.exe -c %%i -o %%i.spv
	)
)

::pause