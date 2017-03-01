#ifndef RIFF_h
#define RIFF_h

#include "WinTypes.h"
#include "InputStream.h"

typedef long int64;

struct RIFFChunk {
	enum Type {
		RIFF,
		LIST,
		Custom
		};

	fourcc	id;
	dword 	size;
	Type  	type;
	int64  	start;

	void	ReadFrom(InputStream* file);
	void	Seek(InputStream* file);
	void	SeekAfter(InputStream* file);
	int64	End() { return start + size; }

	std::string	ReadString(InputStream* file);
	};


#endif 	// !RIFF_h

