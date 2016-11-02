


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
