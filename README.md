# La Tabla

La Tabla is a magical table—put things on it and they come to life. Make music and animations. Play games. Design your own pinball tables. Use your body, your friends, paper, drawings, game pieces—whatever strikes your fancy.

For a quick introduction, please visit [http://tablaviva.org](http://tablaviva.org).

La Tabla requires a physical installation utilizing a computer, camera, projector, and tabletop. It is an experimental research project, and some technical expertise is required to build and configure an installation. 

This document is intended for a technically proficient audience interested in building and setting up a La Tabla installation. Please bear in mind that La Tabla is a work in progress that is not as easy to setup, run, configure, and develop for as one would hope.

# Platform

What do you need to build and run La Tabla?

## Hardware

La Tabla requires three key pieces of hardware. These are also the most expensive components:

- Projector
- Camera
- Computer (Mac)

You should also get:

- Speakers

To connect all this stuff you will need a fair amount of appropriately lengthed power cables, HDMI cables, USB cables, USB hub, etc... You might also want some audio speakers so the sound is good.

La Tabla is built with the cross-platform and open source libcinder library. While it has been developed and tested on Mac, it should be possible for a proficient programmer to get it running on Windows (or maybe even iOS?).

We don't have any exact performance specs for the computer, but we have successfully run La Tabla on MacBook Pros from 2013. It does make extensive use of the GPU, both for graphics and computer vision.

### Recommended Camera and Projector

- *Projector.* Optoma ML750ST Ultra-Compact 700 Lumen WXGA Short Throw LED Projector ($525 USD)
- *Camera.* Logitech C922x ($80 USD)

One of the nice thing about this pair of models is that the camera's field of view and projector's throw angle are similar, which means that you can mount them very close to one another, greatly simplifying rigging. For a similar price you might be able to get a larger and brighter projector, but the added bulk will complicate rigging.

Another nice quality is that they both have standard screw threads for mounting to off the shelf rigging gear.

### Low Cost Option

These have much lower resolutions, and the projector is much less bright, but they are inexpensive and have been verified to work with many of the game world modes:

- *Projector.* Abdtech Mini LED Multimedia Home Theater Projector with 1200 Luminous Efficiency ($70 USD)
- *Camera.* Logitech HD Webcam C310 ($30 USD)


## Environment

In addition, you will need a table to play on and a way to mount the projector and camera above the table:

- Black tabletop
- Rigging equipment for projector and camera

La Tabla uses rather simple vision techniques that partition the camera image into light and dark areas. For this reason, it is important to install La Tabla in an environment with somewhat uniform and controllable lighting.

### Suggestions

- We use the IKEA STORNÄS Bar table (not the regular table) ($250 USD)
- Simple clamps. BESTEAM Tripod Camera Clip Clamp ($9 USD)
- Fancy clamps. e.g. Manfrotto 244 Variable Friction Magic Arm with Camera Bracket ($128)

Stay tuned for suggestions on making your own black surface. The dimensions of the Stornas table are pretty good: 50 by 27 inches.

## Accessories

You need things to play with:

- Paper, scissors, markers, game tokens, little plastic blocks, pipe cleaner, etc...
- PlayStation 4 controller (for Pinball).
- RFID reader and tokens (to make game world selection something you do in the world rather than on the host computer screen.)

## Messy Equipment List with Links

Maybe you want to see a messy equipment list we used to plan and build an install:
[Public Google Doc](https://docs.google.com/document/d/1K7NaN21PSqebu3U11W-bXNBG9W4zTyX9aS1QcowLojo/edit?usp=sharing)

# Building La Table Executable

Stay tuned for binary distributions of La Tabla. In the meantime, you have to build La Tabla from source to run it. (This is, at least in part, a kind of Brown M&M test. We want to ensure that people who jump in are prepared to deal with La Tabla's experimental and non-user friendly aspects.)

To build La Tabla you will need:

- XCode (7.3.1 or later)
- La Tabla project and source code (in this repo)
- libcinder for OS X (version 0.9.0). You must download separately and place in an adjacent directory. [https://libcinder.org/download](https://libcinder.org/download)

# Configuring La Tabla

Once you have gotten La Tabla running and the camera and projector installed you need to configure it. In short, you need to:

- Select the right projector and camera (what devices to use?)
- Calibrate camera and projector (where are they?)

## Introduction

First, some basics.

When La Tabla launches you should see two windows:

- Configuration. This is where you monkey around with most settings.
- Projector. If you have only one screen then this window will be shown on your main screen. If you have two screens then La Tabla full screens this window on the second display, which is assumed to be the projector display.

Let's go over the elements of the configuration window:

### Game World Menu

In the bottom right corner is a white box that should say something like "BallWorld" in it. This is a pop-up menu for switching game worlds. Try using it to change the mode. You must hold the mouse (or trackpad) button down for the menu to open it, and release the button to make a selection.

### Camera Menu

The next menu, adjacent to the game world menu, is for choosing the input camera. This menu also shows you various resolution and frame rate options for each device, as well as devices that may not be physically attached to your computer right now.

### Orienting Vector 

Some worlds have an orienting vector, such as gravity in Pinball or time in Animation. The little blue circle next to the camera menu will let you set that vector in the modes that need it.

### Pipeline View

If you push **Command-t** on your keyboard you will toggle a thumbnail view of the image processing pipeline. The pipeline view not only shows you what is happening under the hood, but it is how you configure various aspects of La Tabla.

It is best to hide all the thumbnail images when running La Tabla in a kiosk mode as drawing all of them hammers the frame rate and system responsiveness.

The big image shown in the window is a detail of one pipeline stage, whose thumbnail is outlined in purple. You can change which pipeline stage is shown by clicking on a thumbnail or using the up/down arrow keys. Holding command and pushing up/down will jump to the first/last.

## Selecting Camera

The first thing you'll want to do is select your camera. Not only that, you'll want to pick a good resolution and frame rate. In general you will get the best performance with the highest possible resolution and frame rate for your camera.

## Calibration

To calibrate you will go to certain pipeline stages and drag control points.

The C922x camera has no barrel distortion, but if you are using a custom camera with 
barrel distortion then you might need to compensate for that. To do this you use use the Camera Calibrate Game World, but I haven't documented that yet.

### Camera

Go to the 2nd pipeline stage, called "undistorted." Use the four control points (white circle and red, green, and blue box) to indicate to La Tabla the corners of your black table.

### World boundaries

Go to the next pipeline stage, called "world-boundaries." Drag the green box to indicate how big your table is.

### Projector

Finally, go to the final stage, called "projector." Drag the control points to adjust your projection so that it covers the entire table. It can be helpful to place white objects near the corners of your table to help you see how close you are.

There is a technique for automatically calibrating the projector, or getting close to where you want it to be. The problem is that it doesn't always work, and sometimes it puts your control points off screen, which can require some aggressive window resizing to rectify. I think it works best if you turn off all ambient light. To try it, go to the Projector Calibration World.

## Advanced Configuration

### XML Parameters

Sometimes you have to go to the XML configuration files. Pushing certain keys will open up XML files using whatever editor you have configured in the Finder to open .xml files. In practice you probably won't need to go here, but this is included for reference.

- **Command-x**. La Tabla global parameters.
- **x**. Game World specific parameters.
- **Command-l**. Camera and projector light link config file.

### Other keyboard commands, etc...

- **Command-k**. Trigger Audio-Video Clacker 
- **Command-s**. Save camera image to disk (brings up save dialog box).
- **Command-r**. Reload game world.
- **Command-c**. Toggle drwa contours.

And for completeness:

- **Command-t**. Toggle thumbnail menu.
- **Command-x**. La Tabla global parameters.
- **x**. Game World specific parameters. 
- **Command-l**. Camera and projector.
- **Up**. Go to previous pipeline stage.
- **Down**. Go to next pipeline stage.
- **Command-Up**. Go to first pipeline stage.
- **Command-Down**. Go to last pipeline stage.

Using static debug images:

- Drag an image from Finder into a window to load it. (Will become an option in camera menu.)
- **Escape key**. Forget active debug camera image.

Some game worlds have their own keyboard bindings.

### More...

Stay tuned for more documentation.

# Using Game Worlds

Stay tuned for proper documentation.

Please note that some worlds are for developers and configuration only (e.g. calibration modes), while others require certain accessories (e.g. music and pinball). The bouncing ball mode is the right place to get off the ground as it has no external dependencies.

Switching modes can be done either via an on screen menu, or via RFID scanner and tokens.

# Building New Game Worlds

Stay tuned. In the meanwhile, you might take a look at some of the simpler Game Worlds to see how they are constructed. AnimWorld is a pretty good place to start.

Future plans are to:

- Write an intro for developers.
- Refactor project so that GameWorld assets and source code are in their own directory.
- Better namespacing.
- Cleaner #include path separation.
- Clarify and refine license so that everyone can happily play together.