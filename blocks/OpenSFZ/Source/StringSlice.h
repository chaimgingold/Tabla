#ifndef StringSlice_h
#define StringSlice_h

#include <string.h>



class StringSlice {
	public:
		StringSlice(const char* startIn, const char* endIn)
			: start(startIn), end(endIn) {}

		unsigned int length() {
			return end - start;
			}

		bool operator==(const char* other) {
			return strncmp(start, other, length()) == 0;
			}
		bool operator!=(const char* other) {
			return strncmp(start, other, length()) != 0;
			}

		const char*	start;
		const char*	end;
	};


#endif 	// !StringSlice_h

