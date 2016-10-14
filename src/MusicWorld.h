//
//  MusicWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#ifndef MusicWorld_hpp
#define MusicWorld_hpp

#include "GameWorld.h"
#include "PureDataNode.h"

class MusicWorld : public GameWorld
{
public:

	MusicWorld();
	~MusicWorld();

	void setParams( XmlTree ) override;
	
	string getSystemName() const override { return "MusicWorld"; }

	void update() override;
	void updateContours( const ContourVector &c ) override;
	void updateCustomVision( Pipeline& ) override; // extract bitmaps we need

	void draw( bool highQuality ) override;

private:
	
	// params
	float mStartTime;	// when MusicWorld created
	vec2  mTimeVec;		// in world space, which way does time flow forward?
	float mPhase;		// how fast to playback?
	
	// scores
	class Score
	{
	public:
		// time params -- same as MusicWorld by default
		float mStartTime;
		float mPhase;

		// shape
		vec2  mQuad[4];
		/*  Vertices are played back like so:
		
			0---1
			|   |
			3---2
			
			 --> time		*/

		// bitmap
		cv::Mat		mImage;
		
		// constructing shape
		bool		setQuadFromPolyLine( PolyLine2, vec2 timeVec );
		
		// getting stuff
		PolyLine2	getPolyLine() const;
		float		getPlayheadFrac() const;
		void		getPlayheadLine( vec2 line[2] ) const;
	};
	vector<Score> mScore;
	
	// synthesis
	cipd::PureDataNodeRef	mPureDataNode;	// synth engine
	cipd::PatchRef			mPatch;			// pong patch
	
	void setupSynthesis();
};

class MusicWorldCartridge : public GameCartridge
{
public:
	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<MusicWorld>();
	}
};

#endif /* MusicWorld_hpp */
