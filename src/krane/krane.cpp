#include "krane.hpp"

using namespace Krane;
using namespace std;


int main(int argc, char* argv[]) {
	string myarg = argc > 1 ? argv[1] : "";


	try {
		if(myarg == "build.bin") {
			KBuildFile bild;
			bild.loadFrom(myarg, 3);
		}
		else if(myarg == "anim.bin") {
			KAnimFile<> anim;
			anim.loadFrom(myarg, 2);
		}
		else {
			cerr << "Give me a build.bin or anim.bin!" << endl;
			exit(1);
		}
	}
	catch(exception& e) {
		cerr << e.what() << endl;
		return -1;
	}
	
	return 0;
}
