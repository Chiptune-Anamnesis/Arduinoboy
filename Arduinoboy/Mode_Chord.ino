/**************************************************************************
 * Name:    Timothy Lamb                                                  *
 * Email:   trash80@gmail.com                                             *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

void modeChordSetup()
{
  digitalWrite(pinStatusLed,LOW);
  pinMode(pinGBClock,OUTPUT);
  digitalWrite(pinGBClock,HIGH);

#ifdef USE_TEENSY
  usbMIDI.setHandleRealTimeSystem(NULL);
#endif

  blinkMaxCount=1000;
  blinkLight(2,1);
  modeChord();
}

void modeChord()
{
  while(1){                                // Loop foreverrrr
    modeChordUsbMidiReceive();

    if (serial->available()) {          // If MIDI is sending
      incomingMidiByte = serial->read();    // Get the byte sent from MIDI

      if(!checkForProgrammerSysex(incomingMidiByte) && !usbMode) serial->write(incomingMidiByte); //Echo the Byte to MIDI Output

      // 
      if(incomingMidiByte & 0x80) {
        switch (incomingMidiByte) {
          case 0x80: // Note off
          case 0x90: // Note on
          case 0xB0: // CC
            midiValueMode = false;
            midiAddressMode = true;
            midiData[0] = incomingMidiByte;
            sendByteToGameboy(midiData[0]);
            delayMicroseconds(CHORD_MIDI_DELAY);
            statusLedOn();
            break;
          case 0xf8: // Clock
            midiValueMode = false;
            midiAddressMode = false;
            midiData[0] = incomingMidiByte;
            sendByteToGameboy(midiData[0]);
            delayMicroseconds(CHORD_MIDI_DELAY);
            statusLedOn();
            break;
          default:
            midiValueMode = false;
            break;
        }
      } else if (midiAddressMode){
        midiAddressMode = false;
        midiValueMode = true;
        midiData[1] = incomingMidiByte;
        sendByteToGameboy(midiData[1]);
        delayMicroseconds(CHORD_MIDI_DELAY);
      } else if (midiValueMode) {
        midiAddressMode = true;
        midiValueMode = false;
        midiData[2] = incomingMidiByte;
        sendByteToGameboy(midiData[2]);
        delayMicroseconds(CHORD_MIDI_DELAY);
        statusLedOn();
        blinkLight(midiData[0],midiData[2]);
      }
    } else {
      setMode();                // Check if mode button was depressed
      updateBlinkLights();
      updateStatusLed();
    }
  }
}

void modeChordUsbMidiReceive()
{
#ifdef USE_TEENSY

    while(usbMIDI.read()) {
        uint8_t ch = usbMIDI.getChannel() - 1;
        uint8_t s;

        switch(usbMIDI.getType()) {
            case usbMIDI.NoteOff: // Note off
            case usbMIDI.NoteOn: // Note on
                s = usbMIDI.NoteOn + ch;
                if(usbMIDI.getType() == usbMIDI.NoteOff) {
                    s = usbMIDI.NoteOff + ch;
                }
                sendByteToGameboy(s);
                delayMicroseconds(CHORD_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData1());
                delayMicroseconds(CHORD_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData2());
                delayMicroseconds(CHORD_MIDI_DELAY);
                blinkLight(s, usbMIDI.getData2());
            break;
            case usbMIDI.ControlChange: // CC
                sendByteToGameboy(usbMIDI.ControlChange + ch);
                delayMicroseconds(CHORD_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData1());
                delayMicroseconds(CHORD_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData2());
                delayMicroseconds(CHORD_MIDI_DELAY);
                blinkLight(0xB0+ch, usbMIDI.getData2());
            break;
            case usbMIDI.Clock: // Clock
                sendByteToGameboy(usbMIDI.Clock);
                delayMicroseconds(CHORD_MIDI_DELAY);
            break;
        }

        statusLedOn();
    }
#endif

#ifdef USE_LEONARDO

    midiEventPacket_t rx;
      do
      {
        rx = MidiUSB.read();
        uint8_t type = rx.byte1 & 0xF0;
        
        if (type == 0x80 ||
            type == 0x90 ||
            type == 0xb0) {
          sendByteToGameboy(rx.byte1);
          delayMicroseconds(CHORD_MIDI_DELAY);
          sendByteToGameboy(rx.byte2);
          delayMicroseconds(CHORD_MIDI_DELAY);
          sendByteToGameboy(rx.byte3);
          delayMicroseconds(CHORD_MIDI_DELAY);
          blinkLight(rx.byte1, rx.byte2);
        } else if (rx.byte1 == 0xf8) {
          sendByteToGameboy(rx.byte1);
          delayMicroseconds(CHORD_MIDI_DELAY);
        }
          
        statusLedOn();
      } while (rx.header != 0);
#endif
}
