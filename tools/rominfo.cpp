/* Программа читает информацию об образе */

#include <iostream>
#include <fstream>
#include <ines.h>

#define USAGE \
	"Usage: rominfo romfile\n" \
	" Read header of ROM file\n\n" \
	"Report bugs to <viktprog@gmail.com>"

int main(int argc, char *argv[]) {
	vpnes::ines::NES_ROM_Data Data;
	std::ifstream ROM;

	if (argc != 2) {
		std::cerr << USAGE << std::endl;
		return 0;
	}
	ROM.open(argv[1], std::ios_base::in | std::ios_base::binary);
	if (ROM.fail())
		return -1;
	if (vpnes::ReadROM(ROM, &Data) < 0) {
		ROM.close();
		return -1;
	}
	std::cerr << "ROM: " << Data.Header.Mapper << " mapper, PRG " << Data.Header.PRGSize <<
		", W-RAM " << Data.Header.RAMSize << (Data.Header.HaveBattery ? " (battery "
		"backed), CHR " : " (no battery), CHR " ) << Data.Header.CHRSize <<
		((Data.CHR == NULL) ? " RAM, System " : ", System ") << Data.Header.TVSystem <<
		", Mirroring " << Data.Header.Mirroring << ((Data.Trainer == NULL) ?
		", no trainer" : ", have trainer 512") << std::endl;
	switch (Data.Header.Mapper) {
		case 0:
		case 1:
		case 2:
		case 4:
		case 7:
			return 0;
	}
	return -1;
}
