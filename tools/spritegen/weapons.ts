// Waffen als eigene Sprites (3 Ausrichtungen), mit Griff- und Mündungsanker.
// Signaturwaffen der Bros + Metal-Slug-artige Pickups (H, R, F, L ...).
import { Pix } from './png.js';
import type { ColorName } from './palette.js';
import type { Aim } from './humanoid.js';

export type WeaponKind =
  | 'shotgun' | 'smg' | 'bass' | 'bottle' | 'pistol' | 'rifle'
  | 'heavy' | 'rocket' | 'flame' | 'laser';

export interface WeaponSprite { pix: Pix; grip: [number, number]; tip: [number, number] }

export function weaponPix(kind: WeaponKind, c: ColorName, accent: ColorName): WeaponSprite {
  switch (kind) {
    case 'shotgun': {           // Ketchup-Pumpgun (Willi)
      const p = new Pix(11, 4);
      p.rect(0, 1, 11, 2, 'slate'); p.hline(2, 1, 9, 'ash');
      p.rect(0, 0, 3, 3, 'brown');
      p.rect(4, 3, 3, 1, c);
      p.set(10, 1, accent); p.set(10, 2, accent);
      return { pix: p, grip: [2, 2], tip: [11, 1] };
    }
    case 'smg': {               // Entwerter-MP (Berti)
      const p = new Pix(8, 5);
      p.rect(0, 1, 8, 2, 'slate'); p.hline(1, 1, 6, 'ash');
      p.rect(2, 3, 2, 2, c); p.set(0, 0, 'brown');
      return { pix: p, grip: [1, 2], tip: [8, 1] };
    }
    case 'rifle': {
      const p = new Pix(13, 4);
      p.rect(0, 0, 4, 3, 'brown'); p.rect(3, 1, 10, 2, 'slate'); p.hline(4, 1, 8, 'ash'); p.rect(6, 3, 2, 1, c);
      return { pix: p, grip: [3, 2], tip: [13, 1] };
    }
    case 'pistol': {            // Mook-Standard
      const p = new Pix(5, 4);
      p.rect(0, 0, 5, 2, 'slate'); p.hline(1, 0, 3, 'ash'); p.rect(0, 2, 2, 2, 'ink');
      return { pix: p, grip: [1, 2], tip: [5, 0] };
    }
    case 'bottle': {            // Pfandflasche (Sven)
      const p = new Pix(3, 7);
      p.rect(0, 2, 3, 5, c); p.rect(1, 0, 1, 2, c);
      p.set(1, 3, 'glass'); p.set(1, 4, 'glass'); p.rect(0, 4, 3, 1, accent);
      return { pix: p, grip: [1, 5], tip: [1, 0] };
    }
    case 'bass': {              // Bass-Kanone (Doro)
      const p = new Pix(11, 6);
      p.rect(0, 1, 9, 4, c); p.hline(1, 1, 7, 'ash');
      p.rect(8, 0, 3, 6, 'navy'); p.rect(9, 1, 1, 4, accent);
      p.set(10, 2, 'neonCyan'); p.set(10, 3, 'neonCyan'); p.rect(2, 5, 3, 1, 'ink');
      return { pix: p, grip: [2, 3], tip: [11, 2] };
    }
    case 'heavy': {             // "H" – Heavy Machine Gun (Pickup)
      const p = new Pix(14, 6);
      p.rect(0, 1, 3, 4, 'brown');
      p.rect(2, 1, 12, 3, 'steelDark'); p.hline(3, 1, 10, 'steel');
      p.rect(5, 4, 3, 2, 'slate'); p.rect(12, 0, 2, 5, 'slate'); p.set(13, 2, 'gold');
      return { pix: p, grip: [3, 3], tip: [14, 2] };
    }
    case 'rocket': {            // "R" – Raketenwerfer (Pickup)
      const p = new Pix(14, 5);
      p.rect(0, 0, 14, 4, 'moss'); p.hline(1, 0, 12, 'mossLight'); p.rect(0, 1, 2, 2, 'ink');
      p.rect(11, 0, 3, 4, 'red'); p.rect(4, 4, 3, 1, 'brown');
      return { pix: p, grip: [5, 3], tip: [14, 1] };
    }
    case 'flame': {             // "F" – Flammenwerfer (Pickup)
      const p = new Pix(12, 6);
      p.rect(0, 1, 9, 3, 'orange'); p.hline(1, 1, 7, 'gold'); p.rect(0, 4, 4, 2, 'red');
      p.rect(8, 0, 4, 5, 'slate'); p.set(11, 2, 'red');
      return { pix: p, grip: [2, 3], tip: [12, 2] };
    }
    case 'laser': {             // "L" – Laser (Pickup)
      const p = new Pix(13, 5);
      p.rect(0, 1, 13, 3, 'fog'); p.hline(1, 1, 11, 'white');
      p.rect(3, 0, 6, 1, 'neonCyan'); p.rect(0, 4, 3, 1, 'slate'); p.set(12, 2, 'neonCyan');
      return { pix: p, grip: [2, 3], tip: [13, 2] };
    }
  }
}

export function rotateWeapon(w: WeaponSprite, aim: Aim): WeaponSprite {
  if (aim === 'h') return w;
  const src = w.pix;
  const out = new Pix(src.h, src.w);
  const map = (x: number, y: number): [number, number] => (aim === 'u' ? [y, src.w - 1 - x] : [src.h - 1 - y, x]);
  for (let y = 0; y < src.h; y++) for (let x = 0; x < src.w; x++) { const [nx, ny] = map(x, y); out.set(nx, ny, src.get(x, y)); }
  return { pix: out, grip: map(...w.grip), tip: map(...w.tip) };
}

export const WEAPON_DEFS: { kind: WeaponKind; color: ColorName; accent: ColorName }[] = [
  { kind: 'shotgun', color: 'red', accent: 'gold' },
  { kind: 'smg', color: 'bvgYellow', accent: 'bvgYellow' },
  { kind: 'bass', color: 'purple', accent: 'neonPink' },
  { kind: 'bottle', color: 'brown', accent: 'gold' },
  { kind: 'pistol', color: 'slate', accent: 'slate' },
  { kind: 'rifle', color: 'slate', accent: 'slate' },
  { kind: 'heavy', color: 'steelDark', accent: 'gold' },
  { kind: 'rocket', color: 'moss', accent: 'red' },
  { kind: 'flame', color: 'orange', accent: 'red' },
  { kind: 'laser', color: 'fog', accent: 'neonCyan' },
];
