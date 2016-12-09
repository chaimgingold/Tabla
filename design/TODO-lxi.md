QUALITY OF LIFE
[ ] Use last cartidge-world on next startup

[ ] Integrate FluidSynth & sample library

[ ] Ball world synthesis

[ ] Support hotswapping arduino
		Wrap mSerialDevice->writeBytes in sendSerialByte in try/catch, null mSerialDevice in catch.
		Periodically run SerialPort::getPorts(); if mSerialDevice is null and setup when one is found.

 
[ ] Tempo-sync gradients by using multiples of PI for the cosine waves and for the speeds at which time progresses (which should also be multiplied by tempo)

SYNTH IDEAS
[ ] Vocoder!!!!!!

[ ] Autotune! This can be separate from vocoder, since we can use pitch of voice

BUGS
 
[ ] Additive synth rendering is very slow; maybe texture transfer phase? 
 
[x] Fix stuck notes on paper removal
[ ] Fix crashes on exit (try newer branch of LibPd-Cinder from Ryan)
[ ] Fix losing MIDI when switching cartridges
     (something to do with not closing ports properly?)


 
 
DONE
[x] Harmonic additive synth
