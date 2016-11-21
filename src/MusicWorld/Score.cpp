#include "MusicWorld.h"
#include "cinder/gl/Context.h"
#include "CinderOpenCV.h"
#include "geom.h"

using namespace ci::gl;

void drawLines( vector<vec2> points )
{
	const int dims = 2;
	const int size = sizeof( vec2 ) * points.size();
	auto ctx = context();
	const GlslProg* curGlslProg = ctx->getGlslProg();
	if( ! curGlslProg ) {
//		CI_LOG_E( "No GLSL program bound" );
		return;
	}

	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();

	VboRef arrayVbo = ctx->getDefaultArrayVbo( size );
	ScopedBuffer bufferBindScp( arrayVbo );

	arrayVbo->bufferSubData( 0, size, points.data() );
	int posLoc = curGlslProg->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, dims, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)nullptr );
	}
	ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_LINES, 0, points.size() );
	ctx->popVao();
}

void drawSolidTriangles( vector<vec2> pts )
{
	auto ctx = context();
	const GlslProg* curGlslProg = ctx->getGlslProg();
	if( ! curGlslProg ) {
//		CI_LOG_E( "No GLSL program bound" );
		return;
	}

//	GLfloat data[3*2+3*2]; // both verts and texCoords
//	memcpy( data, pts, sizeof(float) * pts.size() * 2 );
//	if( texCoord )
//		memcpy( data + 3 * 2, texCoord, sizeof(float) * 3 * 2 );

	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();
	VboRef defaultVbo = ctx->getDefaultArrayVbo( sizeof(float)*pts.size()*2 );
	ScopedBuffer bufferBindScp( defaultVbo );
	defaultVbo->bufferSubData( 0, sizeof(float) * pts.size() * 2, &pts[0] );

	int posLoc = curGlslProg->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	}
//	if( texCoord ) {
//		int texLoc = curGlslProg->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
//		if( texLoc >= 0 ) {
//			enableVertexAttribArray( texLoc );
//			vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*6) );
//		}
//	}
	ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_TRIANGLES, 0, pts.size() );
	ctx->popVao();
}

bool Score::isScoreValueHigh( uchar value ) const
{
	const int kValueThresh = 100;

	return value < kValueThresh ;
	// if dark, then high
	// else low
}

bool Score::isNoteOn( float playheadFrac, int note ) const
{
	int x = constrain( (int)(playheadFrac * (float)(mQuantizedImage.cols)), 0, mQuantizedImage.cols-1 );
	int y = mNoteCount-1-note;
	if (y >= mQuantizedImage.rows || y < 0 || x < 0) {
		return false;
	}
	bool isOn = isScoreValueHigh(mQuantizedImage.at<unsigned char>(y,x));
	
	return isOn;
}

int Score::getNoteLengthAsImageCols( cv::Mat image, int x, int y ) const
{
	int x2;

	for( x2=x;
		 x2<image.cols
			&& isScoreValueHigh(image.at<unsigned char>(image.rows-1-y,x2)) ;
		 ++x2 )
	{}

	int len = x2-x;

	return len;
}

float Score::getNoteLengthAsScoreFrac( cv::Mat image, int x, int y ) const
{
	// can return 0, which means we filtered out super short image noise-notes

	return (float)getNoteLengthAsImageCols(image,x,y) / (float)image.cols;
}

int Score::drawNotes( GameWorld::DrawType drawType ) const
{
	int numOnNotes=0;
	
	// collect notes into a batch
	// (probably wise to extract this geometry/data when processing vision data,
	// then use it for both playback and drawing).
	const float invcols = 1.f / (float)(mQuantizedImage.cols);

	const float yheight = 1.f / (float)mNoteCount;


	vector<vec2> onNoteTris;
	vector<vec2> offNoteTris;

	const float playheadFrac = getPlayheadFrac();

	for ( int y=0; y<mNoteCount; ++y )
	{
		const float y1frac = y * yheight + yheight*.2f;
		const float y2frac = y * yheight + yheight*.8f;

		for( int x=0; x<mQuantizedImage.cols; ++x )
		{
			if ( isScoreValueHigh(mQuantizedImage.at<unsigned char>(mNoteCount-1-y,x)) )
			{
				// how wide?
				int length = getNoteLengthAsImageCols(mQuantizedImage,x,y);

				const float x1frac = x * invcols;
				const float x2frac = min( 1.f, (x+length) * invcols );

				vec2 start1 = fracToQuad( vec2( x1frac, y1frac) ) ;
				vec2 end1   = fracToQuad( vec2( x2frac, y1frac) ) ;

				vec2 start2 = fracToQuad( vec2( x1frac, y2frac) ) ;
				vec2 end2   = fracToQuad( vec2( x2frac, y2frac) ) ;

				const bool isInFlight = playheadFrac > x1frac && playheadFrac < x2frac;
				if (isInFlight) numOnNotes++;
				
				vector<vec2>& tris = isInFlight ? onNoteTris : offNoteTris ;

				tris.push_back( start1 );
				tris.push_back( start2 );
				tris.push_back( end2 );
				tris.push_back( start1 );
				tris.push_back( end2 );
				tris.push_back( end1 );

				// skip ahead
				x = x + length+1;
			}
		}
	}

	// draw batched notes
	if (!onNoteTris.empty())
	{
		gl::color(mInstrument->mNoteOnColor);
		drawSolidTriangles(onNoteTris);
	}
	if (!offNoteTris.empty())
	{
		gl::color(mInstrument->mNoteOffColor);
		drawSolidTriangles(offNoteTris);
	}
	
	return numOnNotes;
}

void Score::drawScoreLines( GameWorld::DrawType drawType ) const
{
	// TODO: Make this configurable for 5/4 time, etc.
	const int drawBeatDivision = 4;

	vector<vec2> pts;

	gl::color(mInstrument->mScoreColor);
	gl::draw( getPolyLine() );

	// Scale lines
	for( int i=0; i<mNoteCount; ++i )
	{
		float f = (float)(i+1) / (float)mNoteCount;

		pts.push_back( lerp(mQuad[3], mQuad[0],f) );
		pts.push_back( lerp(mQuad[2], mQuad[1],f) );
	}

	// Off-beat lines
	for( int i=0; i<mBeatCount; ++i )
	{
		if (i % drawBeatDivision == 0) continue;

		float f = (float)i / (float)mBeatCount;

		pts.push_back( lerp(mQuad[1], mQuad[0],f) );
		pts.push_back( lerp(mQuad[2], mQuad[3],f) );
	}

	drawLines(pts);

	// New points for new colors
	pts.clear();
	// Down-beat lines
	for( int i=0; i<mBeatCount; ++i )
	{
		if (i % drawBeatDivision != 0) continue;

		float f = (float)i / (float)mBeatCount;

		pts.push_back( lerp(mQuad[1], mQuad[0],f) );
		pts.push_back( lerp(mQuad[2], mQuad[3],f) );
	}
	vec3 scoreColorHSV = rgbToHsv(mInstrument->mScoreColor);
	scoreColorHSV.x = fmod(scoreColorHSV.x + 0.5, 1.0);
	gl::color( hsvToRgb(scoreColorHSV)  );
	drawLines(pts);

}

void Score::drawPlayhead( GameWorld::DrawType drawType ) const
{
	gl::color( mInstrument->mPlayheadColor );

	vec2 playhead[2];
	getPlayheadLine(playhead);
	gl::drawLine( playhead[0], playhead[1] );

	// octave indicator
	float octaveFrac = (float)mOctave / (float)mNumOctaves + .5f ;

	gl::drawSolidCircle( lerp(playhead[0],playhead[1],octaveFrac), .5f, 6 );
}

void Score::drawMetaParam( GameWorld::DrawType drawType ) const
{
	Rectf r( mQuad[0], mQuad[2] );
	r.canonicalize();

	gl::color(mInstrument->mScoreColor);
	gl::draw( getPolyLine() );

	// level
	float y1, y2;
	if ( mInstrument->mMetaParamInfo.isDiscrete() )
	{
		if ( drawType != GameWorld::DrawType::UIPipelineThumb ) // optimization
		{
			vector<vec2> lines;

			for( int i=0; i<mInstrument->mMetaParamInfo.mNumDiscreteStates; ++i )
			{
				float y = (float)i/(float)(mInstrument->mMetaParamInfo.mNumDiscreteStates);
				lines.push_back( fracToQuad(vec2(0.f,y)) );
				lines.push_back( fracToQuad(vec2(1.f,y)) );
			}

			drawLines(lines);
		}

//		y1=mInstrument->mMetaParamInfo.discretize(mMetaParamSliderValue);
//		y2=y1 + 1.f / (float)mInstrument->mMetaParamInfo.mNumDiscreteStates;

		y1=mInstrument->mMetaParamInfo.discretize(mMetaParamSliderValue);
		y2=y1 + 1.f / (float)mInstrument->mMetaParamInfo.mNumDiscreteStates;
	}
	else
	{
		float kcm = 1.f;
		float k = kcm / getSizeInWorldSpace().y ;

		// thick band @ level
		//y1 = max( 0.f, mMetaParamSliderValue - k );
		//y2 = min( 1.f, mMetaParamSliderValue + k );

		// a meter filled up to the level
		y1 = 0.f;
		y2 = max( k, mMetaParamSliderValue );
	}

	if ( mMetaParamSliderValue != -1.f )
	{
		PolyLine2 p;
		p.push_back( fracToQuad(vec2(0.f,y1)) );
		p.push_back( fracToQuad(vec2(0.f,y2)) );
		p.push_back( fracToQuad(vec2(1.f,y2)) );
		p.push_back( fracToQuad(vec2(1.f,y1)) );
		p.setClosed();
		gl::drawSolid(p);
	}
}

void Score::drawInstrumentIcon( tInstrumentIconAnimState pose ) const
{
	// instrument?
	if ( !mInstrument || !mInstrument->mIcon ) return;
	
	// draw
	const float kIconWidth  = 5.f ;
	const float kIconGutter = 1.f ;
	// in world space
	
	//
	const vec2 playheadVec = getPlayheadVec();
	
	vec2 iconLoc = fracToQuad(vec2(0,.5f));
	iconLoc -= playheadVec * (kIconGutter + kIconWidth*.5f);
	iconLoc += pose.mTranslate * vec2(kIconWidth,kIconWidth) ;
	
	//
	Rectf r(-.5f,-.5f,.5f,.5f);
	
	gl::pushModelMatrix();
	gl::translate( iconLoc - r.getCenter() );
	gl::scale( vec2(1,1)*kIconWidth * pose.mScale );
	
	gl::multModelMatrix( mat4( ci::mat3(
		vec3( playheadVec,0),
		vec3( perp(playheadVec),0),
		vec3( cross(vec3(playheadVec,0),vec3(perp(playheadVec),0)) )
		)) );
	gl::rotate( pose.mRotate );
	
	gl::color( pose.mColor );
	
	if (mInstrument && mInstrument->mIcon) gl::draw( mInstrument->mIcon, r );
	else gl::drawSolidRect(r);
	
	gl::popModelMatrix();
}

tInstrumentIconAnimState Score::getInstrumentIconPoseFromScore( float playheadFrac ) const
{
	const vec4 poses[13] =
	{
		vec4(  .25, 0.f, 1.f, -30 ),
		vec4( -.25, 0.f, 1.f,  30 ),
		vec4( 0, 0, 1, 0 ),
		vec4( 0, .4, 1, 0 ),
		vec4( 0, 0, 1, 0 ),
		vec4(  .25, 0.f, 1.f, -30 ),
		vec4( -.25, 0.f, 1.f,  30 ),
		vec4( 0, 0, 1, 20 ),
		vec4( 0, 0, 1, -20 ),
		vec4( 0, .2, 1.2, 0 ),
		vec4( 0, .2, 1.2, 0 ),
		vec4( 0, -.3, 1, 0 ),
		vec4( 0, 0, 1.2, 0 )
	};
	
	// gather
	int numOnNotes=0;
	vec4 pose;
	vec4 poseNormSum;
	
	for( int note=0; note<mNoteCount; note++ )
	{
		if ( isNoteOn(playheadFrac,note) )
		{
			numOnNotes++;
			pose += poses[note%13];
			poseNormSum += vec4(1,1,1,1);
		}
	}
	
	// blend
	if (numOnNotes > 0) {
		pose /= poseNormSum;
	} else {
		pose = vec4( 0, 0, 1, 0); // default pose
	}
	
	// pose
	tInstrumentIconAnimState state = pose;
	
	if (mInstrument) {
		state.mColor = (numOnNotes>0) ? mInstrument->mNoteOnColor : mInstrument->mNoteOffColor;
	} else {
		state.mColor = ColorA(1,1,1,1);
	}
	
	return state;
}

void Score::draw( GameWorld::DrawType drawType ) const
{
	bool drawSolidColorBlocks = drawType == GameWorld::DrawType::Projector;
		// copy-pasted from MusicWorld::draw(). should be an accessor function.

	if (drawSolidColorBlocks)
	{
		gl::color(0,0,0);
		gl::drawSolid(getPolyLine());
	}

	if (mInstrument)
	{
		switch( mInstrument->mSynthType )
		{
			case Instrument::SynthType::Additive:
			{
				if ( mAdditiveShader )
				{
					auto tex = gl::Texture::create( ImageSourceRef( new ImageSourceCvMat(mImage)) );
					gl::ScopedGlslProg glslScp( mAdditiveShader );
					gl::ScopedTextureBind texScp( tex );

					mAdditiveShader->uniform( "uTex0", 0 );
					mAdditiveShader->uniform( "uTime", (float)ci::app::getElapsedSeconds() );
					mAdditiveShader->uniform( "uPhase", getPlayheadFrac());
					mAdditiveShader->uniform( "uAspectRatio", getWindowAspectRatio() );

					vec2 v[6];
					vec2 uv[6];

					v[0] = mQuad[0];
					v[1] = mQuad[1];
					v[2] = mQuad[3];
					v[3] = mQuad[3];
					v[4] = mQuad[1];
					v[5] = mQuad[2];

					float y0=1.f, y1=0.f; // y is inverted to get to texture space; do it here, not in shader.
					uv[0] = vec2(0,y0);
					uv[1] = vec2(1,y0);
					uv[2] = vec2(0,y1);
					uv[3] = vec2(0,y1);
					uv[4] = vec2(1,y0);
					uv[5] = vec2(1,y1);

					gl::drawSolidTriangle(v,uv);
					gl::drawSolidTriangle(v+3,uv+3);
					// TODO: be less dumb and draw as tri strip or quad
				}
				else
				{
					gl::color(mInstrument->mScoreColor);
					gl::draw( getPolyLine() );
				}
			}
			break;
			
			case Instrument::SynthType::Meta:
			{
				// meta
				drawMetaParam(drawType);
			}
			break;
			
			case Instrument::SynthType::MIDI:
			case Instrument::SynthType::RobitPokie:
			{
				// "Note-type" synth (midi or robit)
				if ( drawType != GameWorld::DrawType::UIPipelineThumb ) // optimization
				{
					drawScoreLines(drawType);
					drawNotes(drawType);
				}
				else
				{
					gl::color(mInstrument->mScoreColor);
					gl::draw( getPolyLine() );
				}

				// playhead
				drawPlayhead(drawType);
			}
			break;
		} // switch
	} // if

	// quad debug
	if (0)
	{
		const float kR = 1.f;
		gl::color( 1,1,1 );
		gl::drawSolidCircle(mQuad[0], kR);
		gl::color( 1,0,0 );
		gl::drawSolidCircle(mQuad[1], kR);
		gl::color( 0,1,0 );
		gl::drawSolidCircle(mQuad[2], kR);
		gl::color( 0,0,1 );
		gl::drawSolidCircle(mQuad[3], kR);
	}
	
	// instrument icon
	drawInstrumentIcon( getInstrumentIconPoseFromScore(getPlayheadFrac()) );
}

bool Score::setQuadFromPolyLine( PolyLine2 poly, vec2 timeVec )
{
	// could have simplified a bit and just examined two bisecting lines. oh well. it works.
	// also calling this object 'Score' and the internal scores 'score' is a little confusing.
	if ( poly.size()==4 )
	{
		auto in = [&]( int i ) -> vec2
		{
			return poly.getPoints()[i%4];
		};

		auto scoreEdge = [&]( int from, int to )
		{
			return dot( timeVec, normalize( in(to) - in(from) ) );
		};

		auto scoreSide = [&]( int side )
		{
			/* input pts:
			   0--1
			   |  |
			   3--2

			   side 0 : score of 0-->1, 3-->2
			*/

			return ( scoreEdge(side+0,side+1) + scoreEdge(side+3,side+2) ) / 2.f;
		};

		int   bestSide =0;
		float bestScore=0;

		for( int i=0; i<4; ++i )
		{
			float score = scoreSide(i);

			if ( score > bestScore )
			{
				bestScore = score ;
				bestSide  = i;
			}
		}

		// copy to mQuad
		mQuad[0] = in( bestSide+3 );
		mQuad[1] = in( bestSide+2 );
		mQuad[2] = in( bestSide+1 );
		mQuad[3] = in( bestSide+0 );
		// wtf i don't get the logic but it works

		return true ;
	}
	else return false;
}

PolyLine2 Score::getPolyLine() const
{
	PolyLine2 p;
	p.setClosed();

	for( int i=0; i<4; ++i ) p.getPoints().push_back( mQuad[i] );

	return p;
}

void Score::tick(float globalPhase, float beatDuration)
{
	mPhase = fmod(globalPhase, mDurationFrac) / mDurationFrac;

	if (!mInstrument) return;

	switch( mInstrument->mSynthType )
	{
		// Additive
		case Instrument::SynthType::Additive:
		{
			// Update time
			mInstrument->mPureDataNode->sendFloat(toString(mInstrument->mAdditiveSynthID)+string("phase"),
												  getPlayheadFrac() );
		}
		break;

		// Notes
		case Instrument::SynthType::MIDI:
		case Instrument::SynthType::RobitPokie:
		{
			// send midi notes
			if (!mQuantizedImage.empty())
			{
				int x = getPlayheadFrac() * (float)(mQuantizedImage.cols);

				for ( int y=0; y<mNoteCount; ++y )
				{
					unsigned char value = mQuantizedImage.at<unsigned char>(mNoteCount-1-y,x);

					int note = noteForY(y);

					if ( isScoreValueHigh(value) )
					{
						float duration =
						beatDuration *
						mDurationFrac *
						getNoteLengthAsScoreFrac(mQuantizedImage,x,y);

						if (duration>0)
						{
							mInstrument->doNoteOn( note, duration );
						}
					}
					// See if the note was previously triggered but no longer exists, and turn it off if so
					else if (mInstrument->isNoteInFlight( note ))
					{
						// TODO: this should work as long a there isn't >1 score per instrument. In that case,
						// this will start to behave weirdly. Proper solution is to scan all scores and
						// aggregate all the on notes, and then join the list of desired on notes to actual on notes
						// in a single pass, taking action to on/off them as needed.
						mInstrument->doNoteOff( note );
					}
				}
			}

			mInstrument->tickArpeggiator();
		}
		break;

		default:
		break;
	}

	// retire notes
	mInstrument->updateNoteOffs();
}

void Score::updateAdditiveSynthesis() {
	InstrumentRef instr = mInstrument;
	if (!instr) return;

	// send image for additive synthesis
	if ( instr->mSynthType==Instrument::SynthType::Additive && !mImage.empty() )
	{
		PureDataNodeRef pd = instr->mPureDataNode;
		int additiveSynthID = mInstrument->mAdditiveSynthID;

		int rows = mImage.rows;
		int cols = mImage.cols;

		// Update pan
		pd->sendFloat(toString(additiveSynthID)+string("pan"),
							     mPan);

		// Update per-score pitch
		pd->sendFloat(toString(additiveSynthID)+string("note-root"),
								 20 );

		// Update range of notes covered by additive synthesis
		pd->sendFloat(toString(additiveSynthID)+string("note-range"),
								 100 );

		// Update resolution
		pd->sendFloat(toString(additiveSynthID)+string("resolution-x"),
								 cols);

		pd->sendFloat(toString(additiveSynthID)+string("resolution-y"),
								 rows);

		// Create a float version of the image
		cv::Mat imageFloatMat;

		// Copy the uchar version scaled 0-1
		mImage.convertTo(imageFloatMat, CV_32FC1, 1/255.0);

		// Convert to a vector to pass to Pd

		// Grab the current column at the playhead. We send updates even if the
		// phase doesn't change enough to change the colIndex,
		// as the pixels might have changed instead.
		float phase = getPlayheadFrac();
		int colIndex = imageFloatMat.cols * phase;

		cv::Mat columnMat = imageFloatMat.col(colIndex);

		// We use a list message rather than writeArray as it causes less contention with Pd's execution thread
		auto ampsByFreq = pd::List();
		for (int i = 0; i < columnMat.rows; i++) {
			ampsByFreq.addFloat(columnMat.at<float>(i, 0));
		}

		pd->sendList(toString(additiveSynthID)+string("vals"), ampsByFreq);

		// Turn the synth on
		pd->sendFloat(toString(additiveSynthID)+string("volume"), 1);
	}
}

float Score::getPlayheadFrac() const
{
	return mPhase;
}

void Score::getPlayheadLine( vec2 line[2] ) const
{
	float t = getPlayheadFrac();

	line[0] = lerp(mQuad[3],mQuad[2],t); // bottom
	line[1] = lerp(mQuad[0],mQuad[1],t); // top
}

vec2 Score::getPlayheadVec() const
{
	return normalize( fracToQuad(vec2(1.f,.5f)) - fracToQuad(vec2(0.f,.5f)) );
}

vec2 Score::getSizeInWorldSpace() const
{
	vec2 size;

	size.x = distance( lerp(mQuad[0],mQuad[3],.5f), lerp(mQuad[1],mQuad[2],.5f) );
	size.y = distance( lerp(mQuad[0],mQuad[1],.5f), lerp(mQuad[3],mQuad[2],.5f) );

	return size;
}

vec2 Score::fracToQuad( vec2 frac ) const
{
	vec2 top = lerp(mQuad[0],mQuad[1],frac.x);
	vec2 bot = lerp(mQuad[3],mQuad[2],frac.x);

	return lerp(bot,top,frac.y);
}

int Score::noteForY( int y ) const {

	if (mInstrument && mInstrument->mMapNotesToChannels) {
		return y;
	}

	int numNotes = mScale.size();
	int extraOctaveShift = y / numNotes * 12;
	int degree = y % numNotes;
	int note = mScale[degree];

	return note + extraOctaveShift + mRootNote + mOctave*12;
}

float Score::getQuadMaxInteriorAngle() const
{
	float mang=0.f;

	for( int i=0; i<4; ++i )
	{
		vec2 a = mQuad[i];
		vec2 x = mQuad[(i+1)%4];
		vec2 b = mQuad[(i+2)%4];

		a -= x;
		b -= x;

		float ang = acos( dot(a,b) / (length(a)*length(b)) );

		if (ang>mang) mang=ang;

	}

	return mang;
}

