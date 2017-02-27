#pragma once

#include "SFZCommon.h"

class InputStream;

class Path
{
public:
	Path(std::string sz = std::string(""));
	Path(const Path &other);
	const std::string getFullPath() const { return fullPath; };

	std::string getPathUpToLastSlash() const;
    Path getParentDirectory() const;
    std::string getFileName() const;
    std::string getFileNameWithoutExtension() const;
    std::string getExtension() const;
    Path getChildFile (std::string relativePath) const;
    Path getSiblingFile (const std::string& fileName) const;
    std::string addTrailingSeparator (const std::string& path) const;
    
	static const char separator;
    static const std::string separatorString;

    InputStream *createInputStream() const;
        
	bool exists();
	bool isEmpty();
private:
	std::string fullPath;
};