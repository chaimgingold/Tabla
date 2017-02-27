#include "SF2Generator.h"
#include <stdio.h>	// just for NULL; where does that live really?

#define SF2GeneratorValue(name, type) 	\
	{ #name, SF2Generator::type }
static const SF2Generator generators[] = {
	#include "sf2-chunks/generators.h"
	};
#undef SF2GeneratorValue
static const int numGenerators = sizeof(generators) / sizeof(generators[0]);


const SF2Generator* GeneratorFor(unsigned short index)
{
	if (index >= numGenerators)
		return NULL;
	return &generators[index];
}



