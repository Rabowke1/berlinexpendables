// 16x16-Tileset, Props und Items.
import { Pix, rng } from './png.js';

export const TILE = 16;

export function buildTiles(): { name: string; pix: Pix }[] {
  const out: { name: string; pix: Pix }[] = [];
  const R = rng(1337);

  // Erde
  const dirt = new Pix(16, 16, 'soil');
  for (let i = 0; i < 14; i++) dirt.set((R() * 16) | 0, (R() * 16) | 0, 'soilDark');
  for (let i = 0; i < 5; i++) dirt.set((R() * 16) | 0, (R() * 16) | 0, 'soilLight');
  out.push({ name: 'dirt', pix: dirt });

  // Erde mit Gras-Kante
  const dirtTop = dirt.clone();
  dirtTop.rect(0, 0, 16, 3, 'moss');
  dirtTop.hline(0, 0, 16, 'mossLight');
  for (let x = 0; x < 16; x += 3) dirtTop.set(x + ((R() * 2) | 0), -1 + 1, 'mossLight');
  for (let x = 1; x < 16; x += 4) dirtTop.set(x, 3, 'mossDark');
  dirtTop.set(2, 0, 0); dirtTop.set(9, 0, 0); dirtTop.set(3, -1, 0); // kleine Grashalme-Lücken
  dirtTop.set(5, 0, 'mossLight'); dirtTop.set(12, 0, 'mossLight');
  dirtTop.hline(0, 3, 16, 'soilDark');
  out.push({ name: 'dirt_top', pix: dirtTop });

  // Ziegel
  const brick = new Pix(16, 16, 'brick');
  for (let row = 0; row < 4; row++) {
    const y = row * 4;
    brick.hline(0, y + 3, 16, 'brickDark');
    const off = row % 2 ? 0 : 8;
    brick.vline((off + 3) % 16, y, 3, 'brickDark');
    brick.vline((off + 11) % 16, y, 3, 'brickDark');
    brick.hline(0, y, 16, 'brickLight');
    brick.set((off + 3) % 16, y, 'brickDark'); brick.set((off + 11) % 16, y, 'brickDark');
  }
  for (let i = 0; i < 6; i++) brick.set((R() * 16) | 0, (R() * 16) | 0, 'brickDark');
  out.push({ name: 'brick', pix: brick });

  // Ziegel mit Betonkrone
  const brickTop = brick.clone();
  brickTop.rect(0, 0, 16, 3, 'concrete'); brickTop.hline(0, 0, 16, 'ash'); brickTop.hline(0, 2, 16, 'slate');
  out.push({ name: 'brick_top', pix: brickTop });

  // Stahl (unzerstörbar)
  const steel = new Pix(16, 16, 'steelDark');
  steel.rect(1, 1, 14, 14, 'steelDark');
  steel.hline(0, 0, 16, 'steel'); steel.vline(0, 0, 16, 'steel');
  steel.hline(0, 15, 16, 'navy'); steel.vline(15, 0, 16, 'navy');
  for (const [x, y] of [[3, 3], [12, 3], [3, 12], [12, 12]]) { steel.set(x, y, 'ash'); steel.set(x + 1, y + 1, 'navy'); }
  steel.line(5, 10, 10, 5, 'steel');
  out.push({ name: 'steel', pix: steel });

  // Beton
  const concrete = new Pix(16, 16, 'concrete');
  for (let i = 0; i < 10; i++) concrete.set((R() * 16) | 0, (R() * 16) | 0, 'slate');
  for (let i = 0; i < 6; i++) concrete.set((R() * 16) | 0, (R() * 16) | 0, 'ash');
  concrete.hline(0, 0, 16, 'ash'); concrete.hline(0, 15, 16, 'slate');
  out.push({ name: 'concrete', pix: concrete });

  // Plattform (einseitig durchlässig): Metallgitter, 5px hoch
  const plat = new Pix(16, 16);
  plat.rect(0, 0, 16, 5, 'slate');
  plat.hline(0, 0, 16, 'ash');
  for (let x = 1; x < 16; x += 3) plat.set(x, 2, 'navy');
  plat.hline(0, 4, 16, 'ink');
  out.push({ name: 'platform', pix: plat });

  // Risse (Overlay, 3 Stufen)
  for (let lvl = 1; lvl <= 3; lvl++) {
    const c = new Pix(16, 16);
    const r2 = rng(100 + lvl);
    for (let i = 0; i < lvl * 2; i++) {
      let x = (r2() * 16) | 0, y = (r2() * 16) | 0;
      for (let s = 0; s < 4 + lvl * 2; s++) { c.set(x, y, 'ink'); x += r2() < 0.5 ? 1 : -1; y += r2() < 0.6 ? 1 : 0; }
    }
    out.push({ name: `crack_${lvl}`, pix: c });
  }

  // Hintergrund-Wandtiles (nicht solide)
  const plaster = new Pix(16, 16, 'beige');
  for (let i = 0; i < 8; i++) plaster.set((R() * 16) | 0, (R() * 16) | 0, 'cream');
  for (let i = 0; i < 5; i++) plaster.set((R() * 16) | 0, (R() * 16) | 0, 'brown');
  out.push({ name: 'back_plaster', pix: plaster });
  const win = plaster.clone();
  win.rect(3, 2, 10, 12, 'navy'); win.rect(4, 3, 8, 10, 'night');
  win.rect(5, 4, 3, 4, 'gold'); win.rect(9, 4, 2, 4, 'glass'); win.hline(3, 14, 10, 'brown');
  out.push({ name: 'back_window', pix: win });
  const backBrick = new Pix(16, 16, 'brickDark');
  for (let row = 0; row < 4; row++) { backBrick.hline(0, row * 4 + 3, 16, 'night'); backBrick.vline((row % 2 ? 3 : 11), row * 4, 3, 'night'); }
  out.push({ name: 'back_brick', pix: backBrick });
  const graffiti = backBrick.clone();
  graffiti.rect(2, 5, 12, 6, 'neonPink'); graffiti.rect(4, 7, 8, 2, 'neonCyan'); graffiti.set(3, 6, 'white'); graffiti.set(12, 9, 'white');
  out.push({ name: 'back_graffiti', pix: graffiti });

  // Props
  const crate = new Pix(16, 16);
  crate.rect(0, 0, 16, 16, 'brown'); crate.rect(1, 1, 14, 14, 'soilLight');
  crate.rect(0, 0, 16, 2, 'brown'); crate.rect(0, 14, 16, 2, 'brown'); crate.rect(0, 0, 2, 16, 'brown'); crate.rect(14, 0, 2, 16, 'brown');
  crate.line(2, 2, 13, 13, 'brown'); crate.line(13, 2, 2, 13, 'brown'); crate.hline(1, 1, 14, 'beige');
  crate.outline('ink');
  out.push({ name: 'crate', pix: crate });

  const barrel = new Pix(16, 16);   // Gasflasche vom Grill
  barrel.rect(4, 3, 8, 13, 'red'); barrel.rect(5, 2, 6, 1, 'red'); barrel.rect(6, 0, 4, 2, 'slate');
  barrel.vline(5, 3, 12, 'brickLight'); barrel.vline(10, 4, 11, 'blood');
  barrel.rect(4, 8, 8, 2, 'white'); barrel.set(7, 8, 'ink'); barrel.set(8, 9, 'ink');
  barrel.outline('ink');
  out.push({ name: 'barrel', pix: barrel });

  // Käfig für Gefangene (16x24 → als 2 Tiles hoch)
  const cage = new Pix(16, 24);
  cage.rect(0, 0, 16, 24, 'steelDark');
  for (let x = 2; x < 16; x += 3) cage.vline(x, 1, 22, 0);
  cage.hline(0, 0, 16, 'steel'); cage.hline(0, 23, 16, 'ink'); cage.hline(0, 12, 16, 'steelDark');
  out.push({ name: 'cage', pix: cage });

  // Items (Metal-Slug-Pickups): Box mit Buchstabe
  const glyph: Record<string, string[]> = {
    H: ['#.#', '#.#', '###', '#.#', '#.#'],
    R: ['##.', '#.#', '##.', '#.#', '#.#'],
    F: ['###', '#..', '##.', '#..', '#..'],
    L: ['#..', '#..', '#..', '#..', '###'],
    B: ['##.', '#.#', '##.', '#.#', '##.'],
  };
  const itemColor: Record<string, 'gold' | 'red' | 'orange' | 'neonCyan' | 'moss'> = { H: 'gold', R: 'red', F: 'orange', L: 'neonCyan', B: 'moss' };
  for (const k of Object.keys(glyph)) {
    const it = new Pix(10, 10);
    it.rect(1, 1, 8, 8, itemColor[k]); it.hline(1, 1, 8, 'white'); it.vline(1, 1, 8, 'white');
    glyph[k].forEach((row, y) => [...row].forEach((ch, x) => { if (ch === '#') it.set(x + 3, y + 2, 'ink'); }));
    it.outline('ink');
    out.push({ name: `item_${k}`, pix: it });
  }
  // Extra-Leben: Currywurst
  const wurst = new Pix(12, 8);
  wurst.rect(1, 2, 10, 4, 'brickLight'); wurst.hline(2, 2, 8, 'orange'); wurst.rect(3, 1, 2, 1, 'red'); wurst.rect(7, 1, 2, 1, 'red');
  wurst.set(2, 6, 'bvgYellow'); wurst.set(8, 6, 'bvgYellow'); wurst.outline('ink');
  out.push({ name: 'item_life', pix: wurst });

  // Levelende: Fluchtwagen (Trabi), 40x20
  const trabi = new Pix(40, 20);
  trabi.rect(2, 8, 36, 8, 'glass'); trabi.rect(2, 8, 36, 8, 'beige');
  trabi.rect(9, 2, 20, 7, 'beige'); trabi.rect(11, 3, 7, 5, 'glass'); trabi.rect(20, 3, 7, 5, 'glass');
  trabi.hline(2, 8, 36, 'cream'); trabi.rect(0, 10, 2, 4, 'beige'); trabi.rect(38, 10, 2, 4, 'beige');
  trabi.circle(9, 16, 3, 'night'); trabi.circle(30, 16, 3, 'night'); trabi.set(9, 16, 'ash'); trabi.set(30, 16, 'ash');
  trabi.set(37, 11, 'gold'); trabi.set(2, 11, 'red'); trabi.outline('ink');
  out.push({ name: 'trabi', pix: trabi });

  return out;
}
