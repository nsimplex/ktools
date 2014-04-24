#include "kanim.hpp"

using namespace std;

namespace Krane {
	void KAnim::getFullName(std::string& fullname) const {
		static const char RIGHT_SUFFIX[] = "_right";
		static const char UP_SUFFIX[] = "_up";
		static const char LEFT_SUFFIX[] = "_left";
		static const char DOWN_SUFFIX[] = "_down";
		static const char SIDE_SUFFIX[] = "_side";

		fullname.assign(getName());

		switch(facing_byte) {
			case FACING_RIGHT:
				fullname.append(RIGHT_SUFFIX, sizeof(RIGHT_SUFFIX) - 1);
				break;

			case FACING_UP:
				fullname.append(UP_SUFFIX, sizeof(UP_SUFFIX) - 1);
				break;

			case FACING_LEFT:
				fullname.append(LEFT_SUFFIX, sizeof(LEFT_SUFFIX) - 1);
				break;

			case FACING_DOWN:
				fullname.append(DOWN_SUFFIX, sizeof(DOWN_SUFFIX) - 1);
				break;

			case FACING_SIDE:
				fullname.append(SIDE_SUFFIX, sizeof(SIDE_SUFFIX) - 1);
				break;

			default:
				break;
		}
	}
}
