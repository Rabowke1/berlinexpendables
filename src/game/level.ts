// ASCII-Levelformat. Jede Zeile = eine Tilereihe. Legende siehe docs/ABLAUF.md.
import { Tile, Back } from './constants';

export interface EntitySpawn { type: string; tx: number; ty: number; arg?: string }
export interface LevelData {
  name: string;
  w: number; h: number;
  tiles: Uint8Array;
  back: Uint8Array;
  spawn: { tx: number; ty: number };
  entities: EntitySpawn[];
}
export interface LevelSource { name: string; rows: string[] }

const SOLID: Record<string, Tile> = { '#': Tile.DIRT, '=': Tile.BRICK, S: Tile.STEEL, C: Tile.CONCRETE, '-': Tile.PLATFORM, c: Tile.CRATE };
const BACKS: Record<string, Back> = { w: Back.PLASTER, W: Back.WINDOW, r: Back.BRICK, g: Back.GRAFFITI };
const ENTS: Record<string, string> = { m: 'mook', s: 'security', b: 'barrel', k: 'cage', E: 'exit', '+': 'life', H: 'item', R: 'item', F: 'item', L: 'item', B: 'item' };

export function parseLevel(src: LevelSource): LevelData {
  const h = src.rows.length;
  const w = Math.max(...src.rows.map((r) => r.length));
  const tiles = new Uint8Array(w * h), back = new Uint8Array(w * h);
  const entities: EntitySpawn[] = [];
  let spawn = { tx: 2, ty: 2 };
  for (let y = 0; y < h; y++) {
    const row = src.rows[y].padEnd(w, '.');
    for (let x = 0; x < w; x++) {
      const ch = row[x];
      const i = y * w + x;
      if (ch in SOLID) tiles[i] = SOLID[ch];
      else if (ch in BACKS) back[i] = BACKS[ch];
      else if (ch === 'P') spawn = { tx: x, ty: y };
      else if (ch in ENTS) entities.push({ type: ENTS[ch], tx: x, ty: y, arg: ch });
      else if (ch !== '.' && ch !== ' ') throw new Error(`Unbekanntes Levelzeichen '${ch}' bei ${x},${y}`);
    }
  }
  // Hinter Gebäude-Innenräumen (Fenster/Putz) auch unter Entities Putz annehmen: Nachbar-Back übernehmen
  for (const e of entities) {
    const i = e.ty * w + e.tx;
    const left = e.tx > 0 ? back[i - 1] : 0, right = e.tx < w - 1 ? back[i + 1] : 0;
    if (left && right) back[i] = left;
  }
  return { name: src.name, w, h, tiles, back, spawn, entities };
}
