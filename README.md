# vpnes 0.5

NES Emulator

Copyright &copy; 2012-2017  Ivanov Viktor

## Installation

### Cross platform

To install from the source code run the following:

1. Clone repository

`$ git clone http://github.com/itmo153277/vpnes.git .`

2. Generate autotools scripts 

`$ sh ./autogen.sh`

3. Configure

`$ ./configure`

4. Compile

`$ make`

### Microsoft Visual Studio 14.0 (Visual Studio 2015)

Open vpnes_msvc.sln and build the solution in Relase configuration. Output directory will be ./Release.

5. Install as root

`# make install`

For more detailed information about compiling the sources see [INSTALL](./INSTALL) and `./configure --help`

## Documentation

To generate documentation via autotools+doxygen execute `$ make doxygen-doc` after configuring the project.

