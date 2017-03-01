//
//  Score.h
//  PaperBounce3
//
//  Created by Luke Iannini on 11/14/16.
//
//

#ifndef Score_h
#define Score_h

#include "Instrument.h"
#include "MusicStamp.h"

// notes
struct ScoreNote
{	
	int   mStartTimeAsCol; // 0, 1, 2..
	int   mLengthAsCols; // 1, 2, ...

	// 0..1
	float mStartTimeAsScoreFrac;
	float mLengthAsScoreFrac;
};

class ScoreNotes : public vector<vector<ScoreNote>>
{
public:
	// for storing parsed notes.
	// - first index is note
	// - then, a list of notes, in order of start time

	int   mNumCols=0;

	const ScoreNote* isNoteOn( float playheadFrac, int note ) const;
	const ScoreNote* isNoteOn( int    playheadCol, int note ) const;		
};

// scores
class Score
{
public:
	InstrumentRef mInstrument;
	gl::GlslProgRef mAdditiveShader;

	void draw( GameWorld::DrawType ) const;

	// shape
	vec2  mQuad[4];
	/*  Vertices are played back like so:

	 0---1
	 |   |
	 3---2

	 --> time		*/

	// detection meta-data
	bool mIsZombie=false;
	bool mDoesZombieTouchOtherZombies=false;

	// image data
	cv::UMat	mImage;				// thresholded image
	cv::UMat	mQuantizedImagePreThreshold; // (for inter-frame smoothing)
	cv::Mat		mQuantizedImage;	// quantized image data for midi playback
	float		mMetaParamSliderValue=-1.f; // 0..1
	gl::TextureRef mTexture; // used for additive, so we don't do it per frame
	ScoreNotes	mNotes;
	
	// synth parameters
	float		mPosition=0; // progress from 0-mDuration
	void        tick(float globalPhase, float beatDuration);

	int			mOctave=-1; // controlled by mOctaveFrac
	float		mOctaveFrac=0.5f;
	float		mRootNote=0.f;
	int			mNoteCount=-1;
	int			mNoteInstrument=-1;
	int         mNumOctaves=-1;

	float       mMeasureCount=-1;
	int			mBeatsPerMeasure=-1;
	int			mBeatQuantization=-1;
	
	int			getQuantizedBeatCount() const { return mMeasureCount * mBeatsPerMeasure * mBeatQuantization; }
	int			getBeatCount()			const { return mMeasureCount * mBeatsPerMeasure; }
	
	float		mPan;
	//float		mVolume;

	// constructing shape
	bool		setQuadFromPolyLine( PolyLine2, vec2 timeVec );

	// getting stuff
	PolyLine2	getPolyLine() const;
	float		getPlayheadFrac() const;
	void		getPlayheadLine( vec2 line[2] ) const; // line goes from [3..2] >> [0..1]
	vec2		getPlayheadVec() const; // effectively approximates MusicWorld's mTimeVec
	vec2		getSizeInWorldSpace() const;
	// if not a perfect rect, then it's approximate (measures two bisecting axes)
	// x is axis along which playhead moves; y is perp to it (parallel to playhead)
	vec2		fracToQuad( vec2 frac ) const; // frac.x = time[0,1], frac.y = note_space[0,1]
	vec2		getCentroid() const { return fracToQuad(vec2(.5,.5)); }
//	float		getQuadMaxInteriorAngle() const; // looking for concave-ish shapes...

	Scale mScale;
	int noteForY( int y ) const;

	// additive synth
	void updateAdditiveSynthesis();

	// icon animation
	tIconAnimState getIconPoseFromScore( float playheadFrac ) const;

private:
	tIconAnimState getIconPoseFromScore_Melodic( float playheadFrac ) const;
	tIconAnimState getIconPoseFromScore_Percussive( float playheadFrac ) const;
	tIconAnimState getIconPoseFromScore_Additive( float playheadFrac ) const;
	tIconAnimState getIconPoseFromScore_Meta( float playheadFrac ) const;

	int  drawNotes		( GameWorld::DrawType drawType ) const; // returns # on notes
	void drawScoreLines	( GameWorld::DrawType drawType ) const;
	void drawPlayhead	( GameWorld::DrawType drawType ) const;
	void drawMetaParam	( GameWorld::DrawType drawType ) const;
};

class ScoreVec : public vector<Score>
{
public:
	const Score* pick( vec2 ) const;
	Score* pick( vec2 );
	Score* getScoreForInstrument( InstrumentRef );
};

#endif /* Score_h */
