# Packaging
- [ ] License
- [ ] Documentation
	- How many files for this? Probably different ones for game modes... Or maybe not.
	- What is this? (about.md)
	- How to build/set up/install? (installation.md)
		- Hardware list
	- Projection mapping system (get photo Luke took of my explanation).
	- App architecture (architecture.md)
	- For game worlds
	- Misc notes on this...
		- What it is.
		- How to install, calibrate and use.
        - Hardware needed.
            - Camera
            - Projector
            - Table you can black out
        - Software setup.
            - How to calibrate
                - Concepts
                    - Coordinate spaces
                        - World space units
                - Polygons
                - Xml Files
                - RFID information
            - Advanced
                - xml config blocks per game world. (Music being the most important!)
        - How to play… Explanation of modes, behaviors.
- [ ] Contributors
	- Who made this? Who has done what?
- [ ] Flesh out Related Work

# Application

- [ ] Put lightlink.xml in settings.xml (i think it's a good idea...; maybe not?)
- [x] Make polygon xml into a list of vertices inside of polygon block: e.g. <v>x1 y1</v> <v>x2 y2</v> etc... (will make file look less crazy).
- [ ] Move settings to /Library
- [ ] Each GameWorld can have their own settings (either in settings.xml, or in per-game files). e.g. for which way is up
- [ ] Automatic projector calibration. (And rename CalibateWorld to CameraCalibrateWorld; add a ProjectorCalibrateWorld that does this). Simply project a box in known projector space, find it in world space, then use that perspective transform to set the projector profile's mProjectorCoords.
- Game controller
	- [ ] Move to App, not just Pinball
	- [ ] allow multiple controller profiles (PS4, Xbox, etc...).
- [ ] Move RectFinder into Vision pipeline

# Game Worlds

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