const CPU_SPEED_MULTIPLIER = 0.94;  // 2025-01-04 Peter Samson mentioned the CHM PDP-1 CPU is 6% slower than spec.
const BPM = Math.floor(115 * CPU_SPEED_MULTIPLIER);  // 108 BPM is the target, 115 acounts for CPU speed discrepancy.
const VOLUME = 0.25;

class ScorePlayback {
  constructor(score, bpm, beatsPerMeasure = 4) {
    this.score = score;
    this.bpm = bpm;
    this.notes = this.parseScore(score);
    this.beatsPerMeasure = beatsPerMeasure;
  }

  get measures() {
    return this.notes.reduce((acc, { duration }) => acc + 1 / duration, 0);
  }

  get duration() {
    return this.measures * this.beatsPerMeasure * 60 / this.bpm;
  }

  parseScore(score) {
    // each note is separate by a space or newline. Each note is in the format "{note}t{duration}". {note} has a value
    // such as c3, d4, etc. {duration} is is a fraction of a whole note: 1, 2, 4, 8, 16, 32, or 64. Anything else is
    // is ignored.
    let notes = [];
    let noteStrings = score.split(/[ \n]/g);
    for (let noteString of noteStrings) {
      let note = noteString.match(/^[a-gA-GrRr][#b]?(?:\d+)?/);
      let duration = noteString.match(/t\d+$/);
      if (note && duration) {
        notes.push({
          note: note[0],
          duration: parseInt(duration[0].substring(1), 10)
        });
      }
    }

    return notes;
  }

  getNoteAtTime(time) {
    if (time < 0) {
      return null;
    }

    const secondsPerBeat = 60 / this.bpm;
    const wholeNoteDurationSeconds = secondsPerBeat * this.beatsPerMeasure;

    let timeSoFar = 0;
    for (let i = 0; i < this.notes.length; i++) {
      const { note, duration } = this.notes[i];
      const noteDurationSeconds = wholeNoteDurationSeconds / duration;
      if (time >= timeSoFar && time < timeSoFar + noteDurationSeconds) {
        return note;
      }
      timeSoFar += noteDurationSeconds;
    }

    return null;
  }
}



const NOTES = [ 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B' ];

class SquareWaveGenerator {
  phaseAccumulator = 0;
  phaseSteps = {};

  constructor(sampleRate = 44100, accumaltorBits = 18) {
    this.sampleRate = sampleRate;
    this.accumaltorBits = accumaltorBits;
    this.calculateBasePhaseSteps();
  }

  calculateBasePhaseSteps() {
    // pre-calculating the phase step for each note from C1 to B1
    const baseFrequency = 32.7032; // C1
    for (let i = 0; i < 12; i++) {
      const frequency = baseFrequency * Math.pow(2, i / 12);
      const phaseStep = frequency * ((1 << this.accumaltorBits) / this.sampleRate);
      this.phaseSteps[NOTES[i]] = phaseStep;
    }
  }

  calculatePhaseStep(note) {
    const octave = parseInt(note[note.length - 1], 10);
    const baseNote = note.substring(0, note.length - 1);
    return (this.phaseSteps[baseNote] << (octave - 1)) * CPU_SPEED_MULTIPLIER;
  }

  nextSample(note) {
    if (note.toLowerCase() === 'r') {
      return 0;
    }

    const phaseStep = this.calculatePhaseStep(note);
    this.phaseAccumulator += phaseStep;
    return ((this.phaseAccumulator >> (this.accumaltorBits - 1)) & 1) ? 1 : -1;
  }
}



class PlaybackProcessor extends AudioWorkletProcessor {
  scorePlayers = {};
  loaded = false;
  lastVoiceNotes = {};
  lastPlaybackTime = '';

  constructor() {
    super();

    this.port.onmessage = (event) => {
      const message = JSON.parse(event.data);
      switch (message.type) {
        case 'score':
          this.scorePlayers[message.voice] = {
            score: new ScorePlayback(message.score, BPM),
            generator: new SquareWaveGenerator(globalThis.sampleRate),
          };
          break;
        case 'loaded':
          this.port.postMessage(
            JSON.stringify({
              type: 'duration',
              duration: this.formatTime(this.scorePlayers.bass.score.duration),
            }
          ));

          this.loaded = true;
          this.startTime = globalThis.currentTime;
          break;
      }
    }
  }

  process(inputs, outputs, parameters) {
    if (!this.loaded) {
      return true;
    }

    const { bass, tenor, alto, treble } = this.scorePlayers;
    const playbackTime = (globalThis.currentTime - this.startTime) % this.scorePlayers.bass.score.duration;

    const left = outputs[0][0];
    const right = outputs[0][1];

    const changedNotes = {};

    for (let i = 0; i < left.length; i++) {
      const timeInLoop = (playbackTime + i / globalThis.sampleRate) % this.scorePlayers.bass.score.duration;

      const noteBass = bass.score.getNoteAtTime(timeInLoop);
      const noteTenor = tenor.score.getNoteAtTime(timeInLoop);
      const noteAlto = alto.score.getNoteAtTime(timeInLoop);
      const noteTreble = treble.score.getNoteAtTime(timeInLoop);
      
      if (noteBass !== this.lastVoiceNotes.bass) {
        changedNotes.bass = noteBass;
      }
      if (noteTenor !== this.lastVoiceNotes.tenor) {
        changedNotes.tenor = noteTenor;
      }
      if (noteAlto !== this.lastVoiceNotes.alto) {
        changedNotes.alto = noteAlto;
      }
      if (noteTreble !== this.lastVoiceNotes.treble) {
        changedNotes.treble = noteTreble;
      }

      // left[i] = VOLUME * (tenor.generator.nextSample(noteTenor) + treble.generator.nextSample(noteTreble));
      // right[i] = VOLUME * (bass.generator.nextSample(noteBass) + alto.generator.nextSample(noteAlto));

      left[i] = VOLUME * (tenor.generator.nextSample(noteAlto) + treble.generator.nextSample(noteTreble));
      right[i] = VOLUME * (bass.generator.nextSample(noteBass) + alto.generator.nextSample(noteTenor));
    }

    if (Object.keys(changedNotes).length > 0) {
      this.port.postMessage(JSON.stringify({ type: 'notes', notes: changedNotes }));
    }

    const formattedPlaybackTime = this.formatTime(playbackTime);
    if (formattedPlaybackTime !== this.lastPlaybackTime) {
      this.port.postMessage(JSON.stringify({ type: 'playbackTime', playbackTime: formattedPlaybackTime }));
      this.lastPlaybackTime = formattedPlaybackTime;
    }

    return true;
  }

  formatTime(seconds) {
    const minutes = Math.floor(seconds / 60);
    const remainingSeconds = Math.floor(seconds % 60);
    return `${minutes.toString().padStart(2, '0')}:${remainingSeconds.toString().padStart(2, '0')}`;
  }
}

registerProcessor('playback-processor', PlaybackProcessor);