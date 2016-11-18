//
//  main.m
//  Based on DHSendMIDI Copyright (c) 2013 Douglas Heriot
//
//  Created by Luke Iannini on 17/11/16.
//

#import <Foundation/Foundation.h>
#import <SnoizeMIDI/SnoizeMIDI.h>

int main(int argc, char * const *argv)
{
	@autoreleasepool
	{
		printf("Clearing all connected MIDI devices... ");
		NSMutableArray *messages = [NSMutableArray array];
		for (int channel = 1; channel<=16; channel++) {
			for (int note = 0; note < 128; note++) {
				SMVoiceMessage *message = [[SMVoiceMessage alloc] initWithTimeStamp:0 statusByte:SMVoiceMessageStatusNoteOff];
				[message setTimeStampToNow];
				[message setChannel:channel];
				[message setDataByte1:note];
				[message setDataByte2:0];

				[messages addObject:message];
			}
		}
		
		
		SMPortOutputStream *os = [[SMPortOutputStream alloc] init];
		
		// Set the destination endpoints
		[os setEndpoints:[NSSet setWithArray:[SMDestinationEndpoint destinationEndpoints]]];
		
		
		// Send the message
		[os takeMIDIMessages:messages];

		printf("Done.\n");

	}
    return 0;
}





