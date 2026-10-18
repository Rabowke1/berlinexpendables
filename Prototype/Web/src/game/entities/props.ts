import type { World } from '../world';
import type { Renderer } from '../../engine/renderer';
import type { PickupKind } from '../characters';
import { PICKUPS } from '../characters';
import { T } from '../constants';

/** Gasflasche: ein Treffer und sie geht hoch (mit Kettenreaktion). */
export class Barrel {
  dead = false;
  w = 8; h = 13;
  fuse = -1;
  constructor(public x: number, public y: number) {}
  ignite(_w: World, delay: number): void { if (this.fuse < 0) this.fuse = delay; }
  update(w: World): void {
    if (this.fuse >= 0 && --this.fuse <= 0) { this.dead = true; w.explode(this.x + 4, this.y + 6, 40, { kind: 'enemy' }, 'barrel'); w.addScore(50, this.x, this.y); }
  }
  draw(r: Renderer, w: World): void {
    const t = w.assets.atlas('tiles');
    r.world(t.image, t.frames['barrel'], this.x - 4, this.y - 3);
  }
}

/** Käfig mit Gefangenem – befreien = +1 Leben, Item, Charakterwechsel (Broforce-Regel). */
export class Cage {
  dead = false;
  w = 16; h = 24;
  hp = 3;
  constructor(public x: number, public y: number) {}
  hurt(w: World, dmg: number, rescuer: number | null): void {
    this.hp -= dmg;
    w.sparks(this.x + 8, this.y + 8, 3, ['steel', 'ash']);
    if (this.hp <= 0) this.open(w, rescuer);
  }
  open(w: World, rescuer: number | null): void {
    if (this.dead) return;
    this.dead = true;
    w.rescue(this, rescuer);
  }
  update(): void {}
  draw(r: Renderer, w: World): void {
    const cap = w.assets.atlas('captive');
    const frame = cap.animations['idle_h'][Math.floor(w.tick / 14) % 4];
    r.world(cap.image, cap.frames[frame], this.x + 8 - 16, this.y + 24 - 31, Math.floor(w.tick / 90) % 2 === 1);
    const t = w.assets.atlas('tiles');
    r.world(t.image, t.frames['cage'], this.x, this.y);
  }
}

/** Befreiter Gefangener: rennt winkend davon. */
export class Captive {
  dead = false;
  w = 10; h = 22; vy = 0; dropThrough = 0;
  life = 110;
  constructor(public x: number, public y: number, public facing: number) {}
  update(w: World): void {
    this.vy = Math.min(6, this.vy + 0.3);
    if (w.map.moveX(this, this.facing * 1.2)) this.facing *= -1;
    if (w.map.moveY(this, this.vy)) this.vy = 0;
    if (--this.life <= 0 || this.y > w.map.pixelH) { this.dead = true; w.puff(this.x + 5, this.y + 10); }
  }
  draw(r: Renderer, w: World): void {
    const a = w.assets.atlas('captive');
    const frame = a.animations['run_h'][Math.floor(w.tick / 5) % 8];
    if (this.life < 20 && (this.life & 2)) return;
    r.world(a.image, a.frames[frame], this.x - 11, this.y - 8, this.facing < 0);
  }
}

export type ItemKind = PickupKind | 'bombs' | 'life';
const ITEM_FRAME: Record<ItemKind, string> = { heavy: 'item_H', rocket: 'item_R', flame: 'item_F', laser: 'item_L', bombs: 'item_B', life: 'item_life' };

export class Item {
  dead = false;
  w = 10; h = 10; vy = -2; vx = 0; dropThrough = 0;
  life = 1200;
  constructor(public x: number, public y: number, public kind: ItemKind) {}
  update(w: World): void {
    this.vy = Math.min(5, this.vy + 0.25);
    if (w.map.moveX(this, this.vx)) this.vx = 0;
    if (w.map.moveY(this, this.vy)) { this.vy = 0; this.vx *= 0.8; }
    if (--this.life <= 0 || this.y > w.map.pixelH) this.dead = true;
    for (const p of w.players) {
      if (!p.alive) continue;
      if (p.x < this.x + this.w && p.x + p.w > this.x && p.y < this.y + this.h && p.y + p.h > this.y) { this.dead = true; this.apply(w, p); break; }
    }
  }
  private apply(w: World, p: import('./player').Player): void {
    w.sfx.play('pickup');
    switch (this.kind) {
      case 'bombs': p.grenades = Math.min(9, p.grenades + 5); w.popup('+5 GRANATEN', this.x, this.y - 10, '#7fd04f'); break;
      case 'life': w.lives++; w.popup('+1 LEBEN', this.x, this.y - 10, '#ff3fa4'); break;
      default: { const pk = PICKUPS[this.kind]; p.pickup = { kind: this.kind, ammo: pk.ammo }; w.popup(pk.name, this.x, this.y - 10, '#ffcf5c'); }
    }
    w.addScore(100, this.x, this.y, false);
  }
  draw(r: Renderer, w: World): void {
    if (this.life < 180 && (this.life & 4)) return;
    const t = w.assets.atlas('tiles');
    const bob = Math.round(Math.sin(w.tick / 10) * 1);
    r.world(t.image, t.frames[ITEM_FRAME[this.kind]], this.x, this.y + bob);
  }
}

/** Levelausgang: der Fluchttrabi. */
export class Exit {
  dead = false;
  w = 40; h = 20;
  leaving = -1;
  constructor(public x: number, public y: number) { this.y = y + T - 20; }
  update(w: World): void {
    if (this.leaving >= 0) { this.leaving++; this.x += 1 + this.leaving * 0.08; if (this.leaving % 3 === 0) w.puff(this.x, this.y + 18); return; }
    const alive = w.players.filter((p) => p.alive);
    if (!alive.length) return;
    const touching = alive.filter((p) => p.x < this.x + this.w && p.x + p.w > this.x && p.y < this.y + this.h && p.y + p.h > this.y);
    const near = alive.every((p) => Math.abs(p.x + p.w / 2 - (this.x + this.w / 2)) < 90);
    if (touching.length && near) { this.leaving = 0; w.win(); }
  }
  draw(r: Renderer, w: World): void {
    const t = w.assets.atlas('tiles');
    r.world(t.image, t.frames['trabi'], this.x, this.y + (this.leaving >= 0 ? (this.leaving & 2 ? 1 : 0) : 0));
    if (this.leaving < 0) r.label('ZUM TRABI!', this.x + this.w / 2 - r.camX, this.y - 14 - r.camY + Math.round(Math.sin(w.tick / 12) * 2), '#ffcf5c', 'center');
  }
}
