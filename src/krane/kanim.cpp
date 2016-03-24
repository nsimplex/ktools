#include "kanim.hpp"

/*
 * Animation suffixes for each of the FACING_ possibilities (see kanim.hpp).
 */




static const char SUFFIX_RIGHT[] = "_right";
static const char SUFFIX_UP[] = "_up";
static const char SUFFIX_LEFT[] = "_left";
static const char SUFFIX_DOWN[] = "_down";
static const char SUFFIX_UPRIGHT[] = "_upright";
static const char SUFFIX_UPLEFT[] = "_upleft";
static const char SUFFIX_DOWNRIGHT[] = "_downright";
static const char SUFFIX_DOWNLEFT[] = "_downleft";
static const char SUFFIX_SIDE[] = "_side";
static const char SUFFIX_UPSIDE[] = "_upside";
static const char SUFFIX_DOWNSIDE[] = "_downside";
static const char SUFFIX_45S[] = "_45s";
static const char SUFFIX_90S[] = "_90s";

using namespace std;

#define FACEOPT(byte, suf) \
	case byte : \
		fullname.append(suf, sizeof(suf) - 1); \
		break;

namespace Krane {
	void KAnim::getFullName(std::string& fullname) const {
		fullname.assign(getName());

		switch(facing_byte) {
			FACEOPT(FACING_RIGHT, SUFFIX_RIGHT)
			FACEOPT(FACING_UP, SUFFIX_UP)
			FACEOPT(FACING_LEFT, SUFFIX_LEFT)
			FACEOPT(FACING_DOWN, SUFFIX_DOWN)
			FACEOPT(FACING_UPRIGHT, SUFFIX_UPRIGHT)
			FACEOPT(FACING_UPLEFT, SUFFIX_UPLEFT)
			FACEOPT(FACING_DOWNRIGHT, SUFFIX_DOWNRIGHT)
			FACEOPT(FACING_DOWNLEFT, SUFFIX_DOWNLEFT)
			FACEOPT(FACING_SIDE, SUFFIX_SIDE)
			FACEOPT(FACING_UPSIDE, SUFFIX_UPSIDE)
			FACEOPT(FACING_DOWNSIDE, SUFFIX_DOWNSIDE)
			FACEOPT(FACING_45S, SUFFIX_45S)
			FACEOPT(FACING_90S, SUFFIX_90S)

			default:
				break;
		}
	}
}
