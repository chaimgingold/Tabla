#include "SFZReader.h"
#include "SFZRegion.h"
#include "SFZSound.h"
#include "StringSlice.h"

#include <sstream>
typedef long int64;

SFZReader::SFZReader(SFZSound* soundIn)
	: sound(soundIn), line(1)
{
}


SFZReader::~SFZReader()
{
}


void SFZReader::read(const Path& file)
{

    
    
    std::ifstream t;
    unsigned int length;
    
    t.open(file.getFullPath().c_str());      // open input file
    
    if(!t.good())
    {
        sound->addError("Couldn't read \"" + file.getFullPath() + "\"");
        return;
    }
    
    t.seekg(0, std::ios::end);    // go to the end
    length = (unsigned int)t.tellg();           // report location (this is the length)
    t.seekg(0, std::ios::beg);    // go back to the beginning
    char *buffer = new char[length];    // allocate memory for a buffer of appropriate dimension
    t.read(buffer, length);       // read the whole file into the buffer
    t.close();

	read((const char*) buffer, (unsigned int)length);
    
    delete[] buffer;
}


void SFZReader::read(const char* text, unsigned int length)
{
	const char* p = text;
	const char* end = text + length;
	char c;

	SFZRegion curGroup;
	SFZRegion curRegion;
	SFZRegion* buildingRegion = NULL;
	bool inControl = false;
    std::string defaultPath;

	while (p < end) {
		// We're at the start of a line; skip any whitespace.
		while (p < end) {
			c = *p;
			if (c != ' ' && c != '\t')
				break;
			p += 1;
			}
		if (p >= end)
			break;

		// Check if it's a comment line.
		if (c == '/') {
			// Skip to end of line.
			while (p < end) {
				c = *++p;
				if (c == '\n' || c == '\r')
					break;
				}
			p = handleLineEnd(p);
			continue;
			}

		// Check if it's a blank line.
		if (c == '\r' || c == '\n') {
			p = handleLineEnd(p);
			continue;
			}

		// Handle elements on the line.
		while (p < end) {
			c = *p;

			// Tag.
			if (c == '<')
            {
				p += 1;
				const char* tagStart = p;
                
				while (p < end)
                {
					c = *p++;
					if (c == '\n' || c == '\r')
                    {
						error("Unterminated tag");
						goto fatalError;
                    }
					else if (c == '>')
						break;
					}
				if (p >= end) {
					error("Unterminated tag");
					goto fatalError;
					}
				StringSlice tag(tagStart, p - 1);
				if (tag == "region")
                {
					if (buildingRegion && buildingRegion == &curRegion)
						finishRegion(&curRegion);
                    
					curRegion = curGroup;
					buildingRegion = &curRegion;
					inControl = false;
                }
				else if (tag == "group")
                {
					if (buildingRegion && buildingRegion == &curRegion)
						finishRegion(&curRegion);
                    
					curGroup.clear();
					buildingRegion = &curGroup;
					inControl = false;
                }
				else if (tag == "control")
                {
					if (buildingRegion && buildingRegion == &curRegion)
						finishRegion(&curRegion);
                    
					curGroup.clear();
					buildingRegion = NULL;
					inControl = true;
					}
				else
					error("Illegal tag");
				}

			// Comment.
			else if (c == '/')
            {
				// Skip to end of line.
				while (p < end)
                {
					c = *p;
					if (c == '\r' || c == '\n')
						break;
					p += 1;
                }
            }

			// Parameter.
			else {
				// Get the parameter name.
				const char* parameterStart = p;
				while (p < end) {
					c = *p++;
					if (c == '=' || c == ' ' || c == '\t' || c == '\r' || c == '\n')
						break;
					}
				if (p >= end || c != '=')
                {
					error("Malformed parameter");
					goto nextElement;
                }
				StringSlice opcode(parameterStart, p - 1);
				if (inControl) {
					if (opcode == "default_path")
						p = readPathInto(&defaultPath, p, end);
					else {
						const char* valueStart = p;
						while (p < end) {
							c = *p;
							if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
								break;
							p++;
							}
						std::string value(valueStart, p - valueStart);
						std::string fauxOpcode = std::string(opcode.start, opcode.length()) + std::string(" (in <control>)");
						sound->addUnsupportedOpcode(fauxOpcode);
						}
					}
				else if (opcode == "sample") {
                    std::string path;
					p = readPathInto(&path, p, end);
					if (path != "") {
						if (buildingRegion)
							buildingRegion->sample = sound->addSample(path, defaultPath);
						else
							error("Adding sample outside a group or region");
						}
					else
						error("Empty sample path");
					}
				else {
					const char* valueStart = p;
					while (p < end) {
						c = *p;
						if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
							break;
						p++;
                    }
                    std::string value(valueStart, p - valueStart);
                    
                    std::istringstream ss(value);
                    
					if (buildingRegion == NULL)
						error("Setting a parameter outside a region or group");
					else if (opcode == "lokey")
						buildingRegion->lokey = keyValue(value);
					else if (opcode == "hikey")
						buildingRegion->hikey = keyValue(value);
					else if (opcode == "key") {
						buildingRegion->hikey =
						buildingRegion->lokey =
						buildingRegion->pitch_keycenter =
							keyValue(value);
						}
					else if (opcode == "lovel")
                    {
                        int i;
                        ss >> i;
						buildingRegion->lovel = i;
					} else if (opcode == "hivel") {
                        int i;
                        ss >> i;

						buildingRegion->hivel = i;
                    }
					else if (opcode == "trigger")
						buildingRegion->trigger = (SFZRegion::Trigger) triggerValue(value);
					else if (opcode == "group") {
                        unsigned long l;
                        ss >> l;
                        
						buildingRegion->group = l;
                    }
					else if (opcode == "off_by") {
                        unsigned long l;
                        ss >> l;

						buildingRegion->off_by = l;
                    } else if (opcode == "offset") {
                        unsigned long l;
                        ss >> l;

						buildingRegion->offset = l;
                    }
					else if (opcode == "end") {
                        unsigned long l;
                        ss >> l;

						int64 end = l;
						if (end < 0)
							buildingRegion->negative_end = true;
						else
							buildingRegion->end = end;
						}
					else if (opcode == "loop_mode") {
						bool modeIsSupported =
							value == "no_loop" ||
							value == "one_shot" ||
							value == "loop_continuous";
						if (modeIsSupported)
							buildingRegion->loop_mode = (SFZRegion::LoopMode) loopModeValue(value);
						else {
                            std::string fauxOpcode = std::string(opcode.start, opcode.length()) + "=" + value;
							sound->addUnsupportedOpcode(fauxOpcode);
							}
						}
					else if (opcode == "loop_start")
						ss >> buildingRegion->loop_start;
					else if (opcode == "loop_end")
						ss >> buildingRegion->loop_end;
					else if (opcode == "transpose")
						ss >> buildingRegion->transpose;
					else if (opcode == "tune")
						ss >> buildingRegion->tune;
					else if (opcode == "pitch_keycenter")
						buildingRegion->pitch_keycenter = keyValue(value);
					else if (opcode == "pitch_keytrack")
						ss >> buildingRegion->pitch_keytrack;
					else if (opcode == "bend_up")
						ss >> buildingRegion->bend_up;
					else if (opcode == "bend_down")
						ss >> buildingRegion->bend_down;
					else if (opcode == "volume")
						ss >> buildingRegion->volume;
					else if (opcode == "pan")
						ss >> buildingRegion->pan;
					else if (opcode == "amp_veltrack")
						ss >> buildingRegion->amp_veltrack;
					else if (opcode == "ampeg_delay")
						ss >> buildingRegion->ampeg.delay;
					else if (opcode == "ampeg_start")
						ss >> buildingRegion->ampeg.start;
					else if (opcode == "ampeg_attack")
						ss >> buildingRegion->ampeg.attack;
					else if (opcode == "ampeg_hold")
						ss >> buildingRegion->ampeg.hold;
					else if (opcode == "ampeg_decay")
						ss >> buildingRegion->ampeg.decay;
					else if (opcode == "ampeg_sustain")
						ss >> buildingRegion->ampeg.sustain;
					else if (opcode == "ampeg_release")
						ss >> buildingRegion->ampeg.release;
					else if (opcode == "ampeg_vel2delay")
						ss >> buildingRegion->ampeg_veltrack.delay;
					else if (opcode == "ampeg_vel2attack")
						ss >> buildingRegion->ampeg_veltrack.attack;
					else if (opcode == "ampeg_vel2hold")
						ss >> buildingRegion->ampeg_veltrack.hold;
					else if (opcode == "ampeg_vel2decay")
						ss >> buildingRegion->ampeg_veltrack.decay;
					else if (opcode == "ampeg_vel2sustain")
						ss >> buildingRegion->ampeg_veltrack.sustain;
					else if (opcode == "ampeg_vel2release")
						ss >> buildingRegion->ampeg_veltrack.release;
					else if (opcode == "default_path")
						error("\"default_path\" outside of <control> tag");
					else
						sound->addUnsupportedOpcode(std::string(opcode.start, opcode.length()));
					}
				}

			// Skip to next element.
nextElement:
			c = 0;
			while (p < end)
            {
				c = *p;
				if (c != ' ' && c != '\t')
					break;
				p += 1;
            }
			if (c == '\r' || c == '\n') {
				p = handleLineEnd(p);
				break;
				}
			}
		}

fatalError:
	if (buildingRegion && buildingRegion == &curRegion)
		finishRegion(buildingRegion);
}


const char* SFZReader::handleLineEnd(const char* p)
{

	// Check for DOS-style line ending.
	char lineEndChar = *p++;
	if (lineEndChar == '\r' && *p == '\n')
		p = p + 1;
	line += 1;
	return p;
}


const char* SFZReader::readPathInto(
                                    std::string* pathOut, const char* pIn, const char* endIn)
{
	// Paths are kind of funny to parse because they can contain whitespace.
	const char* p = pIn;
	const char* end = endIn;
	const char* pathStart = p;
	const char* potentialEnd = NULL;
	while (p < end) {
		char c = *p;
		if (c == ' ') {
			// Is this space part of the path?  Or the start of the next opcode?  We
			// don't know yet.
			potentialEnd = p;
			p += 1;
			// Skip any more spaces.
			while (p < end && *p == ' ')
				p += 1;
			}
		else if (c == '\n' || c == '\r' || c == '\t')
			break;
		else if (c == '=') {
			// We've been looking at an opcode; we need to rewind to
			// potentialEnd.
			p = potentialEnd;
			break;
			}
		p += 1;
		}
	if (p > pathStart) {
		// Can't do this:
		//  	std::string path(CharPointer_UTF8(pathStart), CharPointer_UTF8(p));
		// It won't compile for some unfathomable reason.

        
        
        std::string path(pathStart, p - pathStart);
        
//		CharPointer_UTF8 end(p);
		//String path(CharPointer_UTF8(pathStart), p);
		*pathOut = path;
		}
	else
		*pathOut = "";
	return p;
}


int SFZReader::keyValue(const std::string & str)
{
	char c = str[0];
    
    
	if (c >= '0' && c <= '9')
    {
        std::istringstream ss(str);
        int i;
        ss >> i;
		return i;
    }

	int note = 0;
	static const int notes[] = {
		12 + 0, 12 + 2, 3, 5, 7, 8, 10,
		};
	if (c >= 'A' && c <= 'G')
		note = notes[c - 'A'];
	else if (c >= 'a' && c <= 'g')
		note = notes[c - 'a'];
	int octaveStart = 1;
	c = str[1];
	if (c == 'b' || c == '#') {
		octaveStart += 1;
		if (c == 'b')
			note -= 1;
		else
			note += 1;
		}
    
    std::istringstream ss2(str.substr(octaveStart));
    
	int octave;
    ss2 >> octave;
	// A3 == 57.
	int result = octave * 12 + note + (57 - 4 * 12);
	return result;
}


int SFZReader::triggerValue(const std::string& str)
{
	if (str == "release")
		return SFZRegion::release;
	else if (str == "first")
		return SFZRegion::first;
	else if (str == "legato")
		return SFZRegion::legato;
	return SFZRegion::attack;
}


int SFZReader::loopModeValue(const std::string& str)
{
	if (str == "no_loop")
		return SFZRegion::no_loop;
	else if (str == "one_shot")
		return SFZRegion::one_shot;
	else if (str == "loop_continuous")
		return SFZRegion::loop_continuous;
	else if (str == "loop_sustain")
		return SFZRegion::loop_sustain;
	return SFZRegion::sample_loop;
}


void SFZReader::finishRegion(SFZRegion* region)
{
	SFZRegion* newRegion = new SFZRegion();
	*newRegion = *region;
	sound->addRegion(newRegion);
}


void SFZReader::error(const std::string& message)
{
    std::string fullMessage = message;
    
    std::stringstream ss;
    ss << " (line " << line << ").";
	fullMessage += ss.str();
	sound->addError(fullMessage);
}



