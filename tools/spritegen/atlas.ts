// Shelf-Packer + TexturePacker-kompatibles JSON (Hash-Format) mit Zusatzfeldern (hand, grip, tip, animations).
import { Pix } from './png.js';

export interface PackInput { name: string; pix: Pix; extra?: Record<string, unknown> }
export interface PackedFrame { frame: { x: number; y: number; w: number; h: number }; [k: string]: unknown }
export interface Atlas {
  frames: Record<string, PackedFrame>;
  animations: Record<string, string[]>;
  meta: Record<string, unknown>;
}

export function pack(inputs: PackInput[], image: string, maxW = 512, pad = 1, meta: Record<string, unknown> = {}): { sheet: Pix; atlas: Atlas } {
  // Stabile Reihenfolge: nach Höhe absteigend, dann Name (deterministisch)
  const items = inputs.map((it, i) => ({ ...it, i })).sort((a, b) => b.pix.h - a.pix.h || a.i - b.i);
  let x = pad, y = pad, shelfH = 0, usedW = 0;
  const placed: { it: PackInput; x: number; y: number }[] = [];
  for (const it of items) {
    if (x + it.pix.w + pad > maxW) { x = pad; y += shelfH + pad; shelfH = 0; }
    placed.push({ it, x, y });
    x += it.pix.w + pad;
    shelfH = Math.max(shelfH, it.pix.h);
    usedW = Math.max(usedW, x);
  }
  const sheet = new Pix(usedW, y + shelfH + pad);
  const frames: Record<string, PackedFrame> = {};
  const animations: Record<string, string[]> = {};
  for (const { it, x: px, y: py } of placed) {
    sheet.blit(it.pix, px, py);
    frames[it.name] = { frame: { x: px, y: py, w: it.pix.w, h: it.pix.h }, ...(it.extra ?? {}) };
  }
  // Animationen: alles mit Suffix _<n> gruppieren
  for (const input of inputs) {
    const m = /^(.*)_(\d+)$/.exec(input.name);
    if (!m) continue;
    (animations[m[1]] ??= []).push(input.name);
  }
  for (const k of Object.keys(animations)) animations[k].sort((a, b) => Number(a.split('_').pop()) - Number(b.split('_').pop()));
  return { sheet, atlas: { frames, animations, meta: { app: 'berlin-expendables-spritegen', version: '1', image, format: 'RGBA8888', size: { w: sheet.w, h: sheet.h }, scale: '1', ...meta } } };
}
