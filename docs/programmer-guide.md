# Programmer's Guide

This file explains how to create a new GameWorld mode for Tabla. Tabla leans heavily on libcinder and OpenCV, but you don't need to understand all that to write a new game mode. The main prerequisite is working knowledge of C++ and some graphics programming experience. To do more advanced projects you'll need to dig into OpenCV yourself, and learn more about libcinder and OpenGL.

If you haven't already, you should read through installation.md, as it covers basic concepts this file assumes you know. While it's not necessary to have digested the architecture.md guide, you might want to read that after working through this guide, as it help you do projects using more advanced OpenCV techniques.

## Hello, Tabla

Let's make a sample project together. This project will detect rectangles, and draw different colors in them. This project is a fun and simple way to get up and running while covering all the key concepts you need to understand in order to build a world.

### GameWorld

- Make your class and source files.
	- Parent class; methods to fill in.
- Cartridge + Library
- Let's make it draw something. A circle in the middle of the table. Voila!
- Run it. Launch your world.

### Adding Vision

- xml file, parameters
	- note hot-loading
- iterate contours; maybe just draw them all? that's pretty fun already.
- detect and draw rectangles

## Just add data

### Adding XML parameters

### Adding Data Files

- Directory structure
- FileWatch
- Image
- Shader

## Further and Farther

Where do you go from here? If you want to learn more we suggest you check out the existing projects. They demonstrate a range of different techniques.

### Extracting Graphics

AnimWorld is a simple program that demonstrates how to extract and work with graphics directly, as it pulls out images from rectangles, and then projects them onto other rectangles. The most advanced demo we have of custom vision code can be found in MusicWorld. Ultimately, you can do anything you want with OpenCV, but in practice interpreting contours gets you quite far.

### 3d Graphics

PinballWorld demonstrates a technique for doing 3d graphics.