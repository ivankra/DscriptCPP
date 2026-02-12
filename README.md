# DscriptCPP
Javascript implementation in C++

## Building on modern Linux
This is an ancient project developed during x86 era. Due to some assumptions in the code about running on a 32-bit processor, it won't work well with modern 64-bit compilers - you need i386 multilib gcc to build it. Here's an example how to build it in a docker container:

```
$ podman run -it --arch amd64 -v $PWD:$PWD -w $PWD debian:stable bash
$ dpkg --add-architecture i386 && apt-get update -y && apt-get install -y build-essential g++-multilib
$ make
$ ./dscript repl.js
Digital Mars DMDScript 1.05
http://www.digitalmars.com
Compiled by GNU C++
written by Walter Bright
1 source files
>
```
