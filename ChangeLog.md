12/05/2016 - 12/11/2016
* New TokenWorld CV tracking playground for testing out OpenCV's library of techniques
* Game Controller support for Pinball
* Further work

11/28/2016 - 12/05/2016
* Stamp tracking much more solid, controllable.
* Multi-measure stamp tracking much better.
* Bug fix to new stamp/instrument->score assignment.
* Rippling shaders for dancing robit icons
* BallWorld drawing optimization
* Rainbow shader stood up in app

11/21/2016 - 11/26/2016
* Multi-measure tracks (and short tracks) works properly now.
* New stamp system for instrument assignment.
* More awesome character animation.
* New Sharp Crab character
* Robits V3, now with 12-solenoid robit

11/14/2016 - 11/18/2016
* Barrel distortion correction!
* New table!
* Robits V2! Now with 2 Robits!!
* Robits V1!
* Paula Icons!
* New "Tuned Additive Synth" that adapts to tuning and scale of rest of song
* ClearMIDINotes exe for killing midi when force-killing the app via Xcode stop button.
    Use Xcode>Preferences>Behaviors>Running>Completes>"Run" and choose xcode/scripts/ClearMIDINotes
* Drag-and-drop image onto config window to use it as a faux camera. Escape to clear.
* Refactored MusicWorld into smaller files

10/31/2016 - 11/4/2016
* Clock system for synced variable tempos
* Disable sleep
* Metaparameter sliders (tempo/scale/root-note/beats-per-measure)
* Discrete sliders for scales
* Sliders easier to see
* Calibration game world WIP
* Better slider value persistence; root note slider is discrete.
* Improved slider graphics; improved FPS, refactored Score::draw().
* Meta-sliders have much more solid inter-frame coherency.
* RFID tag -> game loading works
* alt-tab to cycle cameras; setting is locally persisted.

10/24/2016 - 10/28/2016
* Danceability! Fix off-by-one errors in gridlines and number of beats during playback.
* Persistent scores!
    Can now dynamically move hands through scores without disrupting playback.
* Adaptive temporal blending/stabilization!
    keeps slow/un-moving objects more spatially stable
    while still allowing fast-moving objects
* Finally fix AdditiveSynth stability
   (by passing active column as a list rather than an array, which was causing Pd thread contention)
* Added internal global parameter support for switching root note, beat length, scale
* Added more scales
* WIP first pass at global parameter UIs
* Hotload speed improvement
* Beat quantization lines
* Concave score rejection
* Fix RtMidi runloop interaction and crashes
* Fix Pd crash-on-exit
* Initial multi-tempo support
* Drawing performance improvements mk 2


10/17/2016 - 10/21/2016
* MIDI synths!
* Instrument regions!
* Scale system!
* Note-to-channel system for Korg Volca Sample
* Large AdditiveSynth optimizations
* Octave shifting + indicator
* Tunable thresholding
* Drawing performance improvements

10/10/2016 - 10/14/2016
* Music world is born
* Additive synths
