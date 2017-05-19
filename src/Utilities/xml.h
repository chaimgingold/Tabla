//
//  xml.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/5/16.
//
//

#ifndef xml_h
#define xml_h

#include "cinder/Xml.h"

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <vector>

//// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
//// trim from start
//static inline std::string &ltrim(std::string &s) {
//    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
//            std::not1(std::ptr_fun<int, int>(std::isspace))));
//    return s;
//}
//
//// trim from end
//static inline std::string &rtrim(std::string &s) {
//    s.erase(std::find_if(s.rbegin(), s.rend(),
//            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
//    return s;
//}
//
//// trim from both ends
//static inline std::string &trim(std::string &s) {
//    return ltrim(rtrim(s));
//}

using namespace ci;
using namespace std;

inline bool getXml( const XmlTree &xml, string name, bool& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			var = stoi(n->getValue());
			return true;
		}
		catch( std::exception ){}
	}
	
	return false;
} ;

inline bool getXml( const XmlTree &xml, string name, string& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		var = n->getValue() ;
		return true;
	}
	
	return false;
} ;

inline bool getXml( const XmlTree &xml, string name, float& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			var = stof( n->getValue() ) ;
			return true;
		}
		catch( std::exception ){}
	}
	
	return false;
} ;

inline bool getXml( const XmlTree &xml, string name, vec2& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() ); //+ " " ); // hack, because !s.fail() fails when it bumps against the end of the string
			vec2 t ;
			s >> t.x >> t.y ;
			if (!s.fail())
			{
				var=t ;
				return true;
			}
		}
		catch( std::exception ){}
	}
	
	return false;
} ;

inline bool getXml( const XmlTree &xml, string name, vector<vec2> &var )
{
	auto n = xml.begin(name);
	bool ok=false;
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() );
			vec2 t ;

			while(1)
			{
				s >> t.x >> t.y ;
				
				if (!s.fail()) var.push_back(t) ;
				else break;
				
			}
		}
		catch( std::exception ){}
	}
	
	return ok;
} ;

template<class T>
bool getXml( const XmlTree &xml, string name, vector<T> &var )
{
	auto n = xml.begin(name);
//	bool ok=true;
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() );
			T t ;

			while(1)
			{
				s >> t ;
//				cout << t;
				
				if (!s.fail()) var.push_back(t) ;
				else break;
				
			}
		}
		catch( std::exception ){}
	}
	
	return true;
} ;


inline bool getXml( const XmlTree &xml, string name, vec2 var[], int len )
{
	vector<vec2> v ;
	
	getXml(xml, name, v);
	
	if (v.size()==len)
	{
		for( size_t i=0; i<v.size(); ++i ) var[i]=v[i];
		
		return true ;
	}
	else return false ;
}


inline vector<vec2> getVec2sFromXml( const XmlTree &xml, string name )
{
	vector<vec2> vs;

	for( auto i = xml.begin( name + "/v" ); i != xml.end(); ++i )
	{		
		vec2 v;
		if ( sscanf( i->getValue().c_str(), "%f %f", &v.x, &v.y ) == 2 )
		{
			vs.push_back(v);
		}
	}
	
	return vs;
}

inline bool getVec2sFromXml( const XmlTree &xml, string name, vec2 var[], int len )
{
	vector<vec2> vs = getVec2sFromXml(xml,name);

	if (vs.size()==len)
	{
		for( int i=0; i<len; ++i ) var[i] = vs[i];
		return true;
	}
	else return false;
}

inline string vecToString( vec2 v )
{
	return toString(v.x) + " " + toString(v.y);
}

inline XmlTree vec2sToXml( string name, vector<vec2> vs )
{
	XmlTree xml(name,"");
	for( auto v : vs ) {
		xml.push_back( XmlTree("v", vecToString(v) ) );
	}
	return xml;
}

inline XmlTree vec2sToXml( string name, const vec2* vs, int n )
{
	XmlTree xml(name,"");
	for ( int i=0; i<n; ++i ) {
		vec2 v = vs[i];
		xml.push_back( XmlTree("v", vecToString(v) ) );
	}
	return xml;
}

inline bool getXml( const XmlTree &xml, string name, ColorAf& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() );

			unsigned int value;
			s >> hex >> value;
			// TODO: Take 0xRGBA as well as 1 1 1 format; test for "0x" at start
			
			if (!s.fail())
			{
				var = ColorA::hex(value) ;
				return true;
			}
		}
		catch( std::exception ){}
	}
	
	return false;
} ;

template<class T>
bool getXml( const XmlTree &xml, string name, T& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() );
			T t ;
			s >> t ;
			if (!s.fail())
			{
				var=t ;
				return true;
			}
		}
		catch( std::exception ){}
	}
	
	return false;
} ;

#endif /* xml_h */
