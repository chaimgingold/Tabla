#include "SFZCommon.h"

#include "SF2.h"
#include "RIFF.h"


#define readAbyte(name, file) 	\
	name = (byte) file->readByte();
#define readAchar(name, file) 	\
	name = file->readByte();
#define readAdword(name, file) 	\
	name = (dword) file->readInt();
#define readAword(name, file) 	\
	name = (word) file->readShort();
#define readAshort(name, file) 	\
	name = file->readShort();
#define readAchar20(name, file) 	\
	file->read(name, 20);
#define readAgenAmountType(name, file) 	\
	name.shortAmount = file->readShort();

#define SF2Field(type, name) 	\
	readA##type(name, file)


void SF2::iver::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/iver.h"
}


void SF2::phdr::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/phdr.h"
}


void SF2::pbag::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/pbag.h"
}


void SF2::pmod::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/pmod.h"
}


void SF2::pgen::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/pgen.h"
}


void SF2::inst::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/inst.h"
}


void SF2::ibag::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/ibag.h"
}


void SF2::imod::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/imod.h"
}


void SF2::igen::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/igen.h"
}


void SF2::shdr::ReadFrom(InputStream* file)
{
	#include "sf2-chunks/shdr.h"
}


SF2::Hydra::Hydra()
	: phdrItems(NULL), pbagItems(NULL), pmodItems(NULL), pgenItems(NULL),
		instItems(NULL), ibagItems(NULL), imodItems(NULL), igenItems(NULL),
		shdrItems(NULL)
{
}


SF2::Hydra::~Hydra()
{
	delete phdrItems;
	delete pbagItems;
	delete pmodItems;
	delete pgenItems;
	delete instItems;
	delete ibagItems;
	delete imodItems;
	delete igenItems;
	delete shdrItems;
}


void SF2::Hydra::ReadFrom(InputStream* file, int64 pdtaChunkEnd)
{
	int i, numItems;

	#define HandleChunk(chunkName) 	\
		if (FourCCEquals(chunk.id, #chunkName)) { 	\
			numItems = chunk.size / SF2::chunkName::sizeInFile; 	\
			chunkName##NumItems = numItems; 	\
			chunkName##Items = new SF2::chunkName[numItems]; 	\
			for (i = 0; i < numItems; ++i) 	\
				chunkName##Items[i].ReadFrom(file); 	\
			} 	\
		else

	while (file->getPosition() < pdtaChunkEnd) {
		RIFFChunk chunk;
		chunk.ReadFrom(file);

		HandleChunk(phdr)
		HandleChunk(pbag)
		HandleChunk(pmod)
		HandleChunk(pgen)
		HandleChunk(inst)
		HandleChunk(ibag)
		HandleChunk(imod)
		HandleChunk(igen)
		HandleChunk(shdr)
		{}

		chunk.SeekAfter(file);
		}
}


bool SF2::Hydra::IsComplete()
{
	return
		phdrItems && pbagItems && pmodItems && pgenItems &&
		instItems && ibagItems && imodItems && igenItems &&
		shdrItems;
}



