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

const ScoreNote*
ScoreNotes::isNoteOn( float playheadFrac, int note ) const
{
	if ( note < 0 || note >= size() ) return 0;
	
	for( const auto &n : (*this)[note] )
	{
		if ( playheadFrac >= n.mStartTimeAsScoreFrac )
		{
			auto end = n.mStartTimeAsScoreFrac + n.mLengthAsScoreFrac;
			
			if ( playheadFrac < end ) {
				return &n;
			}
		}
		else return 0;
	}
	
	return 0;
}

const ScoreNote*
ScoreNotes::isNoteOn( int playheadCol, int note ) const
{
	if ( note < 0 || note >= size() ) return 0;
	
	for( const auto &n : (*this)[note] )
	{
		if ( playheadCol >= n.mStartTimeAsCol )
		{
			auto end = n.mStartTimeAsCol + n.mLengthAsCols;
			
			if ( playheadCol < end ) {
				return &n;
			}
		}
		else return 0;
	}
	
	return 0;
}

int Score::drawNotes( InstrumentRef instrument, GameWorld::DrawType drawType ) const
{
	const float kNoteFadeOutTimeFrac = .2f;
	const float kInflateOnHitFrac = .25f; // cm (world units)
	
	const vec2 xvec = getPlayheadVec();
	const vec2 yvec = perp(xvec);
	
	int numOnNotes=0;
	
	const float yheight = 1.f / (float)mNoteCount;
	const float playheadFrac = getPlayheadFrac();

	TriMesh mesh( TriMesh::Format().positions(2).colors(4) );	
	
	for ( int note=0; note<mNoteCount; ++note )
	{
		const float y1frac = note * yheight + yheight*.2f;
		const float y2frac = note * yheight + yheight*.8f;
		
		for ( auto anote : mNotes[note] )
		{
			// how wide?
			const float x1frac = anote.mStartTimeAsScoreFrac;
			const float x2frac = anote.mStartTimeAsScoreFrac + anote.mLengthAsScoreFrac;

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
			ColorA color = lerp( instrument->mNoteOffColor, instrument->mNoteOnColor, strikeColor );
			
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
		}
	}

	// draw batched notes
	gl::draw(mesh);
	
	return numOnNotes;
}

void Score::drawScoreLines( InstrumentRef instrument, GameWorld::DrawType drawType ) const
{
	vector<vec2> pts;

	gl::color(instrument->mScoreColor);
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
	gl::color( instrument->mScoreColorDownLines  );
	drawLines(pts);

}

void Score::drawPlayhead( InstrumentRef instrument, GameWorld::DrawType drawType ) const
{
	if (instrument) gl::color( instrument->mPlayheadColor );
	else gl::color(0,1,1); // TODO: make into xml value
	
	vec2 playhead[2];
	getPlayheadLine(playhead);
	gl::drawLine( playhead[0], playhead[1] );

	// octave indicator
	float octaveFrac = (float)mOctave / (float)mNumOctaves + .5f ;

	gl::drawSolidCircle( lerp(playhead[0],playhead[1],octaveFrac), .5f, 6 );
}

void Score::drawMetaParam( InstrumentRef instrument, GameWorld::DrawType drawType ) const
{
	const float sliderValue = getMetaParamSliderValue(instrument);
	
	gl::color(instrument->mScoreColor);
	gl::draw( getPolyLine() );

	// level
	float y1, y2;
	if ( instrument->mMetaParamInfo.isDiscrete() )
	{
		if ( drawType != GameWorld::DrawType::UIPipelineThumb ) // optimization
		{
			vector<vec2> lines;

			for( int i=0; i<instrument->mMetaParamInfo.mNumDiscreteStates; ++i )
			{
				float y = (float)i/(float)(instrument->mMetaParamInfo.mNumDiscreteStates);
				lines.push_back( fracToQuad(vec2(0.f,y)) );
				lines.push_back( fracToQuad(vec2(1.f,y)) );
			}

			drawLines(lines);
		}

//		y1=instrument->mMetaParamInfo.discretize(mMetaParamSliderValue);
//		y2=y1 + 1.f / (float)instrument->mMetaParamInfo.mNumDiscreteStates;

		y1=instrument->mMetaParamInfo.discretize(sliderValue);
		y2=y1 + 1.f / (float)instrument->mMetaParamInfo.mNumDiscreteStates;
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
		y2 = max( k, sliderValue );
	}

	if ( sliderValue != -1.f )
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

void Score::drawAdditive( InstrumentRef instrument, GameWorld::DrawType drawType ) const
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
		gl::color(instrument->mScoreColor);
		gl::draw( getPolyLine() );
	}	
}

tIconAnimState Score::getIconPoseFromScore_Percussive( InstrumentRef instrument, float playheadFrac ) const
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
		if ( mNotes.isNoteOn(playheadFrac,note) )
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
	
	if (instrument) {
		state.mColor = (numOnNotes>0) ? instrument->mNoteOnColor : instrument->mNoteOffColor;
	} else {
		state.mColor = ColorA(1,1,1,1);
	}

	float fracOnNote = (float)numOnNotes/(float)mNoteCount;
	
	state.mGradientSpeed = lerp( kMinGradientSpeed, kMaxGradientSpeed, fracOnNote );
	state.mGradientFreq  = lerp( kMinGradientFreq, kMaxGradientFreq, fracOnNote );
	if (instrument) state.mGradientCenter = instrument->mIconGradientCenter;
	
	return state;
}

tIconAnimState Score::getIconPoseFromScore_Melodic( InstrumentRef instrument, float playheadFrac ) const
{
	// gather
	int numOnNotes=0;
	
	int noteIndexSum=0;
	
	for( int note=0; note<mNoteCount; note++ )
	{
		if ( mNotes.isNoteOn(playheadFrac,note) )
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
	
	if (instrument) {
		state.mColor = (numOnNotes>0) ? instrument->mNoteOnColor : instrument->mNoteOffColor;
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
	if (instrument) state.mGradientCenter = instrument->mIconGradientCenter;
	
	return state;
}

tIconAnimState Score::getIconPoseFromScore_Additive( InstrumentRef instrument, float playheadFrac ) const
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
		pose.mColor = sum>0.f ? instrument->mNoteOnColor : instrument->mNoteOffColor;
		pose.mScale = vec2(1,1) * lerp( 1.f, 1.4f, scaleDegree*scaleDegree );
		pose.mTranslate.y = lerp( .5f, -.5f, weightedy ) ; // low values means up
		pose.mGradientSpeed = lerp( kMinGradientSpeed, kMaxGradientSpeed, powf(fracOn,.5) );
		pose.mGradientFreq = lerp( kMinGradientFreq, kMaxGradientFreq, powf(fracOn,.5) );
		if (instrument) pose.mGradientCenter = instrument->mIconGradientCenter;
		return pose;
	}
	else return tIconAnimState();
}

tIconAnimState Score::getIconPoseFromScore_Meta( InstrumentRef instrument, float playheadFrac ) const
{
	const float sliderValue = getMetaParamSliderValue(instrument);
	
	tIconAnimState pose;
	pose.mGradientSpeed = lerp(
		kMinGradientSpeed,
		kMaxGradientSpeed,
		powf( constrain(sliderValue,0.f,1.f), 3.f) );
		
	pose.mGradientFreq = lerp( kMinGradientFreq, kMaxGradientFreq, sliderValue );
	if (instrument) pose.mGradientCenter = instrument->mIconGradientCenter;
	return pose;
}

tIconAnimState Score::getIconPoseFromScore( InstrumentRef instrument, float playheadFrac ) const
{
	// no instrument?
	if ( !instrument ) return tIconAnimState();

	// synth type
	switch( instrument->mSynthType )
	{
		case Instrument::SynthType::MIDI:
		case Instrument::SynthType::Sampler:
			return getIconPoseFromScore_Melodic(instrument,playheadFrac);
			break;
		
		case Instrument::SynthType::RobitPokie:
			return getIconPoseFromScore_Percussive(instrument,playheadFrac);
			break;
		
		case Instrument::SynthType::Additive:
			return getIconPoseFromScore_Additive(instrument,playheadFrac);
			break;
		
		case Instrument::SynthType::Meta:
			return getIconPoseFromScore_Meta(instrument,playheadFrac);
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

	// FIXME: Looping isn't quite right, but it's a start to this refactoring.
	for( auto instrument : mInstruments )
	{
		switch( instrument->mSynthType )
		{
			case Instrument::SynthType::Additive:
			{
				drawAdditive(instrument,drawType);
			}
			break;
			
			case Instrument::SynthType::Meta:
			{
				// meta
				drawMetaParam(instrument,drawType);
			}
			break;
			case Instrument::SynthType::Sampler:
			case Instrument::SynthType::MIDI:
			case Instrument::SynthType::RobitPokie:
			{
				// "Note-type" synth (midi or robit)
				if ( drawType != GameWorld::DrawType::UIPipelineThumb ) // optimization
				{
					drawScoreLines(instrument,drawType);
					drawNotes(instrument,drawType);
				}
				else
				{
					gl::color(instrument->mScoreColor);
					gl::draw( getPolyLine() );
				}

				// playhead
				drawPlayhead(instrument,drawType);
			}
			break;
		} // switch
	} // if
	
	// i would like to do this, but spurious token scores make it broken :P
//	if ( mInstruments.empty() ) drawPlayhead(0,drawType);
	
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
	mBeatDuration = beatDuration;
	mPosition = fmod(globalPhase, (float)getBeatCount() );

	/*
	for( auto instrument : mInstruments )
	{
		switch( instrument->mSynthType )
		{
			// Notes
			case Instrument::SynthType::Sampler:
			case Instrument::SynthType::MIDI:
			case Instrument::SynthType::RobitPokie:
			{
				// send midi notes
				if (!mNotes.empty())
				{
					int x = getPlayheadFrac() * (float)mNotes.mNumCols;

					for ( int y=0; y<mNotes.size(); ++y )
					{
						int note = noteForY(instrument,y);
						
						const ScoreNote* isOn = mNotes.isNoteOn(x,y);
						
						if (isOn)
						{
							float duration =
							beatDuration *
							(float)getQuantizedBeatCount() *
							isOn->mLengthAsScoreFrac;
							// Note: that if we trigger late, we will go on for too long...

							if (duration>0)
							{
								instrument->doNoteOn( note, duration );
							}
						}
						else
						{
							// See if the note was previously triggered but no longer exists, and turn it off if so
							if (instrument->isNoteInFlight( note ))
							{
								// TODO: this should work as long a there isn't >1 score per instrument. In that case,
								// this will start to behave weirdly. Proper solution is to scan all scores and
								// aggregate all the on notes, and then join the list of desired on notes to actual on notes
								// in a single pass, taking action to on/off them as needed.
								instrument->doNoteOff( note );
							}
						}
					}
				}

				instrument->tickArpeggiator();
			}
			break;

			default:
			break;
		}
	}*/
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

float Score::getMetaParamSliderValue( InstrumentRef i ) const
{
	auto valueit = mMetaParamSliderValue.find(i->mMetaParam);

	if (valueit==mMetaParamSliderValue.end()) return i->mMetaParamInfo.mDefaultValue;
	else return valueit->second;
}

int Score::noteForY( const Instrument* instrument, int y ) const
{	
	bool isPokie = instrument->mSynthType == Instrument::SynthType::RobitPokie;
	if (instrument && instrument->mMapNotesToChannels && !isPokie) {
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

Score* ScoreVec::getScoreForInstrument( InstrumentRef instr )
{
	for( auto &s : *this )
	{
		if ( s.mInstruments.hasInstrument(instr) ) return &s;
	}
	return 0;
}

ScoreVec ScoreVec::getFiltered( std::function<bool(const Score&)> test ) const
{
	ScoreVec out;
	
	for( auto &s : *this )
	{
		if ( test(s) ) out.push_back(s);
	}
	
	return out;
}