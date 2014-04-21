#include "kanim.hpp"

using namespace Krane;
using namespace std;

namespace Krane {
	const uint32_t KAnim::MAGIC_NUMBER = *reinterpret_cast<const uint32_t*>("ANIM");

	const int32_t KAnim::MIN_VERSION = 4;
	const int32_t KAnim::MAX_VERSION = 4;
}
