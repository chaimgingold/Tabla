# Application

## Major Bugs
- [?] Projector window sometimes comes up blank.
	- Solution implemented: hide window before fullscreening it. (!)
	- Needs more testing to verify this did it; It might not be that robust of a fix.
	- Seems like full-screening that second window sometimes results in two zombie windows for the projector (a fullscreen one, and a second windowed one). Probably a libcinder issue, or a misuse of libcinder; perhaps a simple change of flags on that window will do the trick? 
	- Known workaround: disable <HasConfigWindow> in app.xml.
	- Another possible workaround: fullscreen, then un-fullscreen the second bogus blank window. (Can we do this programatically?)
	- Might be a cinder limitation that requires us to drop into the OS ourselves:
		https://forum.libcinder.org/topic/extra-blank-window-hiding-behind-main-window-in-default-fullscreen-mode
- [?] When table is blank, OTSU algorithm freaks out.
	- Mitigating solution implemented: Compute OTSU value on camera input image, and apply to clipped area.
	- SO: This will work for installations where we can place lights NEAR the table, causing OTSU to partition based upon entire camera input.
	- Known workarounds:
		- Set thresholding constant in <Vision> to something between 0..255 (-1 or undefined means OTSU).
		- Put blank paper on the table
	- Notes on proper solution: If we did thresholding before clipping to the table we'd probably get better results. Or analyze full histogram and manually set the threshold. (Avoid doing 2x clipping transforms of the full image and also the thresholded image--that would be lame.)

## Minor Bugs
- [x] Use Scissor Rect on Pop up menu. Refactor code in PipelineStageView::draw() to be reusable.
	- [ ] Button is clipped, but not menu.
- [ ] Rationalize timing for debug file input frame skip. Ideally Vision thread sleep is dynamic, based upon desired FPS--for file and camera--and average/last execution time.
- [ ] Crash on quit from CoreAudio system
- [ ] Music crash in SFZ unit via Core Audio Node system (on quit)

# Game Worlds

## General
- No way to define and save orientation (which way is up?) for various modes. (Pinball, Music, Animation, ...). This problem has two components:
	- Interface for setting direction (can imagine joystick helping here, but that's limited.)
	- Storing it in app specific settings. Although an up-for-editor-mode type things (Music, Animation) that spans apps might be called for. And a second for long game tables (Pong, Pinball) might also make sense. But more elegant might be per mode. I can imagine a future day in which modes mix more freely (use speed token to define speed for ANY mode, in which case a more general OS type sensibility of shared settings might make more sense)
- [x] mDrawMouseDebugInfo not being respected in BallWorld -- drawMouseDebugInfo still getting called for some reason.
- [ ] If hotloaded (or even post-relaunch) test image changes size, then image will update but size will remain stuck at old one. We need to detect this and update the capture profile to reflect the new size. 

## Projector Calibration

- [ ] Rotationally off

## BallWorld
- Detect, flag, and possibly stabilize or fix stuck balls (in spaces that are too small). Do this in ball world. Track old velocities for last three frames could do the trick. (see if it is oscillating wildly, and maybe even track for how long...; then we have rules for what to do about it when detected.)

## Anim
- Sometimes screen draws on computer, but not table! Maybe after pinball mode to repro?

## Music
- Robit serial connection isn't robust; sometimes need to restart computer to get it to see the arduino.
- [ ] Music instrument character wrangling can be hard. Proper solution probably involves tokens for instruments placed next to scores.
- [ ] More reliable/robust token detection
- [ ] Low register of additive synth is hard to hear.
- [ ] More robust character tracking
	- [ ] Have a timeout before they jump home
	- [ ] Do NxM scoring like in Pinball UI tracking
- [ ] Conflicts between stamp search poly and a score should send stamp home directly (instead of timing out, as it does now).
- [ ] Overlapping dancing icons need fixed.

## Pinball
- [ ] Audio sometimes blows up then drops out; culprit seems to be updateBallSynthesis(). To reproduce, load "pinball 1.png" test image in Pinball and spawn multiple balls (with b key).
- [ ] Ball reflection texture map sometimes doesn't show up (seems to not even render, so it's black in debug view, too.). Not sure what this is... A workaround is that if you change MipMap then FBOS get recreated an it resolves itself... So might be an internal cinder issue. Maybe dont pull textures, or recreate fbos... 
- [ ] Max multiball needed... (it can get stuck and keep spawning!)
- Lattice background is hard coded to our setup--in pixel space. So rotation will be wrong in many setups, gradient effect locations, and ball reflection is wrong. To fix:
	- [ ] Lattice should be in world space, oriented to up vector.
	- [ ] Win/lose gradient effects need to respond.
- Not using mipmap for ball reflection. So different tunings and resolutions will not look good.
- Ball could sometimes tunnel through tip of flipper. Flipper rotation done in BallWorld (for finer integration and less tunneling). Proper fix (and architecture) would be to allow a parallel index of rotational transforms for contours that BallWorld does itself (and can do finer integrations of). 
- Small spurious awkward spaces trap the ball (eg groups of white go stones, your hands, etc...). Solution: reject these spaces, perhaps comparing to perimeter/area of a circle that comfortably fits a ball (or more).
- Flippers penetrate walls. Fix: dynamically size them! Only respond to walls, not parts--or things could get crazy.
- Pinball "palm" rejection. People sometimes rest against table. Detect these awkward edge spaces and reject. Probably need a mask array for contours that defines who is a wall; could have another for who is a part. Or unify these into one list, with types (ignored, wall, part)--i like, and we could peel mask functions off of it. Basically !shouldBePart() is used as shouldBeSpace(), and we need the latter to open up the semantic possibilities for contour rejection, UI, etc... Types:
	- Part (could even point to part...)
	- Space (Hole/Space or Non-Hole/Obstacle; implicit with contour type, but we could make it fully explicit in our typing system.)
	- Ignore (Rejected)
	- UI (Score, Balls Remaining, Top Score, etc...)
	- ...future expansion (eg wiring elements)
- Ball messed up
	- [ ] Sometimes first ball is blank (so i did an awful kludge that seems to not really work)
	- [ ] Sometimes ball appears messed up on projector (all white-ish) but fine in main display (!).
		- Possible workaround would be to hide/show main window and see if that helps.
