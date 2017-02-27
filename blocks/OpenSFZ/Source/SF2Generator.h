#ifndef SF2Generator_h
#define SF2Generator_h


struct SF2Generator {
	enum Type {
		Word, Short, Range
		};

	const char* name;
	Type	type;

	#define SF2GeneratorValue(name, type)	name
	enum {
		#include "sf2-chunks/generators.h"
		};
	#undef SF2GeneratorValue
	};

extern const SF2Generator* GeneratorFor(unsigned short index);


#endif 	// !SF2Generator_h

