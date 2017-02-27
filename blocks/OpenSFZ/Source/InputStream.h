//
//  InputStream.h
//  OpenSFZ
//
//  Created by David Wallin on 4/24/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//

#ifndef __OpenSFZ__InputStream__
#define __OpenSFZ__InputStream__

#include <iostream>
#include "SFZCommon.h"

class InputStream
{
public:
    InputStream(const std::string &path);
    ~InputStream();
    
    long getSize() { return lSize; };
    
    void setPosition(long pos);
    long getPosition();
    size_t read(void *data, size_t bytes);
    int readInt();
    int readShort();
    unsigned char readByte();
    std::vector<std::string> errors;
private:
    long lSize;
    FILE * pFile;
};

#endif /* defined(__OpenSFZ__InputStream__) */
