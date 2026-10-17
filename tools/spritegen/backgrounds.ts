// Parallax-Hintergründe: Himmel, Skyline mit Fernsehturm, nahe Häuserzeile.
import { Pix, rng } from './png.js';
import type { ColorName } from './palette.js';

export function buildSky(w = 480, h = 270): Pix {
  const p = new Pix(w, h, 'night');
  const R = rng(7);
  // Dither-Gradient night -> navy -> slate (+ orangenes Glühen am Horizont)
  const stops: { y: number; c: ColorName }[] = [{ y: 0, c: 'night' }, { y: 130, c: 'night' }, { y: 200, c: 'navy' }, { y: 250, c: 'slate' }, { y: 270, c: 'slate' }];
  for (let y = 0; y < h; y++) {
    let a = stops[0], b = stops[1];
    for (let i = 0; i < stops.length - 1; i++) if (y >= stops[i].y && y < stops[i + 1].y) { a = stops[i]; b = stops[i + 1]; }
    const t = (y - a.y) / Math.max(1, b.y - a.y);
    for (let x = 0; x < w; x++) {
      const bayer = ((x & 1) + (y & 1) * 2 + ((x >> 1) & 1) * 1.5) / 4;  // grobes Ordered-Dither
      p.set(x, y, t > bayer ? b.c : a.c);
    }
  }
  for (let y = 236; y < 270; y++) for (let x = 0; x < w; x++) if (R() < (y - 236) / 120) p.set(x, y, R() < 0.3 ? 'orange' : 'slate');
  // Sterne
  for (let i = 0; i < 90; i++) { const x = (R() * w) | 0, y = (R() * 150) | 0; p.set(x, y, R() < 0.2 ? 'white' : 'ash'); }
  // Mond
  p.circle(400, 40, 11, 'cream'); p.circle(396, 37, 9, 'cream'); p.set(403, 44, 'beige'); p.set(398, 35, 'beige'); p.set(405, 38, 'beige');
  return p;
}

/** Ferne Skyline (horizontal kachelbar), Silhouette in navy. */
export function buildSkyline(w = 480, h = 140): Pix {
  const p = new Pix(w, h);
  const R = rng(99);
  let x = 0;
  while (x < w) {
    const bw = 12 + ((R() * 30) | 0), bh = 20 + ((R() * 60) | 0);
    p.rect(x, h - bh, bw, bh, 'navy');
    for (let wy = h - bh + 3; wy < h - 4; wy += 4) for (let wx = x + 2; wx < x + bw - 2; wx += 4) if (R() < 0.12) p.set(wx, wy, R() < 0.7 ? 'gold' : 'orange');
    if (R() < 0.3) p.rect(x + 2, h - bh - 3, 2, 3, 'navy');   // Schornstein / Antenne
    x += bw + (R() < 0.3 ? 2 : 0);
  }
  // Fernsehturm
  const tx = 300;
  p.rect(tx - 3, 30, 6, h - 30, 'navy'); p.rect(tx - 5, h - 20, 10, 20, 'navy');
  p.circle(tx, 36, 10, 'navy'); p.rect(tx - 1, 4, 2, 24, 'navy'); p.set(tx, 2, 'red'); p.set(tx, 3, 'red');
  for (let a = 0; a < 8; a++) p.set(Math.round(tx + Math.cos(a) * 6), Math.round(36 + Math.sin(a) * 6), a % 2 ? 'gold' : 'ash');
  p.rect(tx - 8, 36, 16, 1, 'slate');
  // Baukran (Berlin!)
  p.rect(60, 50, 2, h - 50, 'navy'); p.rect(30, 50, 70, 2, 'navy'); p.rect(96, 52, 1, 20, 'navy'); p.set(96, 72, 'red'); p.set(61, 48, 'red');
  // Kirchturm
  p.rect(180, 70, 10, h - 70, 'navy'); p.line(180, 70, 185, 50, 'navy'); p.line(190, 70, 185, 50, 'navy'); p.rect(184, 70, 3, 4, 'gold');
  return p;
}

/** Nahe Häuserzeile (kachelbar), Altbau-Fassaden mit Fenstern, Spätis, Graffiti. */
export function buildMid(w = 480, h = 120): Pix {
  const p = new Pix(w, h);
  const R = rng(5);
  let x = 0;
  const facades: ColorName[] = ['slate', 'concrete', 'brickDark', 'navy'];
  while (x < w) {
    const bw = 40 + ((R() * 40) | 0), bh = 50 + ((R() * 60) | 0);
    const c = facades[(R() * facades.length) | 0];
    p.rect(x, h - bh, bw, bh, c);
    p.hline(x, h - bh, bw, 'ash');
    for (let wy = h - bh + 6; wy < h - 14; wy += 10) for (let wx = x + 4; wx < x + bw - 6; wx += 9) {
      const lit = R() < 0.45;
      p.rect(wx, wy, 5, 6, lit ? 'gold' : 'night');
      if (lit) { p.set(wx + 1, wy + 1, 'white'); p.vline(wx + 2, wy, 6, 'slate'); }
    }
    // Erdgeschoss: Späti / Laden mit Neon
    const neon: ColorName = R() < 0.5 ? 'neonPink' : 'neonCyan';
    p.rect(x + 3, h - 12, bw - 6, 12, 'night');
    p.rect(x + 5, h - 11, bw - 10, 2, neon);
    p.rect(x + 6, h - 8, 6, 8, 'glass'); p.rect(x + bw - 12, h - 8, 6, 8, 'orange');
    if (R() < 0.5) { p.rect(x + 14, h - 6, 10, 3, 'neonPink'); p.rect(x + 16, h - 5, 6, 1, 'white'); } // Graffiti
    x += bw;
  }
  // Oberleitung / Kabel
  for (let i = 0; i < w; i += 2) p.set(i, 8 + Math.round(Math.sin(i / 40) * 3), 'navy');
  return p;
}
