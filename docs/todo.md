# Packaging
- [ ] License
- Documentation
	- [ ] About; finish it.
	- [ ] Installation; finish it.
	- [ ] App architecture; write it.		
	- [ ] Programming guide (for game worlds); write it.
	- [ ] How to play. Explanation of modes, behaviors; write it.
- [ ] Contributors; Write it: Who made this? Who has done what?
- [ ] Flesh out Related Work

# Application

## New Features
- [?] Finish token system
	- XML Parsing
		- [x] Make Vision/Token XML entries into attributes
		- [x] put a path attribute in Tokens prepended to each Token path
- [x] MIDI controller input!
- [ ] Each GameWorld can have their own settings (either in settings.xml, or in per-game files--latter, i think). e.g. for which way is up

## Usability
### Major
- [ ] Configurable (or adaptive) up vector for modes... 
- [ ] Automatic projector calibration. (And rename CalibateWorld to CameraCameraCalibrateWorld; add a ProjectorCameraCalibrateWorld that does this). Simply project a box in known projector space, find it in world space, then use that perspective transform to set the projector profile's mProjectorCoords.
- [x] Populate capture devices with appropriate resolutions (not just 640x480!)
	- Capture::Device::getNative() on MacOS returns => AVCaptureDevice*
	- https://developer.apple.com/reference/avfoundation/avcapturedevice (see formats)
	- http://stackoverflow.com/questions/34640551/can-avcapturesession-use-custom-resolution
	- https://developer.apple.com/reference/coremedia/1489287-cmvideoformatdescriptiongetdimen?language=objc
	- https://developer.apple.com/reference/avfoundation/avcapturedeviceformat
- [ ] Filter prepopulated capture device formats => profiles
- [x] World bounds poly editor shows camera image, dimensions in CM as text, snaps to CM, and constrained to rectangle.
- [x] Pull down menu with capture options
- [ ] Can easily put Config window in a high performance mode (no gl textures drawn) for demo machine.

### Minor
- FPS indicators
	- [ ] FPS more visual, especially capture, which varies a lot
	- [ ] Smooth it before showing maybe.
- [ ] Capture menu only shows valid options
- [ ] Escape key should exit debug frame and THEN return to last camera source

## Refactor
### For Game Jam / New Contributors
- Game controller
	- [ ] Move to App, not just Pinball
	- [ ] allow multiple controller profiles (PS4, Xbox, etc...).
- Firewall game worlds from one another and the main app. Make it easy to see that people checking in game worlds aren't modifying files outside of the scope of that game world.
	- [x] Instantiate static cartridge objects that add themselves to a global list.
		- [x] Get rid of left/right keys;
		- [ ] Replace with a reset gameworld metakey. (or force worlds to deal with this use case themselves.)
	- [ ] Put assets for each world in their own folder
		- [ ] Add a GameWorld::getAssetPath() function, and use this to pluck the proper file paths.
	- [ ] Restrict include path search so that #include must be more explicit about which file. Do this to prevent accidental includes across GameWorlds. So, #include "tabla/Vision.h" vs. #include "worlds/music/MusicVision.h"
	- [ ] Remove "World" naming all over the place. So just "World/Pinball/Pinball.h" and "Pinball".
- [ ] Move settings to /Library. (Do before distributing it more widely, so we make migration later easier.)

### Minor
- [ ] Move RectFinder into Vision pipeline--not sure this is a good idea yet
- [x] Refactor TablaWindow to manage all the WindowRef stuff, so move direct WindowRef interaction from TablaApp into TablaWindow 
- [ ] ~~Put lightlink.xml in settings.xml (i think it's a good idea...; maybe not?)~~
- [x] Make polygon xml into a list of vertices inside of polygon block: e.g. <v>x1 y1</v> <v>x2 y2</v> etc... (will make file look less crazy).

# Game Worlds

## Projector Calibration
- Improve usability
	- [ ] Show when it is done more clearly
	- [ ] Continuous feedback when new projector calibration is being set (flash something?)--
		Show us which points are working/aren't working...
	- [ ] Don't recompute/save changes until new poly set.
	- [ ] Sample a bunch of points nearby and smooth out?

## Music
- [ ] Easily configurable for different instrument setups. Plan: remove Token list section, fold into each instrument definition. Add <Ensemble>, so we can define multiple ensembles (could have priorities so that one that fails to load could then fall back to another), and you choose them from a menu (MIDI, OpenSFZ, Live, etc...). Would be nice to programatically invert the token images for the recognizer, but whatever.
- [x] Built in software synths
    - [ ] Drum software synth
- [ ] Use object color to influence performance/sound of each note.
- [ ] Add Luke's cool sampling synth!
- Robits
	- [ ] No startup notes.
- [ ] Tokens for instruments!
	- [x] Tokens set score instruments
	- [ ] Draw visual feedback on token (filled pulsing rainbow or something).
	- [ ] Tokens show their radius of influence
	- [x] Multiple instruments per score!
	- [ ] Lower latency for token hiding/showing (implement a per-frame approximation technique)
- [x] Improve Stamp <-> Score tracking
	- [x] Remember last seen polygon, to help with interframe tracking. Draw it. To help player see it, and me improve the algorithm. Maybe poly<>poly intersection, or poly<>poly distance, or poly<>poly similar min bounding rect location, size, orientation (look at difference across all three axes--size being most important, location and orientation can change more). So pull up ocv rotated bounding box function so we can easily get to it.
	- [x] Timeout for stamp going home... show the timer counting down. (visually, somehow). Maybe fill last seen polygon white, with black stamp on it and flicker stamp (like a damaged video game character).  
- Meta params
	- [x] Default values
		- [ ] Encoded in xml. (Only have DefaultTempo; others we are assuming is zero--not necessarily true).
		- [x] If slider is removed, go back to default value.
		- [x] Go back to them when empty pixel data (no average possible), or there is no slider. (This will require a little refactor to automatically set to default when it is absent; or just another process watching to see if metaparam slider is absent, and set it then; that'd be easiest).
		- [ ] Show it on the slider (so we know where normal is).
	- [ ] Adaptive horizontal or vertical
	- [ ] Try max, not average.
- [x] ~~Beats marked on additive synth?~~ No. Luke says it's not what it's about.
- [ ] Mark beats, etc on scores with no instrument?
- [ ] >1 Instrument per Score
- [ ] Can turn octave adjustment (with space) off in xml. Hide the dot. Octave=0 always

## Pinball
- [x] Score, ball count, high score.
- [ ] Make 4th+ UI element back into space, not UI.
- [x] Remove static cube map file from project, and don't load it.
- [ ] Make flipper speed tunable.
- [ ] Make contour dejittering/reshaping more responsive; if contour moves enough (sum of all points) then just jump it. This might be a bad idea, as we already have this working in local areas of the contour, not the whole thing, which might just be better.
- [ ] Merge overlapping bumpers with convex hull 