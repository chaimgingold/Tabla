
[ ] Vocoder!!!!!!

[ ] Autotune! This can be separate from vocoder, since we can use pitch of voice

[ ] Support hotswapping arduino
		Wrap mSerialDevice->writeBytes in sendSerialByte in try/catch, null mSerialDevice in catch.
		Periodically run SerialPort::getPorts(); if mSerialDevice is null and setup when one is found.


BUGS
[ ] Fix stuck notes on paper removal
[ ] Fix crashes on exit (
[ ] Fix losing MIDI when switching cartridges


DONE
[x] Harmonic additive synth
