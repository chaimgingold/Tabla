# Architecture

What is this file?

- App architecture (architecture.md); this could be folded into installation or programmer's guide--or maybe it really does belong in its own section.
		
## Who is this for?

This is probably not the first document you should read. If you are just getting going as a player or programmer, then start with installation and then the player guide. If you want to program your own world, you should then go to the programmer-guide. 

If you want to hack on Tabla's host app, or go deeper into ideas touched upon in programming or installation guides then this is for you. If you are technically minded, it might help you do the software setup, as it explains in more detail some of those concepts. 

## The Light Link
 
What is the light link? It establishes correspondences between the camera, projector, and the world.

### The Problem

### Coordinate Spaces

	/* Coordinates spaces, there are a few:
		
		UI (window coordinates, in points, not pixels)
			- pixels
			- points
			
		Image
			- Camera
				e.g. pixel location in capture image
			- Projector
				e.g. pixel location on a screen
			- Arbitrary
				e.g. supersampled camera image subset
				e.g. location of a pixel on a shown image
		
		World
			(sim size, unbounded)	eg. location of a bouncing ball
	 
	 
	Transforms
		UI    <> Image -- handled by View objects
		Image <> World -- currently handled by Pipeline::Stage transforms

	*/
	
### Linking Coordinate Spaces

- Projection mapping system (get photo Luke took of my explanation).
	- Coordinate spaces (table/world, projector, camera, etc...)