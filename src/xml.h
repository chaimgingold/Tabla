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


inline bool getXml( XmlTree &xml, string name, bool& var )
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

inline bool getXml( XmlTree &xml, string name, float& var )
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

inline bool getXml( XmlTree &xml, string name, vec2& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() );
			vec2 t ;
			s >> t.x >> t.y ;
			if (s.good())
			{
				var=t ;
				return true;
			}
		}
		catch( std::exception ){}
	}
	
	return false;
} ;

inline bool getXml( XmlTree &xml, string name, ColorAf& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() );

			unsigned int value;
			s >> hex >> value;
			// TODO: Take 0xRGBA as well as 1 1 1 format; test for "0x" at start
			
			if (s.good())
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
bool getXml( XmlTree &xml, string name, T& var )
{
	auto n = xml.begin(name);
	
	if (n!=xml.end())
	{
		try {
			stringstream s( n->getValue() );
			T t ;
			s >> t ;
			if (s.good())
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
