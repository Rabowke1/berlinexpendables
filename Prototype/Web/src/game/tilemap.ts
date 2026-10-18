import { T, Tile, TILE_HP, Back, BACK_FRAME } from './constants';
import type { LevelData } from './level';
import type { Renderer } from '../engine/renderer';
import type { Atlas } from '../engine/assets';
import { VIEW_W, VIEW_H } from '../engine/renderer';

export interface Body { x: number; y: number; w: number; h: number; dropThrough?: number }

export class Tilemap {
  readonly w: number; readonly h: number;
  readonly tiles: Uint8Array; readonly hp: Float32Array; readonly back: Uint8Array;
  /** Ursprünglich solide Tiles: dahinter wird dunkles Erdreich gezeichnet, damit Krater nicht "durchsichtig" sind. */
  readonly wasSolid: Uint8Array;
  constructor(level: LevelData) {
    this.w = level.w; this.h = level.h;
    this.tiles = level.tiles.slice(); this.back = level.back.slice();
    this.wasSolid = new Uint8Array(this.w * this.h);
    for (let i = 0; i < this.tiles.length; i++) this.wasSolid[i] = this.isSolid(this.tiles[i] as Tile) && this.tiles[i] !== Tile.CRATE ? 1 : 0;
    this.hp = new Float32Array(this.w * this.h);
    for (let i = 0; i < this.hp.length; i++) this.hp[i] = TILE_HP[this.tiles[i] as Tile];
  }
  get pixelW(): number { return this.w * T; }
  get pixelH(): number { return this.h * T; }

  tile(tx: number, ty: number): Tile {
    if (tx < 0 || tx >= this.w) return Tile.STEEL;      // Levelränder sind Wände
    if (ty < 0 || ty >= this.h) return Tile.AIR;
    return this.tiles[ty * this.w + tx] as Tile;
  }
  tileAtPx(x: number, y: number): Tile { return this.tile(Math.floor(x / T), Math.floor(y / T)); }
  isSolid(t: Tile): boolean { return t !== Tile.AIR && t !== Tile.PLATFORM; }
  solidAtPx(x: number, y: number): boolean { return this.isSolid(this.tileAtPx(x, y)); }

  /** Schaden an einem Tile; true wenn zerstört. */
  damage(tx: number, ty: number, amount: number, onDestroy?: (tx: number, ty: number, t: Tile) => void): boolean {
    if (tx < 0 || ty < 0 || tx >= this.w || ty >= this.h) return false;
    const i = ty * this.w + tx; const t = this.tiles[i] as Tile;
    if (t === Tile.AIR || t === Tile.STEEL) return false;
    this.hp[i] -= amount;
    if (this.hp[i] > 0) return false;
    this.tiles[i] = Tile.AIR; this.hp[i] = 0;
    onDestroy?.(tx, ty, t);
    return true;
  }

  moveX(b: Body, dx: number): boolean {
    if (dx === 0) return false;
    const nx = b.x + dx;
    const y0 = Math.floor(b.y / T), y1 = Math.floor((b.y + b.h - 1) / T);
    if (dx > 0) {
      const tx = Math.floor((nx + b.w - 1) / T);
      for (let ty = y0; ty <= y1; ty++) if (this.isSolid(this.tile(tx, ty))) { b.x = tx * T - b.w; return true; }
    } else {
      const tx = Math.floor(nx / T);
      for (let ty = y0; ty <= y1; ty++) if (this.isSolid(this.tile(tx, ty))) { b.x = (tx + 1) * T; return true; }
    }
    b.x = nx; return false;
  }
  moveY(b: Body, dy: number): boolean {
    if (dy === 0) return false;
    const ny = b.y + dy;
    const x0 = Math.floor(b.x / T), x1 = Math.floor((b.x + b.w - 1) / T);
    if (dy > 0) {
      const ty = Math.floor((ny + b.h - 1) / T);
      const wasAbove = b.y + b.h - 1 < ty * T;
      for (let tx = x0; tx <= x1; tx++) {
        const t = this.tile(tx, ty);
        if (this.isSolid(t) || (t === Tile.PLATFORM && wasAbove && !(b.dropThrough && b.dropThrough > 0))) { b.y = ty * T - b.h; return true; }
      }
    } else {
      const ty = Math.floor(ny / T);
      for (let tx = x0; tx <= x1; tx++) if (this.isSolid(this.tile(tx, ty))) { b.y = (ty + 1) * T; return true; }
    }
    b.y = ny; return false;
  }
  /** Steht der Körper auf Boden/Plattform? */
  grounded(b: Body): boolean {
    const ty = Math.floor((b.y + b.h) / T);
    if ((b.y + b.h) % T !== 0) return false;
    const x0 = Math.floor(b.x / T), x1 = Math.floor((b.x + b.w - 1) / T);
    for (let tx = x0; tx <= x1; tx++) { const t = this.tile(tx, ty); if (this.isSolid(t) || t === Tile.PLATFORM) return true; }
    return false;
  }
  onPlatformOnly(b: Body): boolean {
    const ty = Math.floor((b.y + b.h) / T);
    const x0 = Math.floor(b.x / T), x1 = Math.floor((b.x + b.w - 1) / T);
    let plat = false;
    for (let tx = x0; tx <= x1; tx++) { const t = this.tile(tx, ty); if (this.isSolid(t)) return false; if (t === Tile.PLATFORM) plat = true; }
    return plat;
  }

  draw(r: Renderer, tilesAtlas: Atlas): void {
    const img = tilesAtlas.image, F = tilesAtlas.frames;
    const x0 = Math.max(0, Math.floor(r.camX / T)), x1 = Math.min(this.w - 1, Math.floor((r.camX + VIEW_W) / T));
    const y0 = Math.max(0, Math.floor(r.camY / T)), y1 = Math.min(this.h - 1, Math.floor((r.camY + VIEW_H) / T));
    for (let ty = y0; ty <= y1; ty++) for (let tx = x0; tx <= x1; tx++) {
      const i = ty * this.w + tx;
      const b = this.back[i] as Back;
      if (b) r.world(img, F[BACK_FRAME[b]], tx * T, ty * T);
      const t = this.tiles[i] as Tile;
      if (!t) { if (this.wasSolid[i]) r.worldRect(tx * T, ty * T, T, T, '#1b1510'); continue; }
      const above = this.tile(tx, ty - 1);
      let name: string;
      switch (t) {
        case Tile.DIRT: name = above === Tile.AIR || above === Tile.PLATFORM ? 'dirt_top' : 'dirt'; break;
        case Tile.BRICK: name = above === Tile.AIR ? 'brick_top' : 'brick'; break;
        case Tile.STEEL: name = 'steel'; break;
        case Tile.CONCRETE: name = 'concrete'; break;
        case Tile.PLATFORM: name = 'platform'; break;
        case Tile.CRATE: name = 'crate'; break;
        default: continue;
      }
      r.world(img, F[name], tx * T, ty * T);
      const max = TILE_HP[t];
      if (Number.isFinite(max)) {
        const f = this.hp[i] / max;
        if (f < 0.8) r.world(img, F[f < 0.3 ? 'crack_3' : f < 0.55 ? 'crack_2' : 'crack_1'], tx * T, ty * T);
      }
    }
  }
}
