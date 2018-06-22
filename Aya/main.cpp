#include "aya.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace std;

void help() {
	cout << "aya filename -o outputname [-m maskname] [-p processname] [-s scale]" << endl;
	exit(1);
}

int main(int argc, char **argv)
{
	if (argc < 4 || argc > 10 || argc%2) help();
	else {
		string filename, outputname, maskname, processname;
		float scale = -20;
		filename = argv[1];
		for (int i = 2; i < argc; i += 2) {
			if (!strcmp(argv[i], "-o")) outputname = argv[i + 1];
			else if (!strcmp(argv[i], "-m")) maskname = argv[i + 1];
			else if (!strcmp(argv[i], "-p")) processname = argv[i + 1];
			else if (!strcmp(argv[i], "-s")) {
				scale = atof(argv[i + 1]);
			}
			else help();
		}
		cout << filename << ' ' << outputname << endl;
		if (outputname == "") help();
		Main(filename, maskname, outputname, processname, scale);
	}
	return 0;
}