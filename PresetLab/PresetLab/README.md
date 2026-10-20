**PRESETLAB**
Free instrument and amp/effects sound library for every major DAW

Available as VST3 (Windows + Mac), AU (Mac) and a standalone app

Build your own guitar, bass, piano, synth, drum and other sounds, run them (or your own guitar, bass or mic) through
stomp boxes, an amp, a cab and rack effects, and write chord progressions with one click. The workflow is modelled on
AmpliTube 5's signal chain, flattened onto one screen so nothing is more than one click away.

**FEATURES**

- 15 instruments, all synthesized (no sample downloads): Acoustic Guitar, Electric Guitar, Bass Guitar, Synth Bass,
  Grand Piano, Electric Piano, Organ, Synth, Pad, Strings, Brass, Bell / Pluck, Mallets, Flute / Wind, Drums
- 6 drum kits on the General MIDI map: Studio, Rock, 808, 909, Lo-Fi, Electro
- Sound controls per instrument: tone and character knobs, ADSR, filter cutoff / resonance / envelope, detune, vibrato,
  stereo width, octave, fine tune, velocity sensitivity
- 6 synth waveforms: Saw, Square, Triangle, Sine, Supersaw, Pulse
- 224 factory presets in 15 categories, plus your own (search, category chips, prev/next, save, open folder)
- Signal chain: Source > 3 Stomps > Amp > Cab > 3 Rack effects, each block with its own on/off
- 23 effects for any stomp or rack slot: Overdrive, Distortion, Fuzz, Compressor, Noise Gate, Chorus, Flanger, Phaser,
  Tremolo, Auto-Wah, 3-Band EQ, Delay, Ping-Pong, Tape Echo, Room / Hall / Plate / Spring Reverb, Rotary Speaker,
  Auto Filter, Bit Crusher, Stereo Widener, Lo-Fi Tape
- 9 amps: Clean American, Jazz Clean, British Crunch, Hot Rod Lead, Modern Metal, Bass Tube, Bass Modern, Keys Hi-Fi,
  Lo-Fi Practice (Gain, Bass, Middle, Treble, Presence, Master)
- 10 cabs with Mic Position and Room: 1x12 Open Back, 2x12 Blue, 4x12 British, 4x12 Modern, Tweed Combo, 1x15 Bass,
  4x10 Bass, 8x10 Bass, Full-Range Keys, Vintage Radio
- Live Input mode: process your guitar, bass or mic in real time, with a built-in tuner
- Instrument, Live Input or Both at once
- Auto Chord Generator:
  - Follow Key: play one note, hear the right chord for the key (C major: D plays Dm)
  - Fixed: every note plays the chord type you pick (16 types, Major to Aug)
  - Triads, 7ths or 9ths; Close, Open, Spread, Drop 2 and Guitar voicings; inversions; extra bass note
  - Strum (down, up, alternate, random) and humanize
  - 7 in-key chord pads you can click or trigger
  - Generate: makes a progression in your key (click a pad to change a chord); Play loops it, one chord per bar at the song tempo
  - Drag MIDI: drag the progression straight onto a MIDI track
- Undo / Redo
- Resizable window
- Every control is host-automatable

**FORMATS / DAWS**

- PresetLab (instrument, for MIDI tracks): VST3 + AU + standalone app
- PresetLab FX (effect, for audio tracks: live guitar / bass / mic, or any recorded audio): VST3 + AU
- Works in Ableton Live, FL Studio, Cubase, Studio One, Reaper, Bitwig, Logic Pro, GarageBand and other VST3 / AU hosts
- Pro Tools (AAX) is not included: it needs Avid's AAX SDK and a paid PACE signing account

**INSTALLING**

The GitHub Actions build makes one installer per platform:

- Windows: run PresetLab-Setup.exe. It puts PresetLab.vst3 and PresetLab FX.vst3 in C:\Program Files\Common Files\VST3
  and the app in Program Files with Start menu and desktop shortcuts. Uninstall from Windows Settings > Apps.
- Mac: open PresetLab-Mac-Installer.pkg. It installs both VST3s, both AUs and the app.
- Then rescan plug-ins in your DAW (Ableton: Settings > Plug-Ins > Rescan).

**WORKFLOW**

1. Pick a preset on the left (or click the preset name at the top for a menu of every preset).
2. Click any block in the chain strip to edit it. Click its light to turn it on or off. Right-click a block to switch
   its model or effect.
3. Source: pick an instrument tile and shape it with the knobs on the right.
4. Stomp / Rack: pick an effect from the grid; the pedal or rack unit shows its knobs.
5. Amp / Cab: pick a model, set the knobs.
6. Save: your sound goes into the User category (Documents\PresetLab Presets).

**ABLETON (MIDI)**

Drag PresetLab onto a MIDI track and play your keyboard, or click the on-screen keys.
For chords, turn on CHORDS, pick a key and scale, then play single notes. Generate makes a progression; Play loops it at
Ableton's tempo; Drag MIDI drops it into a clip.

**ABLETON (LIVE GUITAR / BASS / MIC)**

1. Plug into your audio interface and pick it in Ableton (Settings > Audio).
2. Make an audio track, set its input to your guitar's channel and Monitor to In.
3. Drag PresetLab FX onto the track and pick a Live Guitar, Live Bass or Live Vocal & Mic preset.
4. Arm the track and record. Keep the Live buffer size at 128 or 256 samples for low latency.

The FX version also takes MIDI, so you can turn on Both and play the instrument over your live input.

**STANDALONE APP**

Open PresetLab, go to Options > Audio/MIDI Settings, pick your interface and MIDI keyboard. Click Live Input in the top
bar to play your guitar through the chain, or Both to layer it with the instrument.

**BUILDING**

Push the repository to GitHub. The workflow in .github/workflows/main.yml builds Windows and Mac installers on every
push. Download them from the Actions tab > latest run > Artifacts.
Locally: cmake -S PresetLab -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release
(JUCE 8.0.4 is downloaded automatically.)
