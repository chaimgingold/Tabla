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

- Firewall Game Worlds from one another and the main app. Make it easy to see that people checking in game worlds aren't modifying files outside of the scope of that game world.
	- [x] Instantiate static cartridge objects that add themselves to a global list.
		- [x] Get rid of left/right keys;
		- [ ] Replace with a reset gameworld metakey. (or force worlds to deal with this use case themselves.)
	- [ ] Put assets for each world in their own folder
		- [ ] Add a GameWorld::getAssetPath() function, and use this to pluck the proper file paths.
	- [ ] Restrict include path search so that #include must be more explicit about which file. Do this to prevent accidental includes across GameWorlds. So, #include "tabla/Vision.h" vs. #include "worlds/music/MusicVision.h"
- [ ] Put lightlink.xml in settings.xml (i think it's a good idea...; maybe not?)
- [x] Make polygon xml into a list of vertices inside of polygon block: e.g. <v>x1 y1</v> <v>x2 y2</v> etc... (will make file look less crazy).
- [ ] Move settings to /Library
- [ ] Each GameWorld can have their own settings (either in settings.xml, or in per-game files). e.g. for which way is up
- [ ] Automatic projector calibration. (And rename CalibateWorld to CameraCameraCalibrateWorld; add a ProjectorCameraCalibrateWorld that does this). Simply project a box in known projector space, find it in world space, then use that perspective transform to set the projector profile's mProjectorCoords.
- Game controller
	- [ ] Move to App, not just Pinball
	- [ ] allow multiple controller profiles (PS4, Xbox, etc...).
- [ ] Move RectFinder into Vision pipeline
- [ ] Finish token system

# Game Worlds

## Projector Calibration
- [ ] Use cv::cornerSubPix for more accuracy
- [ ] Account for situations where centered rect is off the table... (use mouse to position it?)

## Music
- [ ] Easily configurable for different instrument setups.
- [ ] Built in synths
- [ ] Use object color to influence performance/sound of each note.
- [ ] Add Luke's cool sampling synth!
- Robits
	- [ ] No startup notes.
- [ ] Tokens for instruments!
	- [ ] Integrate with stamp system
	- [ ] Multiple instruments per score!

## Pinball
- [ ] Score, ball count, high score.
- [ ] Remove static cube map file from project, and don't load it.
- [ ] Make flipper speed tunable.