#include "krane.hpp"

using namespace Krane;
using namespace std;


int main(int argc, char* argv[]) {
	string bildname = argc > 1 ? argv[1] : "";

	if(bildname != "build.bin") {
		cerr << "Give me a build.bin!" << endl;
		exit(1);
	}
	
	try {
		KBuild bild;
		bild.loadFrom(bildname, 4);
	}
	catch(exception& e) {
		cerr << e.what() << endl;
		return -1;
	}
	
	return 0;
}
