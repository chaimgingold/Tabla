//
//  InputStream.cpp
//  OpenSFZ
//
//  Created by David Wallin on 4/24/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//

#include "InputStream.h"


using namespace std;

InputStream::InputStream(const string &path)
{
    pFile = fopen ( path.c_str() , "rb" );
    
    if(pFile)
    {
    
        fseek (pFile , 0 , SEEK_END);
        lSize = ftell (pFile);
        rewind (pFile);
    } else {
        errors.push_back("File does not exist");
    }
}

InputStream::~InputStream()
{
    if(pFile)
        fclose(pFile);
}

size_t InputStream::read(void *data, size_t bytes)
{
    if(!pFile)
        return 0;
    
    size_t result = fread (data,1,bytes,pFile);
    
    if(result != bytes)
    {
        errors.push_back("Read error");
    }
    return result;
    
}

int InputStream::readInt()
{
    int i;
    
    read(&i, 4);
    
    return i;
}
int InputStream::readShort()
{
    char shortBuf[2];
    int i = 0;
    
    read(&shortBuf, 2);
    

#ifdef SFZ_BIGENDIAN
    i = (int)shortBuf[1] + ((int)shortBuf[0]<<8);
#else
    i = (int)shortBuf[0] + ((int)shortBuf[1]<<8);
#endif
    return i;
}

unsigned char InputStream::readByte()
{
    unsigned char i;
    
    read(&i, 1);
    
    return i;
    
}

void InputStream::setPosition(long pos)
{
    if(!pFile)
        return;

    
    if(fseek (pFile , pos , SEEK_SET))
        errors.push_back("Set error");
    
}

long InputStream::getPosition()
{
    return ftell(pFile);
}