#include "MusicWorld.h"
#include "cinder/gl/Context.h"
#include "CinderOpenCV.h"
#include "geom.h"

using namespace ci::gl;

const float kMinGradientFreq=2.f;
const float kMaxGradientFreq=15.f;
const float kMinGradientSpeed=0.1f;
const float kMaxGradientSpeed=.5f;

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
	const float kNoteFadeOutTimeFrac = .2f;
	const float kInflateOnHitFrac = .25f; // cm (world units)
	
	const vec2 xvec = getPlayheadVec();
	const vec2 yvec = perp(xvec);
	
	int numOnNotes=0;
	
	// collect notes into a batch
	// (probably wise to extract this geometry/data once when processing vision data,
	// then use it for both playback and drawing).
	const float invcols = 1.f / (float)(mQuantizedImage.cols);
	const float yheight = 1.f / (float)mNoteCount;
	const float playheadFrac = getPlayheadFrac();

	TriMesh mesh( TriMesh::Format().positions(2).colors(4) );	

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
				

				/*	0--1 .. 2
				    |  |
					3--2 .. 1
					.  .
					s  e
				*/
				float strikeColor;
				{
					float t = playheadFrac;
					if (t<x1frac) t += 1.f; // make sure t is always >= x1frac (wrap it)
					strikeColor = 1.f - constrain( (t - x2frac)/kNoteFadeOutTimeFrac, 0.f, 1.f );
				}
				
				vec2 v[4] = {start2,end2,end1,start1};
				ColorA color = lerp( mInstrument->mNoteOffColor, mInstrument->mNoteOnColor, strikeColor );
				
				// stretch it out
				if (isInFlight) // this conditional is redundant to inflate==0.f
				{
					float inflate;
					
					inflate = 1.f - (playheadFrac - x1frac) / (x2frac - x1frac);
					inflate *= inflate;
					inflate *= kInflateOnHitFrac;

					vec2 xd = xvec * inflate;
					vec2 yd = -yvec * inflate; // don't get why this needs to be negative, but it does. :P
					
					v[0] += -xd +yd;
					v[1] +=  xd +yd;
					v[2] +=  xd -yd;
					v[3] += -xd -yd;
				}

				appendQuad(mesh, color, v);
				
				// skip ahead
				x = x + length+1;
			}
		}
	}

	// draw batched notes
	gl::draw(mesh);
	
	return numOnNotes;
}

void Score::drawScoreLines( GameWorld::DrawType drawType ) const
{
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
	for( int i=0; i<getQuantizedBeatCount(); ++i )
	{
		if (i % mBeatQuantization == 0) continue;

		float f = (float)i / (float)getQuantizedBeatCount();

		pts.push_back( lerp(mQuad[1], mQuad[0],f) );
		pts.push_back( lerp(mQuad[2], mQuad[3],f) );
	}

	drawLines(pts);

	// New points for new colors
	pts.clear();
	// Down-beat lines
	for( int i=0; i<getQuantizedBeatCount(); ++i )
	{
		if (i % mBeatQuantization != 0) continue;

		float f = (float)i / (float)getQuantizedBeatCount();

		pts.push_back( lerp(mQuad[1], mQuad[0],f) );
		pts.push_back( lerp(mQuad[2], mQuad[3],f) );
	}
	gl::color( mInstrument->mScoreColorDownLines  );
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

tIconAnimState Score::getIconPoseFromScore_Percussive( float playheadFrac ) const
{
	const vec4 poses[13] =
	{
		vec4(  .25, 0.f, 1.f, -30 ),
		vec4( -.25, 0.f, 1.f,  30 ),
		vec4( .3, 0, 1, 20 ),
		vec4( 0, .4, 1, 0 ),
		vec4( -.3, 0, 1, -20 ),
		vec4(  .25, 0.f, 1.f, -30 ),
		vec4( -.25, 0.f, 1.f,  30 ),
		vec4( 0, 0, 1.1, 20 ),
		vec4( 0, 0, 1.1, -20 ),
		vec4( 0, .2, 1.2, 0 ),
		vec4( .2, 0, 1.2, 0 ),
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
			pose += poses[ (note*3)%13 ]; // separate similar adjacent anim frames (in note space)
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
	tIconAnimState state = pose;
	
	if (mInstrument) {
		state.mColor = (numOnNotes>0) ? mInstrument->mNoteOnColor : mInstrument->mNoteOffColor;
	} else {
		state.mColor = ColorA(1,1,1,1);
	}

	float fracOnNote = (float)numOnNotes/(float)mNoteCount;
	
	state.mGradientSpeed = lerp( kMinGradientSpeed, kMaxGradientSpeed, fracOnNote );
	state.mGradientFreq  = lerp( kMinGradientFreq, kMaxGradientFreq, fracOnNote );
	if (mInstrument) state.mGradientCenter = mInstrument->mIconGradientCenter;
	
	return state;
}

tIconAnimState Score::getIconPoseFromScore_Melodic( float playheadFrac ) const
{
	// gather
	int numOnNotes=0;
	
	int noteIndexSum=0;
	
	for( int note=0; note<mNoteCount; note++ )
	{
		if ( isNoteOn(playheadFrac,note) )
		{
			noteIndexSum += note;
			numOnNotes++;
		}
	}
	
	// blend
	float avgNote = .5f; // since we translate with this, use the median for none.
	
	if (numOnNotes > 0) {
		avgNote = (float)(noteIndexSum/(float)mNoteCount) / (float)numOnNotes;
	}
	
	// pose
	tIconAnimState state;
	
	if (mInstrument) {
		state.mColor = (numOnNotes>0) ? mInstrument->mNoteOnColor : mInstrument->mNoteOffColor;
	} else {
		state.mColor = ColorA(1,1,1,1);
	}
	
	// new anim inspired by additive
	state.mTranslate.y = lerp( -.5f, .5f, avgNote );
	state.mScale = vec2(1,1) * lerp( 1.f, 1.5f, min( 1.f, ((float)numOnNotes / (float)mNoteCount)*2.f ) ) ;
	
//	state.mGradientCenter = vec2( fmod( avgNote * 3.27, 1.f ) , avgNote );
	
	float onNoteFrac = (float)numOnNotes/(float)mNoteCount;
	state.mGradientSpeed = lerp( kMinGradientSpeed, kMaxGradientSpeed, powf( onNoteFrac, 3 ) );
	state.mGradientFreq = lerp( kMinGradientFreq, kMaxGradientFreq, onNoteFrac );
	if (mInstrument) state.mGradientCenter = mInstrument->mIconGradientCenter;
	
	return state;
}

tIconAnimState Score::getIconPoseFromScore_Additive( float playheadFrac ) const
{
	if (!mImage.empty())
	{
		// Create a float version of the image
		cv::Mat imageFloatMat;

		// Copy the uchar version scaled 0-1
		mImage.convertTo(imageFloatMat, CV_32FC1, 1/255.0);
		
		int colIndex = imageFloatMat.cols * getPlayheadFrac();

		cv::Mat columnMat = imageFloatMat.col(colIndex);

		// We use a list message rather than writeArray as it causes less contention with Pd's execution thread
		float sum=0;
		float weightedy=0;
		
		for (int i = 0; i < columnMat.rows; i++)
		{
			float y = (float)i/(float)(columnMat.rows-1); // 0..1
			float v = 1.f - columnMat.at<float>(i,0);
			
			weightedy += y*v;
			sum += v;
		}

		if (sum>0) weightedy /= sum ; // normalize
		else weightedy = .5f; // move to center if empty, since that will mean no vertical translation

		float fracOn = (sum/(float)columnMat.rows);
		float scaleDegree = fracOn / .5f ; // what % of keys on count as 100% scale?
		
		tIconAnimState pose;
		pose.mColor = sum>0.f ? mInstrument->mNoteOnColor : mInstrument->mNoteOffColor;
		pose.mScale = vec2(1,1) * lerp( 1.f, 1.4f, scaleDegree*scaleDegree );
		pose.mTranslate.y = lerp( .5f, -.5f, weightedy ) ; // low values means up
		pose.mGradientSpeed = lerp( kMinGradientSpeed, kMaxGradientSpeed, powf(fracOn,.5) );
		pose.mGradientFreq = lerp( kMinGradientFreq, kMaxGradientFreq, powf(fracOn,.5) );
		if (mInstrument) pose.mGradientCenter = mInstrument->mIconGradientCenter;
		return pose;
	}
	else return tIconAnimState();
}

tIconAnimState Score::getIconPoseFromScore_Meta( float playheadFrac ) const
{
	tIconAnimState pose;
	pose.mGradientSpeed = lerp( kMinGradientSpeed, kMaxGradientSpeed, powf(constrain(mMetaParamSliderValue,0.f,1.f),3.f) );
	pose.mGradientFreq = lerp( kMinGradientFreq, kMaxGradientFreq, mMetaParamSliderValue );
	if (mInstrument) pose.mGradientCenter = mInstrument->mIconGradientCenter;
	return pose;
}

tIconAnimState Score::getIconPoseFromScore( float playheadFrac ) const
{
	// no instrument?
	if ( !mInstrument ) return tIconAnimState();

	// synth type
	switch( mInstrument->mSynthType )
	{
		case Instrument::SynthType::MIDI:
		{
			return getIconPoseFromScore_Melodic(playheadFrac);
		}
		break;
		
		case Instrument::SynthType::RobitPokie:
		{
			return getIconPoseFromScore_Percussive(playheadFrac);
		}
		break;
		
		case Instrument::SynthType::Additive:
		{
			return getIconPoseFromScore_Additive(playheadFrac);
		}
		break;
		
		case Instrument::SynthType::Meta:
		{
			return getIconPoseFromScore_Meta(playheadFrac);
		}
		break;
		
		default: break;
	} // switch
	
	// nada
	return tIconAnimState();
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
				if ( mAdditiveShader && mTexture )
				{
					gl::ScopedGlslProg glslScp( mAdditiveShader );
					gl::ScopedTextureBind texScp( mTexture );

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
	//drawInstrumentIcon( getIconPoseFromScore(getPlayheadFrac()) );
}

bool Score::setQuadFromPolyLine( PolyLine2 poly, vec2 timeVec )
{
	return getOrientedQuadFromPolyLine(poly, timeVec, mQuad);
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
	mPosition = fmod(globalPhase, (float)getBeatCount() );

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
						(float)getQuantizedBeatCount() *
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
	return mPosition / (float)getBeatCount();
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

	
	bool isPokie = mInstrument->mSynthType == Instrument::SynthType::RobitPokie;
	if (mInstrument && mInstrument->mMapNotesToChannels && !isPokie) {
		// Reinterpret octave shift as note shift when using NotesToChannelsMode (and don't go negative)
		int noteShift = mOctave + mNumOctaves/2;
		return y + noteShift;
	}

	int numNotes = mScale.size();
	int extraOctaveShift = y / numNotes * 12;
	int degree = y % numNotes;
	int note = mScale[degree];

	return note + extraOctaveShift + mRootNote + mOctave*12;
}
/*
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
}*/

const Score* ScoreVec::pick( vec2 p ) const
{
	for( auto &s : *this )
	{
		if (s.getPolyLine().contains(p)) return &s;
	}
	return 0;
}

Score* ScoreVec::pick( vec2 p )
{
	for( auto &s : *this )
	{
		if (s.getPolyLine().contains(p)) return &s;
	}
	return 0;
}
