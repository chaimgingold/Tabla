[x] Split out MusicVision as separate class

[x] Score.h

[x] Remove all world refs from Score. Pass in pieces needed, or copy to scores (e.g. mNumOctaves).

[x] Instrument.h
[x] Instrument.cpp

[x] Move all MIDI note tracking into Instrument, which should send note-on/note-offs.

[x] Instrument handles arpeggiator.

[x] tOnNoteKey doesn't need instrument ref if note manager is per-instrument.

[x] Copy mOnNotes before iterating to simplify tOnNoteKey deletion logic.


[ ] Pass instrument list to score rather than pulling it from world, move getInstrumentForScore(s) to:
 score->findInstrumentIn(instruments)
 (did we obviate this? I think so.)



[ ] MetaParams.cpp
[ ] MetaParams.h

[ ] MetaParam struct that carries them in a bundle so they can be passed wholesale down to Score from MusicWorld
 
[ ] Clock.h
[ ] Clock.cpp

[ ] AdditiveSynth.cpp


[ ] Before Arp:
      Instrument::setNotes(vector<Notes> newNotes) {
 
 
     NoteSender
     vector<Note> mOnNotes
     void NoteSender::triggerNotes(vector<Notes> newNotes) {
         let noteOns  = newNotes `diff` mOnNotes
			 noteOffs = mOnNotes `diff` newNotes;
         sendNoteOns <$> noteOns;
         sendNoteOffs <$> noteOffs;
         mOnNotes = newNotes;
     }

     can also add scheduleNote with old behavior of note+duration
 
 
     then Instrument has its own mOnNotes and the arpeggiator sends rhythmic cyclings of those notes 

[x] Striker->RobitPokie

