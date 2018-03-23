vpnes 0.5
=========

NES Emulator

Copyright &copy; 2012-2018 Ivanov Viktor

## Installation

### Cross-platform

Before compiling make sure that you have autoconf-archive (>= 2015.11.07), SDL2 (>= 2.0.5) and SDL2_gfx installed.

#### Compiling From Git

Clone repository

```
$ git clone http://github.com/itmo153277/vpnes.git vpnes-0.5 && cd vpnes-0.5
```

Generate autotools scripts

```	
$ sh ./autogen.sh
```

Configure

```
$ ./configure
```

Compile

 ```
$ make
```

Install as root

```
# make install
```

For more detailed information about compiling the sources see [INSTALL](./INSTALL) and `./configure --help`.

#### Compiling From Source Tarball

Extract sources

```
$ tar xvf vpnes-0.5.tar.gz && cd vpnes-0.5
```

Configure

```
$ ./configure
```

 Compile

 ```
$ make
```

Install as root

```
# make install
```

For more detailed information about compiling the sources see [INSTALL](./INSTALL) and `./configure --help`.

### Microsoft Visual Studio

Before compiling make sure that you have installed SDL2 and SDL2_gfx. SDL2 directories should be added to your user macros as follows:

* `SDLIncludes` &mdash; path to SDL includes for SDL2 and SDL2_gfx

* `SDLLib` &mdash; path to SDL *.lib and *.dll

* `SDLDeps` &mdash; SDL lib files SDL2.lib, SDL2main.lib and SDL2_gfx.lib

Open vpnes_msvc.sln and build the solution in Release configuration. The output file will be in ./Release.

> Note: it is known that MSVC sometimes throws "out of heap" exception. If it does so, consider using another compiler or rewriting `cpu.cpp` and `cpu_compiler.hpp` to comfort MSVC.

## Documentation

To generate documentation via autotools+doxygen execute `$ make doxygen-doc` after configuring the project.
