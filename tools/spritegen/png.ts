// Minimaler Pixel-Canvas + PNG-Export für den Spritegen.
import { PNG } from 'pngjs';
import { writeFileSync, mkdirSync } from 'node:fs';
import { dirname } from 'node:path';
import { PALETTE, type ColorName } from './palette.js';

export type Px = ColorName | 0; // 0 = transparent

export class Pix {
  readonly data: Px[];
  constructor(public readonly w: number, public readonly h: number, fill: Px = 0) {
    this.data = new Array(w * h).fill(fill);
  }
  get(x: number, y: number): Px {
    if (x < 0 || y < 0 || x >= this.w || y >= this.h) return 0;
    return this.data[y * this.w + x];
  }
  set(x: number, y: number, c: Px): void {
    if (x < 0 || y < 0 || x >= this.w || y >= this.h) return;
    this.data[y * this.w + x] = c;
  }
  rect(x: number, y: number, w: number, h: number, c: Px): void {
    for (let j = 0; j < h; j++) for (let i = 0; i < w; i++) this.set(x + i, y + j, c);
  }
  hline(x: number, y: number, w: number, c: Px): void { this.rect(x, y, w, 1, c); }
  vline(x: number, y: number, h: number, c: Px): void { this.rect(x, y, 1, h, c); }
  line(x0: number, y0: number, x1: number, y1: number, c: Px): void {
    const dx = Math.abs(x1 - x0), dy = -Math.abs(y1 - y0);
    const sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    let err = dx + dy;
    for (;;) {
      this.set(x0, y0, c);
      if (x0 === x1 && y0 === y1) break;
      const e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }
  circle(cx: number, cy: number, r: number, c: Px): void {
    for (let y = -r; y <= r; y++) for (let x = -r; x <= r; x++)
      if (x * x + y * y <= r * r + r * 0.5) this.set(cx + x, cy + y, c);
  }
  /** Zeichnet ein anderes Pix hinein (0 = durchsichtig). */
  blit(src: Pix, dx: number, dy: number, flipX = false): void {
    for (let y = 0; y < src.h; y++) for (let x = 0; x < src.w; x++) {
      const c = src.get(flipX ? src.w - 1 - x : x, y);
      if (c !== 0) this.set(dx + x, dy + y, c);
    }
  }
  /** 1px-Outline um alle nicht-transparenten Pixel (nur in transparente Pixel hinein). */
  outline(c: ColorName): void {
    const copy = this.data.slice();
    for (let y = 0; y < this.h; y++) for (let x = 0; x < this.w; x++) {
      if (copy[y * this.w + x] !== 0) continue;
      const n = (xx: number, yy: number) => xx >= 0 && yy >= 0 && xx < this.w && yy < this.h && copy[yy * this.w + xx] !== 0;
      if (n(x - 1, y) || n(x + 1, y) || n(x, y - 1) || n(x, y + 1)) this.data[y * this.w + x] = c;
    }
  }
  clone(): Pix { const p = new Pix(this.w, this.h); p.data.splice(0, p.data.length, ...this.data); return p; }
  flipped(): Pix { const p = new Pix(this.w, this.h); p.blit(this, 0, 0, true); return p; }
  shifted(dx: number, dy: number): Pix { const p = new Pix(this.w, this.h); p.blit(this, dx, dy); return p; }
}

export function toPNG(p: Pix, scale = 1): PNG {
  const png = new PNG({ width: p.w * scale, height: p.h * scale });
  for (let y = 0; y < p.h * scale; y++) for (let x = 0; x < p.w * scale; x++) {
    const c = p.get((x / scale) | 0, (y / scale) | 0);
    const i = (y * png.width + x) * 4;
    if (c === 0) { png.data[i] = 0; png.data[i + 1] = 0; png.data[i + 2] = 0; png.data[i + 3] = 0; continue; }
    const [r, g, b] = PALETTE[c];
    png.data[i] = r; png.data[i + 1] = g; png.data[i + 2] = b; png.data[i + 3] = 255;
  }
  return png;
}

export function savePNG(path: string, p: Pix, scale = 1): void {
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(path, PNG.sync.write(toPNG(p, scale)));
}

export function saveJSON(path: string, obj: unknown): void {
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(path, JSON.stringify(obj, null, 1));
}

/** Deterministischer Zufall (mulberry32) für reproduzierbare Texturen. */
export function rng(seed: number): () => number {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}
