//============================================================================
// Name        : lpccksum.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <fstream>
#include <stdint.h>

using namespace std;

int main(int argc, char *argv[])
{
cout << "LPC checksum - by L. Mini" << endl;
if(argc != 2)
	{
	cout << "USAGE:\n";
	cout << argv[0] << " <file_bin>\n";
	cout << endl;
	}
else
	{
	streampos size;
	char * memblock;

	ifstream ifile(argv[1], ios::in | ios::binary | ios::ate);
	if(ifile.is_open())
		{
		size = ifile.tellg();
		memblock = new char[size];
		ifile.seekg(0, ios::beg);
		ifile.read(memblock, size);
		ifile.close();

		cout << "file read -> patching it"<<endl;

		uint32_t *ptr=(uint32_t*)memblock;     // the base of your (binarray) image
		uint32_t cks = 0;

		for(int i = 0; i < 7; i++)
			{
			cks += *ptr++;
			}

		cks = (~cks) + 1;
		*ptr = cks;               // ptr left pointing to eigth entry at end of loop

		// write data
		ofstream ofile(argv[1], ios::out | ios::binary | ios::ate);
		ofile.write(memblock,size);
		ofile.close();
		cout << "-- DONE --"<<endl;

		delete[] memblock;
		}
	else
		{
		cout << "Unable to open file" << endl;
		}
	}

return 0;
}
