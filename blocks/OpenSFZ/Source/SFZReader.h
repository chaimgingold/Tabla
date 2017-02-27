#ifndef SFZReader_h
#define SFZReader_h

#include "SFZCommon.h"

class SFZRegion;
class SFZSound;


class SFZReader {
	public:
		SFZReader(SFZSound* sound);
		~SFZReader();

		void	read(const Path& file);
		void	read(const char* text, unsigned int length);

	protected:
		SFZSound*	sound;
		int	line;

		const char*	handleLineEnd(const char* p);
		const char*	readPathInto(std::string* pathOut, const char* p, const char* end);
		int 	keyValue(const std::string& str);
		int 	triggerValue(const std::string& str);
		int 	loopModeValue(const std::string& str);
		void	finishRegion(SFZRegion* region);
		void	error(const std::string& message);
	};


#endif 	// !SFZReader_h

