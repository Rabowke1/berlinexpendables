// Einstiegspunkt: `npm run assets` erzeugt alle Sprites/Atlanten nach public/assets und den Kontaktbogen nach docs/art.
import { Pix, savePNG, saveJSON } from './png.js';
import { hex, PALETTE, COLOR_NAMES } from './palette.js';
import { buildCharacterFrames, BODY_BOX, CROUCH_BOX, drawHumanoid, poseFor } from './humanoid.js';
import { WEAPON_DEFS, weaponPix, rotateWeapon } from './weapons.js';
import { CHARACTERS } from './characters.js';
import { buildTiles } from './tiles.js';
import { buildFx } from './fx.js';
import { buildSky, buildSkyline, buildMid } from './backgrounds.js';
import { pack } from './atlas.js';

const OUT = 'public/assets';
const t0 = Date.now();
let files = 0;

// --- Charaktere ---------------------------------------------------------------
for (const ch of CHARACTERS) {
  const frames = buildCharacterFrames(ch.recipe, !ch.player);
  const { sheet, atlas } = pack(
    frames.map((f) => ({ name: f.name, pix: f.pix, extra: f.hand ? { hand: f.hand } : {} })),
    `${ch.id}.png`, 512, 1,
    { character: ch.id, name: ch.name, weapon: ch.weapon, box: BODY_BOX, crouchBox: CROUCH_BOX, skin: hex(ch.recipe.skin), accent: hex(ch.recipe.accent), frameSize: 32 },
  );
  savePNG(`${OUT}/${ch.id}.png`, sheet); saveJSON(`${OUT}/${ch.id}.json`, atlas); files += 2;
}

// --- Waffen -------------------------------------------------------------------
{
  const inputs = [];
  for (const d of WEAPON_DEFS) {
    const base = weaponPix(d.kind, d.color, d.accent);
    for (const aim of ['h', 'u', 'd'] as const) {
      const w = rotateWeapon(base, aim);
      const pix = w.pix.clone();
      // Outline mit 1px Rand
      const padded = new Pix(pix.w + 2, pix.h + 2); padded.blit(pix, 1, 1); padded.outline('ink');
      inputs.push({ name: `${d.kind}_${aim}`, pix: padded, extra: { grip: [w.grip[0] + 1, w.grip[1] + 1], tip: [w.tip[0] + 1, w.tip[1] + 1] } });
    }
  }
  const { sheet, atlas } = pack(inputs, 'weapons.png', 256, 1);
  savePNG(`${OUT}/weapons.png`, sheet); saveJSON(`${OUT}/weapons.json`, atlas); files += 2;
}

// --- Tiles / FX ---------------------------------------------------------------
{
  const { sheet, atlas } = pack(buildTiles(), 'tiles.png', 256, 1, { tile: 16 });
  savePNG(`${OUT}/tiles.png`, sheet); saveJSON(`${OUT}/tiles.json`, atlas); files += 2;
}
{
  const { sheet, atlas } = pack(buildFx(), 'fx.png', 512, 1);
  savePNG(`${OUT}/fx.png`, sheet); saveJSON(`${OUT}/fx.json`, atlas); files += 2;
}

// --- Hintergründe -------------------------------------------------------------
savePNG(`${OUT}/bg_sky.png`, buildSky());
savePNG(`${OUT}/bg_skyline.png`, buildSkyline());
savePNG(`${OUT}/bg_mid.png`, buildMid());
files += 3;

// --- Palette als JSON fürs Spiel ------------------------------------------------
saveJSON(`${OUT}/palette.json`, Object.fromEntries(COLOR_NAMES.map((n) => [n, hex(n)])));
files++;

// --- Kontaktbogen (docs/art/preview.png, 4x skaliert) -----------------------------
{
  const cols = 12, cell = 34;
  const rows = CHARACTERS.length + 3;
  const sheet = new Pix(cols * cell + 8, rows * cell + 8, 'navy');
  CHARACTERS.forEach((ch, r) => {
    const weaponDef = WEAPON_DEFS.find((w) => w.kind === ch.weapon)!;
    const demo: [string, number, 'h' | 'u' | 'd'][] = [['idle', 0, 'h'], ['run', 0, 'h'], ['run', 2, 'h'], ['run', 4, 'h'], ['run', 6, 'h'], ['jump', 0, 'h'], ['fall', 0, 'h'], ['idle', 0, 'u'], ['fall', 0, 'd'], ['crouch', 0, 'h'], ['melee', 1, 'h'], ['panic', 1, 'h']];
    demo.forEach(([anim, f, aim], c) => {
      if (anim === 'panic' && ch.player) return;
      const d = drawHumanoid(ch.recipe, poseFor(anim as never, f, aim));
      const cellPix = new Pix(32, 32); cellPix.blit(d.pix, 0, 0);
      if (d.hand) {
        const w = rotateWeapon(weaponPix(weaponDef.kind, weaponDef.color, weaponDef.accent), aim);
        const padded = new Pix(w.pix.w + 2, w.pix.h + 2); padded.blit(w.pix, 1, 1); padded.outline('ink');
        cellPix.blit(padded, d.hand[0] - w.grip[0] - 1, d.hand[1] - w.grip[1] - 1);
        cellPix.rect(d.hand[0], d.hand[1], 2, 2, ch.recipe.skin);
      }
      sheet.blit(cellPix, 4 + c * cell, 4 + r * cell);
    });
  });
  // Tiles
  buildTiles().slice(0, 12).forEach((t, c) => sheet.blit(t.pix, 4 + c * cell, 4 + CHARACTERS.length * cell + 8));
  // Explosion
  buildFx().filter((f) => f.name.startsWith('explosion')).forEach((f, c) => sheet.blit(f.pix, 4 + c * cell, 4 + (CHARACTERS.length + 1) * cell + 8));
  // Waffen
  WEAPON_DEFS.forEach((d, c) => { const w = weaponPix(d.kind, d.color, d.accent); const padded = new Pix(w.pix.w + 2, w.pix.h + 2); padded.blit(w.pix, 1, 1); padded.outline('ink'); sheet.blit(padded, 4 + c * cell, 12 + (CHARACTERS.length + 2) * cell + 8); });
  savePNG('docs/art/preview.png', sheet, 4);
  // Palette-Swatches
  const sw = new Pix(COLOR_NAMES.length * 10, 10);
  COLOR_NAMES.forEach((n, i) => sw.rect(i * 10, 0, 10, 10, n));
  savePNG('docs/art/palette.png', sw, 4);
  files += 2;
}

console.log(`spritegen: ${files} Dateien in ${Date.now() - t0} ms geschrieben (${Object.keys(PALETTE).length} Farben).`);
