// Winziger Retro-Synth (WebAudio) – keine Sample-Dateien nötig, passt zum Pixel-Look.
export type SfxName = 'shoot' | 'shotgun' | 'smg' | 'bass' | 'throw' | 'explosion' | 'jump' | 'land' | 'hit' | 'die' | 'pickup' | 'melee' | 'rescue' | 'dig' | 'alert';

export class Sfx {
  private ctx: AudioContext | null = null;
  private noise: AudioBuffer | null = null;
  private lastPlay = new Map<SfxName, number>();
  muted = false;

  unlock(): void {
    if (this.ctx) { if (this.ctx.state === 'suspended') void this.ctx.resume(); return; }
    const AC = window.AudioContext ?? (window as any).webkitAudioContext;
    if (!AC) return;
    this.ctx = new AC();
    const len = this.ctx.sampleRate;
    this.noise = this.ctx.createBuffer(1, len, this.ctx.sampleRate);
    const d = this.noise.getChannelData(0);
    for (let i = 0; i < len; i++) d[i] = Math.random() * 2 - 1;
  }

  play(name: SfxName, vol = 1): void {
    const c = this.ctx; if (!c || this.muted || c.state !== 'running') return;
    const now = c.currentTime;
    const last = this.lastPlay.get(name) ?? -1;
    if (now - last < 0.03) return;
    this.lastPlay.set(name, now);
    const out = c.createGain(); out.gain.value = 0.18 * vol; out.connect(c.destination);
    const tone = (type: OscillatorType, f0: number, f1: number, dur: number, g = 1) => {
      const o = c.createOscillator(), g1 = c.createGain();
      o.type = type; o.frequency.setValueAtTime(f0, now); o.frequency.exponentialRampToValueAtTime(Math.max(20, f1), now + dur);
      g1.gain.setValueAtTime(g, now); g1.gain.exponentialRampToValueAtTime(0.001, now + dur);
      o.connect(g1).connect(out); o.start(now); o.stop(now + dur + 0.02);
    };
    const hiss = (dur: number, lp: number, hp = 0, g = 1) => {
      if (!this.noise) return;
      const s = c.createBufferSource(); s.buffer = this.noise;
      const f = c.createBiquadFilter(); f.type = 'lowpass'; f.frequency.setValueAtTime(lp, now); f.frequency.exponentialRampToValueAtTime(Math.max(50, lp / 8), now + dur);
      const h = c.createBiquadFilter(); h.type = 'highpass'; h.frequency.value = hp;
      const g1 = c.createGain(); g1.gain.setValueAtTime(g, now); g1.gain.exponentialRampToValueAtTime(0.001, now + dur);
      s.connect(f).connect(h).connect(g1).connect(out); s.start(now); s.stop(now + dur + 0.02);
    };
    switch (name) {
      case 'shoot': hiss(0.07, 3000, 400, 0.8); tone('square', 600, 150, 0.06, 0.4); break;
      case 'smg': hiss(0.05, 4000, 600, 0.6); tone('square', 900, 200, 0.04, 0.3); break;
      case 'shotgun': hiss(0.18, 1800, 100, 1); tone('triangle', 200, 40, 0.15, 0.8); break;
      case 'bass': tone('sawtooth', 90, 30, 0.22, 1); tone('square', 45, 20, 0.22, 0.5); break;
      case 'throw': tone('sine', 300, 700, 0.12, 0.5); break;
      case 'explosion': hiss(0.6, 900, 0, 1.4); tone('sine', 120, 25, 0.5, 1.2); break;
      case 'jump': tone('square', 220, 520, 0.12, 0.35); break;
      case 'land': hiss(0.05, 500, 0, 0.5); break;
      case 'hit': hiss(0.08, 1200, 200, 0.8); tone('square', 180, 60, 0.08, 0.5); break;
      case 'die': tone('sawtooth', 400, 60, 0.35, 0.7); hiss(0.25, 1500, 0, 0.6); break;
      case 'pickup': tone('square', 660, 660, 0.07, 0.4); tone('square', 990, 990, 0.1, 0.4); break;
      case 'melee': hiss(0.06, 6000, 1500, 0.7); break;
      case 'rescue': tone('square', 523, 523, 0.08, 0.4); tone('square', 659, 659, 0.08, 0.4); tone('square', 784, 1046, 0.25, 0.5); break;
      case 'dig': hiss(0.06, 700, 0, 0.6); break;
      case 'alert': tone('square', 1200, 1200, 0.05, 0.3); break;
    }
  }
}
