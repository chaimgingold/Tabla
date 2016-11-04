//
//  LightLink.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/5/16.
//
//

#include "LightLink.h"
#include "xml.h"

void LightLink::setParams( XmlTree xml )
{
	getXml(xml,"CameraIndex",mCameraIndex);
	getXml(xml,"CaptureSize",mCaptureSize);
	
	getXml(xml, "CaptureCoords", mCaptureCoords, 4 );
	getXml(xml, "CaptureWorldSpaceCoords", mCaptureWorldSpaceCoords, 4 );

	getXml(xml, "ProjectorCoords", mProjectorCoords, 4 );
	getXml(xml, "ProjectorWorldSpaceCoords", mProjectorWorldSpaceCoords, 4 );
	
	getXml(xml, "ProjectorSize", mProjectorSize);

	if ( xml.hasChild("DistCoeffs") )
	{
		// clobber [], chars
		string value = xml.getChild("DistCoeffs").getValue();
		
		for( int i=0; i<value.size(); ++i )
		{
			if (value[i]==','||value[i]=='['||value[i]==']')
			{
				value[i]=' ';
			}
		}		
//		cout << "reading DistCoeffs: " << value << endl;
		
		// break into float vector
		vector<float> f;
		
		std::istringstream ss(value);
		while ( !ss.fail() )
		{
			float x;
			ss >> x;
			if (!ss.fail())
			{
//				cout << "\t" << x << endl;
				f.push_back(x);
			}
		}
		
		// convert float vector to matrix
		mDistCoeffs = cv::Mat(1,f.size(),CV_32F);
		for( int i=0; i<f.size(); ++i ) mDistCoeffs.at<float>(i) = f[i];
//		cout << "\t" << mDistCoeffs << endl;
	}
}

XmlTree LightLink::getParams() const
{
	XmlTree t("LightLink","");
	
	auto v = []( vec2 v )
	{
		return toString(v.x) + " " + toString(v.y);
	};
	
	auto c = [v]( const vec2* a, int n )
	{
		string s;
		
		for( int i=0; i<n; ++i )
		{
			s += v(a[i]) + "\n";
		}
		
		return s;
	};
	
	t.push_back( XmlTree( "CameraIndex", toString(mCameraIndex) ) );
	t.push_back( XmlTree( "CaptureSize", v(mCaptureSize) ) );
	
	t.push_back( XmlTree( "CaptureCoords", c(mCaptureCoords,4) ));
	t.push_back( XmlTree( "CaptureWorldSpaceCoords", c(mCaptureWorldSpaceCoords,4) ));

	t.push_back( XmlTree( "ProjectorCoords", c(mProjectorCoords,4) ));
	t.push_back( XmlTree( "ProjectorWorldSpaceCoords", c(mProjectorWorldSpaceCoords,4) ));
	
	t.push_back( XmlTree( "ProjectorSize", v(mProjectorSize)) );

	if ( mDistCoeffs.rows==1 )
	{
		std::ostringstream ss;
		ss << mDistCoeffs;
		std::string dc;
		t.push_back( XmlTree( "DistCoeffs", dc ) );
	}
	
	return t;
}