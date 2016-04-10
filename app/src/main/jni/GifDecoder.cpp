#include <iostream>
#include <fstream>
#include <string.h>
#include "GifDecoder.h"

using std::cout;
using std::endl;
using std::ifstream;

int main() {
	char *file = "flower.gif";
	ifstream is(file, ifstream::binary);
	if (is.is_open()) {
		is.seekg(0, is.beg);
		processStream(is);
		is.close();
	}
}

void processStream(ifstream& is) {
	char header[6] = {};
	is.read(header, 6);
	if (!is) return;
	if (strncmp("GIF89a", header, 6)) {
		std::cerr << "input file is unsupported!";
		return;
	}

}