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

void modeMidiGbSetup()
{
  digitalWrite(pinStatusLed,LOW);
  pinMode(pinGBClock,OUTPUT);
  digitalWrite(pinGBClock,HIGH);

#ifdef USE_TEENSY
  usbMIDI.setHandleRealTimeSystem(NULL);
#endif

  blinkMaxCount=1000;
  modeMidiGb();
}

void modeMidiGb()
{
  boolean sendByte = false;
  while(1){                                //Loop foreverrrr
    modeMidiGbUsbMidiReceive();

    if (serial->available()) {          //If MIDI is sending
      incomingMidiByte = serial->read();    //Get the byte sent from MIDI

      if(!checkForProgrammerSysex(incomingMidiByte) && !usbMode) serial->write(incomingMidiByte); //Echo the Byte to MIDI Output

      if(incomingMidiByte & 0xF0) {
        switch (incomingMidiByte & 0xF0) {
          case 0xF0:
            midiValueMode = false;
            break;
          default:
            sendByte = false;
            midiStatusChannel = incomingMidiByte&0x0F;
            midiStatusType    = incomingMidiByte&0xF0;
            if(midiStatusChannel == memory[MEM_MGB_CH]) {
               midiData[0] = midiStatusType;
               sendByte = true;
            } else if (midiStatusChannel == memory[MEM_MGB_CH+1]) {
               midiData[0] = midiStatusType+1;
               sendByte = true;
            } else if (midiStatusChannel == memory[MEM_MGB_CH+2]) {
               midiData[0] = midiStatusType+2;
               sendByte = true;
            } else if (midiStatusChannel == memory[MEM_MGB_CH+3]) {
               midiData[0] = midiStatusType+3;
               sendByte = true;
            } else if (midiStatusChannel == memory[MEM_MGB_CH+4]) {
               midiData[0] = midiStatusType+4;
               sendByte = true;
            } else {
              midiValueMode  =false;
              midiAddressMode=false;
            }
            if(sendByte) {
              statusLedOn();
              sendByteToGameboy(midiData[0]);
              delayMicroseconds(GB_MIDI_DELAY);
              midiValueMode  =false;
              midiAddressMode=true;
            }
           break;
        }
      } else if (midiAddressMode){
        midiAddressMode = false;
        midiValueMode = true;
        midiData[1] = incomingMidiByte;
        sendByteToGameboy(midiData[1]);
        delayMicroseconds(GB_MIDI_DELAY);
      } else if (midiValueMode) {
        midiData[2] = incomingMidiByte;
        midiAddressMode = true;
        midiValueMode = false;

        sendByteToGameboy(midiData[2]);
        delayMicroseconds(GB_MIDI_DELAY);
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

 /*
 sendByteToGameboy does what it says. yay magic
 */
void sendByteToGameboy(byte send_byte)
{
 for(countLSDJTicks=0;countLSDJTicks!=8;countLSDJTicks++) {  //we are going to send 8 bits, so do a loop 8 times
   if(send_byte & 0x80) {
       GB_SET(0,1,0);
       GB_SET(1,1,0);
   } else {
       GB_SET(0,0,0);
       GB_SET(1,0,0);
   }

#if defined (F_CPU) && (F_CPU > 24000000)
   // Delays for Teensy etc where CPU speed might be clocked too fast for cable & shift register on gameboy.
   delayMicroseconds(1);
#endif
   send_byte <<= 1;
 }
}

#define MIDI_SYSEX_BYTE 0xF0

#define MIDI_STATUS_NOTE_ON 0x90
#define MIDI_STATUS_NOTE_OFF 0x80
#define MIDI_STATUS_CC 0xB0
#define MIDI_STATUS_PC 0xC0
#define MIDI_STATUS_PITCH_BEND 0xE0


#define MIDI_MSG_SYSEX_EOF 0xF7
#define MIDI_MSG_TIMING_CLOCK 0xF8

#define MIDI_MSG_START 0xFA
#define MIDI_MSG_CONTINUE 0xFB
#define MIDI_MSG_STOP 0xFC

#define MIDI_MSG_ACTIVE_SENSING 0xFE
#define MIDI_MSG_SYSTEM_RESET 0xFF

// Are we capturing SysEx bytes?
bool isSysexMessage = false;

void modeMidiGbUsbMidiReceive()
{
#ifdef USE_TEENSY

    while(usbMIDI.read()) {
        uint8_t ch = usbMIDI.getChannel() - 1;
        boolean send = false;
        if(ch == memory[MEM_MGB_CH]) {
            ch = 0;
            send = true;
        } else if (ch == memory[MEM_MGB_CH+1]) {
            ch = 1;
            send = true;
        } else if (ch == memory[MEM_MGB_CH+2]) {
            ch = 2;
            send = true;
        } else if (ch == memory[MEM_MGB_CH+3]) {
            ch = 3;
            send = true;
        } else if (ch == memory[MEM_MGB_CH+4]) {
            ch = 4;
            send = true;
        }
        if(!send) return;
        uint8_t s;
        switch(usbMIDI.getType()) {
            case 0x80: // note off
            case 0x90: // note on
                s = 0x90 + ch;
                if(usbMIDI.getType() == 0x80) {
                    s = 0x80 + ch;
                }
                sendByteToGameboy(s);
                delayMicroseconds(GB_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData1());
                delayMicroseconds(GB_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData2());
                delayMicroseconds(GB_MIDI_DELAY);
                blinkLight(s, usbMIDI.getData2());
            break;
            case 0xB0: // CC
                sendByteToGameboy(0xB0+ch);
                delayMicroseconds(GB_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData1());
                delayMicroseconds(GB_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData2());
                delayMicroseconds(GB_MIDI_DELAY);
                blinkLight(0xB0+ch, usbMIDI.getData2());
            break;
            case 0xC0: // PG
                sendByteToGameboy(0xC0+ch);
                delayMicroseconds(GB_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData1());
                delayMicroseconds(GB_MIDI_DELAY);
                blinkLight(0xC0+ch, usbMIDI.getData2());
            break;
            case 0xE0: // PB
                sendByteToGameboy(0xE0+ch);
                delayMicroseconds(GB_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData1());
                delayMicroseconds(GB_MIDI_DELAY);
                sendByteToGameboy(usbMIDI.getData2());
                delayMicroseconds(GB_MIDI_DELAY);
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
        maybePassThroughSysex(rx.byte1);
        maybePassThroughSysex(rx.byte2);
        maybePassThroughSysex(rx.byte3);

        if (isSysexMessage) continue;

        uint8_t type = rx.byte1 & 0xF0;

        // 3 byte messages
        if (type == MIDI_STATUS_NOTE_ON ||
            type == MIDI_STATUS_NOTE_OFF ||
            type == MIDI_STATUS_PC ||
            type == MIDI_STATUS_CC ||
            type == MIDI_STATUS_PITCH_BEND) {
          sendByteToGameboy(rx.byte1);
          delayMicroseconds(GB_MIDI_DELAY);
          sendByteToGameboy(rx.byte2);
          delayMicroseconds(GB_MIDI_DELAY);
          sendByteToGameboy(rx.byte3);
          delayMicroseconds(GB_MIDI_DELAY);
          blinkLight(rx.byte1, rx.byte2);
        
        // single byte messages
        } else if (rx.byte1 == MIDI_MSG_TIMING_CLOCK || 
                   rx.byte1 == MIDI_MSG_START || 
                   rx.byte1 == MIDI_MSG_CONTINUE ||
                   rx.byte1 == MIDI_MSG_STOP) {
          sendByteToGameboy(rx.byte1);
          delayMicroseconds(GB_MIDI_DELAY);
        }

        statusLedOn();
      } while (rx.header != 0);
#endif
}

#define SYSEX_ID_NON_COMMERCIAL 0x7D

// What MFG id did we find, if we find the mGB one, we'll pass through all the bits
byte capturedSysexId = 0x00;

// Passes through sysex messages to the GB if they are of the form:
// `0xF0 0x7D ..... 0xF7`
void maybePassThroughSysex(byte incomingMidiByte) {
  // byte 1 - Check for SysEx status byte
  if (!isSysexMessage && incomingMidiByte == MIDI_SYSEX_BYTE) {
    isSysexMessage = true;
    capturedSysexId = 0x00; // prompt check for mfg_id
    blinkLight(0x90+2, 1);
    return;
  }

  if (!isSysexMessage) return;

  // byte 2 (mfg_id)
  if (capturedSysexId == 0x00) {
    capturedSysexId = incomingMidiByte;

    // only if it's the right id
    if (capturedSysexId == SYSEX_ID_NON_COMMERCIAL) {
      // replay Sysex status byte, since we skipped it
      sendByteToGameboy(MIDI_SYSEX_BYTE);
      delayMicroseconds(GB_MIDI_DELAY);
      blinkLight(0x90+2, 1);

      // forward current byte (mfg id)
      sendByteToGameboy(capturedSysexId);
      delayMicroseconds(GB_MIDI_DELAY);
      blinkLight(0x90+2, 1);
    }
    return;
  }

  // stop passthrough, wrong id
  if (capturedSysexId != SYSEX_ID_NON_COMMERCIAL) {
    isSysexMessage = false;
    capturedSysexId = 0x00;
    return;
  }

  // stop passing through, EOF
  if (incomingMidiByte == MIDI_MSG_SYSEX_EOF) {
    isSysexMessage = false;
    capturedSysexId = 0x00;
  }

  // send bytes (incl EOF)
  sendByteToGameboy(incomingMidiByte);
  delayMicroseconds(GB_MIDI_DELAY);
  blinkLight(0x90+2, 1);

}
