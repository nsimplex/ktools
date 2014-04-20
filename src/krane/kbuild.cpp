#include "kbuild.hpp"

using namespace std;

namespace Krane {
	const uint32_t KBuild::MAGIC_NUMBER = *reinterpret_cast<const uint32_t*>("BILD");
}
