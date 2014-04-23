#ifndef KTOOLS_METAPROGRAMMING_HPP
#define KTOOLS_METAPROGRAMMING_HPP

#include "ktools_common.hpp"

namespace KTools {
	template<bool condition>
	class StaticAssertionChecker {
		char static_assertion_failed[condition ? 1 : -1];
	};

	template<bool condition>
	inline void staticAssert() {
		typedef StaticAssertionChecker<condition> CHECKER;
		if(0) {
			(void)CHECKER();
		}
	}

	/*
	 * C++11 has this functionality in type_traits, but I don't want to
	 * require C++11.
	 */
	struct TrueType {
		static const bool value = true;
	};

	struct FalseType {
		static const bool value = false;
	};

	template<typename T>
	struct IsIntegral : public FalseType {};

	template<>
	struct IsIntegral<char> : public TrueType {};

	template<>
	struct IsIntegral<unsigned char> : public TrueType {};

	template<>
	struct IsIntegral<short> : public TrueType {};

	template<>
	struct IsIntegral<unsigned short> : public TrueType {};

	template<>
	struct IsIntegral<int> : public TrueType {};

	template<>
	struct IsIntegral<unsigned int> : public TrueType {};

	template<>
	struct IsIntegral<long> : public TrueType {};

	template<>
	struct IsIntegral<unsigned long> : public TrueType {};

#ifdef HAVE_LONG_LONG
	template<>
	struct IsIntegral<long long> : public TrueType {};

	template<>
	struct IsIntegral<unsigned long long> : public TrueType {};
#endif

	template<typename T>
	struct IsFloating : public FalseType {};

	template<>
	struct IsFloating<float> : public TrueType {};

	template<>
	struct IsFloating<double> : public TrueType {};


	template<typename T>
	inline void staticAssertIntegral() {
		staticAssert< IsIntegral<T>::value >();
	}

	template<typename T>
	inline void staticAssertFloating() {
		staticAssert< IsFloating<T>::value >();
	}
}

#endif
