//
//  ocv.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/10/16.
//
//

#ifndef ocv_h
#define ocv_h

namespace cinder {
	
	inline PolyLine2 fromOcv( vector<cv::Point> pts )
	{
		PolyLine2 pl;
		for( auto p : pts ) pl.push_back(vec2(p.x,p.y));
		pl.setClosed(true);
		return pl;
	}

	inline glm::mat3x3 fromOcvMat3x3( const cv::Mat& m )
	{
		glm::mat3x3 r;
		
		for( int i=0; i<3; ++i )
		{
			for( int j=0; j<3; ++j )
			{
				r[j][i] = m.at<double>(i,j);
				// opencv: (row,column)
				// glm:    (column,row)
			}
		}
		
		return r;
	}
}


#endif /* ocv_h */
