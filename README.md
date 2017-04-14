vpnes 0.5
=========

NES Emulator

Copyright &copy; 2012-2017 Ivanov Viktor

## Installation

### Cross-platform

#### From Git

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

For more detailed information about compiling the sources see [INSTALL](./INSTALL) and `./configure --help`

#### From source tarball

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

For more detailed information about compiling the sources see [INSTALL](./INSTALL) and `./configure --help`

### Microsoft Visual Studio 14.0 (Visual Studio 2015)

Open vpnes_msvc.sln and build the solution in Release configuration. The output file will be in ./Release.

## Documentation

To generate documentation via autotools+doxygen execute `$ make doxygen-doc` after configuring the project.
