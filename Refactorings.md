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

[ ] Clock.h
[ ] Clock.cpp

[ ] AdditiveSynth.cpp
