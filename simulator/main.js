const VOICES = [
  'bass',
  'tenor',
  'alto',
  'treble',
];

(async function main() {
  const scores = await Promise.all(VOICES.map(voice => fetch(`scores/${voice}.txt`).then(res => res.text())));
  let audioContext = null;

  const playButton = document.getElementById('play');
  playButton.addEventListener('click', async () => {
    if (audioContext) {
      if (audioContext.state === 'running') {
        audioContext.suspend();
        playButton.textContent = 'play';
        return;
      } else if (audioContext.state === 'suspended') {
        audioContext.resume();
        playButton.textContent = 'pause';
        return;
      }
    }

    audioContext = new AudioContext();
    await audioContext.audioWorklet.addModule('playback-processor.js');
    const playbackNode = new AudioWorkletNode(audioContext, 'playback-processor', {
      outputChannelCount: [2],
    });

    // bind the note elements to their voices
    const noteEls = VOICES.map(voice => document.getElementById(voice));
    const timeEl = document.getElementById('time');
    let duration = '';

    playbackNode.port.onmessage = (event) => {
      const message = JSON.parse(event.data);
      switch (message.type) {
        case 'notes':
          Object.entries(message.notes).forEach(([voice, note]) => {
            noteEls[VOICES.indexOf(voice)].textContent = note;
          });
          break;
        case 'duration':
          duration = message.duration;
          break;
        case 'playbackTime':
          timeEl.textContent = `${message.playbackTime} / ${duration}`;
      }
    };
    
    // Create a low pass filter
    const filterNode = audioContext.createBiquadFilter();
    filterNode.type = 'lowpass';
    filterNode.frequency.value = 2000; // Set the cutoff frequency to 2kHz
    
    // Connect the nodes
    playbackNode.connect(filterNode);
    filterNode.connect(audioContext.destination);

    scores.forEach((score, i) => {
      playbackNode.port.postMessage(JSON.stringify({ type: 'score', voice: VOICES[i], score }));
    });

    playbackNode.port.postMessage(JSON.stringify({ type: 'loaded' }));
    
    playButton.textContent = 'pause';
  });
})();