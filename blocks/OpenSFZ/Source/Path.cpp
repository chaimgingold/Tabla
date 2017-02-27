#include "Path.h"
#include "InputStream.h"

#include <algorithm>

const char Path::separator = '/';
const std::string Path::separatorString("/");

Path::Path(std::string sz)
{
	fullPath = sz;

	std::replace( fullPath.begin(), fullPath.end(), '\\', '/');
}

Path::Path(const Path &other)
{
	fullPath = other.fullPath;
}

std::string Path::getPathUpToLastSlash() const
{
    const int lastSlash = fullPath.find_last_of (separator);

    if (lastSlash > 0)
        return fullPath.substr (0, lastSlash);
    else if (lastSlash == 0)
        return separatorString;
    else
        return fullPath;
}

Path Path::getParentDirectory() const
{
    Path f;
    f.fullPath = getPathUpToLastSlash();
    return f;
}

std::string Path::getFileName() const
{
    return fullPath.substr (fullPath.find_last_of (separator) + 1);
}

std::string Path::getFileNameWithoutExtension() const
{
    const int lastSlash = fullPath.find_last_of (separator) + 1;
    const int lastDot   = fullPath.find_last_of ('.');

    if (lastDot > lastSlash)
        return fullPath.substr (lastSlash, lastDot);
    else
        return fullPath.substr (lastSlash);
}

std::string Path::getExtension() const
{
    int lastDot   = fullPath.find_last_of ('.');
    
    
    if(lastDot == std::string::npos)
        return std::string("");

    
    std::string ext = fullPath.substr (lastDot + 1);
    
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext;
}


Path Path::getChildFile (std::string relativePath) const
{
    //if (isAbsolutePath (relativePath))
        //return File (relativePath);

    std::string path (fullPath);

    // It's relative, so remove any ../ or ./ bits at the start..
    if (relativePath[0] == '.')
    {
        while (relativePath[0] == '.')
        {
            const char secondChar = relativePath[1];

            if (secondChar == '.')
            {
                const char thirdChar = relativePath[2];

                if (thirdChar == 0 || thirdChar == separator)
                {
                    const int lastSlash = path.find_last_of (separator);
                    if (lastSlash >= 0)
                        path = path.substr (0, lastSlash);

                    relativePath = relativePath.substr (3);
                }
                else
                {
                    break;
                }
            }
            else if (secondChar == separator)
            {
                relativePath = relativePath.substr (2);
            }
            else
            {
                break;
            }
        }
    }

    return Path (addTrailingSeparator (path) + relativePath);
}

Path Path::getSiblingFile (const std::string& fileName) const
{
    return getParentDirectory().getChildFile (fileName);
}

std::string Path::addTrailingSeparator (const std::string& path) const
{
	if(path.length() == 0)
		return separatorString;

	if(path[path.length() - 1] != separator )
		return path + separator;
    
    return path;
}

bool Path::exists()
{
	std::ifstream my_file(fullPath.c_str());
	return my_file.good();

}

bool Path::isEmpty()
{
	return (fullPath == "");
}

InputStream *Path::createInputStream() const
{
    return new InputStream(fullPath);
    

}