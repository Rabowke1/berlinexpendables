// Effekte & Projektile.
import { Pix, rng } from './png.js';

export function buildFx(): { name: string; pix: Pix }[] {
  const out: { name: string; pix: Pix }[] = [];
  const R = rng(42);

  // Explosion: 8 Frames 32x32
  for (let i = 0; i < 8; i++) {
    const p = new Pix(32, 32);
    const t = i / 7;
    const r = Math.round(3 + t * 12);
    if (i < 5) {
      p.circle(16, 16, r, 'orange');
      p.circle(16, 16, Math.max(1, r - 3), 'gold');
      if (i < 3) p.circle(16, 16, Math.max(1, r - 5), 'white');
      // Funken außen
      for (let k = 0; k < 6 + i * 2; k++) {
        const a = R() * Math.PI * 2, d = r + 1 + R() * 3;
        p.set(Math.round(16 + Math.cos(a) * d), Math.round(16 + Math.sin(a) * d), k % 2 ? 'gold' : 'red');
      }
    } else {
      // Rauchring
      p.circle(16, 16, r, 'slate');
      p.circle(16, 16, r - 4, 0);
      for (let k = 0; k < 20; k++) { const a = R() * Math.PI * 2, d = r - 3 + R() * 4; p.set(Math.round(16 + Math.cos(a) * d), Math.round(16 + Math.sin(a) * d), R() < 0.5 ? 'concrete' : 'ash'); }
      for (let k = 0; k < 10; k++) { const a = R() * Math.PI * 2, d = r - 2 + R() * 3; p.set(Math.round(16 + Math.cos(a) * d), Math.round(16 + Math.sin(a) * d), 0); }
    }
    out.push({ name: `explosion_${i}`, pix: p });
  }

  // Mündungsfeuer 3 Frames 8x8
  for (let i = 0; i < 3; i++) {
    const p = new Pix(8, 8);
    const s = 3 - i;
    p.rect(0, 3 - (s >> 1), 2 + s * 2, 1 + (s >> 1) * 2, 'gold');
    p.rect(0, 3, 2 + s, 1, 'white');
    p.set(1, 3 - s, 'gold'); p.set(1, 3 + s, 'gold');
    out.push({ name: `muzzle_${i}`, pix: p });
  }

  // Projektile
  const pellet = new Pix(3, 2); pellet.rect(0, 0, 3, 2, 'orange'); pellet.set(2, 0, 'gold'); out.push({ name: 'pellet', pix: pellet });
  const smg = new Pix(5, 2); smg.rect(0, 0, 5, 2, 'gold'); smg.set(4, 0, 'white'); smg.set(0, 1, 'orange'); out.push({ name: 'smgbullet', pix: smg });
  const mook = new Pix(3, 3); mook.rect(0, 0, 3, 3, 'red'); mook.set(1, 1, 'gold'); out.push({ name: 'mookbullet', pix: mook });
  const heavy = new Pix(6, 3); heavy.rect(0, 0, 6, 3, 'gold'); heavy.hline(1, 1, 5, 'white'); heavy.set(0, 0, 'orange'); heavy.set(0, 2, 'orange'); out.push({ name: 'heavybullet', pix: heavy });
  const rocket = new Pix(9, 4); rocket.rect(0, 1, 7, 2, 'ash'); rocket.rect(6, 0, 3, 4, 'red'); rocket.set(8, 1, 'gold'); rocket.set(8, 2, 'gold'); rocket.rect(0, 0, 2, 1, 'moss'); rocket.rect(0, 3, 2, 1, 'moss'); out.push({ name: 'rocket', pix: rocket });
  const laser = new Pix(8, 3); laser.rect(0, 0, 8, 3, 'neonCyan'); laser.hline(0, 1, 8, 'white'); out.push({ name: 'laserbeam', pix: laser });
  for (let i = 0; i < 2; i++) { // Bass-Welle 2 Frames
    const p = new Pix(8, 10);
    p.line(1 + i, 0, 5 + i, 4, 'neonCyan'); p.line(5 + i, 5, 1 + i, 9, 'neonCyan');
    p.line(0, 1, 3, 4, 'neonPink'); p.line(3, 5, 0, 8, 'neonPink');
    p.set(6 + i, 4, 'white'); p.set(6 + i, 5, 'white');
    out.push({ name: `wave_${i}`, pix: p });
  }
  for (let i = 0; i < 4; i++) { // Flasche rotiert
    const p = new Pix(8, 8);
    if (i % 2 === 0) { p.rect(2, 1, 3, 6, 'brown'); p.rect(3, 0, 1, 1, 'brown'); p.rect(2, 4, 3, 1, 'gold'); if (i === 2) { p.blit(p.clone(), 0, 0); } }
    else { p.rect(1, 2, 6, 3, 'brown'); p.rect(7, 3, 1, 1, 'brown'); p.rect(4, 2, 1, 3, 'gold'); }
    const q = i >= 2 ? p.flipped() : p;
    q.outline('ink');
    out.push({ name: `bottle_${i}`, pix: q });
  }
  for (let i = 0; i < 4; i++) { // Flamme
    const p = new Pix(10, 10);
    const r = 2 + i;
    p.circle(5, 5, r, i < 2 ? 'orange' : 'red'); p.circle(5, 5, Math.max(1, r - 2), i < 2 ? 'gold' : 'orange');
    if (i === 3) { p.circle(5, 5, r, 'slate'); p.circle(5, 5, r - 2, 0); }
    out.push({ name: `flame_${i}`, pix: p });
  }
  // Pommes-Granate (Willi) 7x8 – rote Tüte mit Fritten
  const gren = new Pix(7, 8); gren.rect(1, 3, 5, 5, 'red'); gren.hline(1, 3, 5, 'white'); gren.vline(2, 0, 3, 'gold'); gren.vline(4, 1, 2, 'gold'); gren.vline(3, 0, 3, 'bvgYellow'); gren.outline('ink'); out.push({ name: 'grenade', pix: gren });
  // Rauch 4 Frames
  for (let i = 0; i < 4; i++) { const p = new Pix(8, 8); p.circle(4, 4, 1 + i, i < 2 ? 'ash' : 'concrete'); if (i === 3) p.circle(4, 4, 2, 0); out.push({ name: `smoke_${i}`, pix: p }); }
  // Spielermarker (Pfeil) 7x5
  const arrow = new Pix(7, 5); arrow.rect(0, 0, 7, 2, 'white'); arrow.rect(1, 2, 5, 1, 'white'); arrow.rect(2, 3, 3, 1, 'white'); arrow.set(3, 4, 'white'); out.push({ name: 'marker', pix: arrow });
  // Ausrufezeichen (Gegner alarmiert)
  const ex = new Pix(3, 7); ex.rect(0, 0, 3, 4, 'red'); ex.rect(0, 5, 3, 2, 'red'); ex.outline('ink'); out.push({ name: 'alert', pix: ex });
  return out;
}
