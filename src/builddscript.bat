@echo off

cd dmgc
make -f dmc.mak clean
make -f dmc.mak
cd ..

cd root
make -f dmc.mak clean
make -f dmc.mak
copy rutabega.lib ..\dscript
cd ..

cd dscript
make -f dmc.mak clean
make -f dmc.mak
cd ..

:End
