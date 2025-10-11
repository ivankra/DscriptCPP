@echo off

cd dmgc
nmake -f msvc.mak clean
nmake -f msvc.mak BUILD_CONFIG=RELEASE
cd ..

REM BEGIN OLD GC
REM cd hbgc2
REM nmake -f gc.mak CFG="gc - Win32 Debug" CLEAN
REM nmake -f gc.mak CFG="gc - Win32 Debug" ALL
REM set BUILD_CONFIG=RELEASE
REM nmake -f gc.mak CFG="gc - Win32 Release" CLEAN
REM nmake -f gc.mak CFG="gc - Win32 Release" ALL
REM cd ..
REM END OLD GC

cd root
nmake -f msvc.mak clean
nmake -f msvc.mak BUILD_CONFIG=RELEASE
cd ..

cd dscript
nmake -f msvc.mak clean
nmake -f msvc.mak BUILD_CONFIG=RELEASE
cd ..

:End
