//
//  MusicStamp.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/21/16.
//
//

#ifndef MusicStamp_hpp
#define MusicStamp_hpp

#include "Instrument.h"
#include "cinder/gl/gl.h"

using namespace ci;

class MusicStamp
{
public:

	void draw() const;
	
	vec2 mTimeVec = vec2(1,0);
	vec2 mLoc;
	gl::TextureRef mIcon;
	float mIconWidth = 10.f;

	InstrumentRef mInstrument; // renders mIcon redundant (for right now)
	
private:
	void drawInstrumentIcon( vec2 worldx, tInstrumentIconAnimState pose ) const;
	
};

#endif /* MusicStamp_hpp */
