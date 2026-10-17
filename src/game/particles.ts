import type { Renderer } from '../engine/renderer';
import type { Tilemap } from './tilemap';

export interface Particle { x: number; y: number; vx: number; vy: number; life: number; maxLife: number; color: string; size: number; gravity: number; bounce: boolean }

export class Particles {
  list: Particle[] = [];
  max = 1500;
  add(p: Omit<Particle, 'maxLife'>): void {
    if (this.list.length >= this.max) this.list.shift();
    this.list.push({ ...p, maxLife: p.life });
  }
  burst(x: number, y: number, n: number, colors: string[], speed: number, opts: Partial<Particle> = {}): void {
    for (let i = 0; i < n; i++) {
      const a = Math.random() * Math.PI * 2, s = speed * (0.3 + Math.random() * 0.7);
      this.add({ x, y, vx: Math.cos(a) * s, vy: Math.sin(a) * s - speed * 0.3, life: 30 + Math.random() * 40, color: colors[(Math.random() * colors.length) | 0], size: Math.random() < 0.3 ? 2 : 1, gravity: 0.2, bounce: true, ...opts });
    }
  }
  update(map: Tilemap): void {
    const l = this.list;
    for (let i = l.length - 1; i >= 0; i--) {
      const p = l[i];
      p.vy += p.gravity;
      p.x += p.vx;
      if (p.bounce && map.solidAtPx(p.x, p.y)) { p.x -= p.vx; p.vx *= -0.4; }
      p.y += p.vy;
      if (p.bounce && map.solidAtPx(p.x, p.y)) { p.y -= p.vy; p.vy *= -0.3; p.vx *= 0.7; if (Math.abs(p.vy) < 0.3) p.vy = 0; }
      if (--p.life <= 0) { l[i] = l[l.length - 1]; l.pop(); }
    }
  }
  draw(r: Renderer): void {
    for (const p of this.list) {
      if (p.life < 10 && (p.life & 1)) continue;   // Ausblinken
      r.worldRect(p.x, p.y, p.size, p.size, p.color);
    }
  }
}
