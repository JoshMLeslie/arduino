// via https://github.com/robsoncouto/arduino-songs/
#include "notes.h"




void play_song(int melody[], size_t note_sz, int tempo = 144, byte pin = 11) {
  int divider = 0, noteDuration = 0;

  // note_sz gives the number of bytes, each int value is composed of two bytes (16 bits)
  // there are two values per note (pitch and duration), so for each note there are four bytes

  // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
  int wholenote = (60000 * 4) / tempo;

  // iterate over the notes of the melody. 
  // Remember, the array is twice the number of notes (notes + durations)
  for (unsigned int thisNote = 0; thisNote < note_sz * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause for clarity
    tone(pin, melody[thisNote], noteDuration*0.9);

    // Wait for the specific duration before playing the next note.
    delay(noteDuration);
    
    // stop the waveform generation before the next note.
    noTone(pin);
  }
}