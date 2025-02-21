@git -C %~dp0 submodule update --init Kore
@git -C %~dp0 submodule update --init --recursive RuntimeShaderCompilation/krafix
@call %~dp0\Kore\get_dlc.bat