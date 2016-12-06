//
//  KeyFunctionMap.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/6/16.
//
//

#ifndef KeyFunctionMap_h
#define KeyFunctionMap_h

class KeyFunctionMap
{
public:

	void setParams( XmlTree ) override; // XmlTree's children are traversed
	/*	<params>
			<key>
				<key>key-name</key>
				<value>function-name</value>
			</key>
			...
		</params>
	*/

//	void handleKey( )
	
	typedef int tKey; // template-ize?
	
	map<tKey,string> mKeyToInput; // maps keystrokes to input names
	map<string,function<void()>> mInputToFunction; // maps input names to code handlers

	
};

#endif /* KeyFunctionMap_h */
