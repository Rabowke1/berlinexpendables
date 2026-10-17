import { Renderer, VIEW_H } from '../engine/renderer';
import type { Assets } from '../engine/assets';
import type { Input, DeviceId } from '../engine/input';
import type { Sfx } from '../engine/audio';
import { Camera } from '../engine/camera';
import { Tilemap } from './tilemap';
import { parseLevel, type LevelSource } from './level';
import { Particles } from './particles';
import { Player } from './entities/player';
import { Mook } from './entities/mook';
import { Bullet, type Owner } from './entities/bullet';
import { Grenade } from './entities/grenade';
import { Barrel, Cage, Captive, Item, Exit, type ItemKind } from './entities/props';
import { FxSprite, GibSprite, Popup } from './entities/fx';
import { PLAYER_CHARS } from './characters';
import { T, Tile, TILE_DEBRIS, START_LIVES, MAX_PLAYERS } from './constants';
import { drawHud } from './hud';

export type GameState = 'play' | 'won' | 'gameover';

export class World {
  map: Tilemap;
  camera = new Camera();
  particles = new Particles();
  players: Player[] = [];
  mooks: Mook[] = [];
  bullets: Bullet[] = [];
  grenades: Grenade[] = [];
  barrels: Barrel[] = [];
  cages: Cage[] = [];
  captives: Captive[] = [];
  items: Item[] = [];
  exits: Exit[] = [];
  fx: FxSprite[] = [];
  popups: Popup[] = [];
  state: GameState = 'play';
  stateT = 0;
  tick = 0;
  lives = START_LIVES;
  score = 0;
  freeze = 0;
  debug = false;
  spawn: { x: number; y: number };
  levelName: string;
  settings = { gore: true };
  onRestart: () => void = () => {};

  constructor(public assets: Assets, public input: Input, public sfx: Sfx, level: LevelSource) {
    const data = parseLevel(level);
    this.levelName = data.name;
    this.map = new Tilemap(data);
    this.spawn = { x: data.spawn.tx * T, y: data.spawn.ty * T };
    for (const e of data.entities) {
      const px = e.tx * T, py = e.ty * T;
      switch (e.type) {
        case 'mook': this.mooks.push(new Mook(px + 3, py - 6, 'mook')); break;
        case 'security': this.mooks.push(new Mook(px + 3, py - 6, 'security')); break;
        case 'barrel': this.barrels.push(new Barrel(px + 4, py + 3)); break;
        case 'cage': this.cages.push(new Cage(px, py - 8)); break;
        case 'exit': this.exits.push(new Exit(px, py)); break;
        case 'life': this.items.push(new Item(px + 3, py + 3, 'life')); break;
        case 'item': {
          const kind: ItemKind = ({ H: 'heavy', R: 'rocket', F: 'flame', L: 'laser', B: 'bombs' } as Record<string, ItemKind>)[e.arg ?? 'B'];
          this.items.push(new Item(px + 3, py + 3, kind)); break;
        }
      }
    }
    this.camera.snapTo(this.spawn.x, this.spawn.y, this.map.pixelW, this.map.pixelH);
  }

  // --- Spieler-Verwaltung ------------------------------------------------------
  addPlayer(device: DeviceId): Player | null {
    if (this.players.length >= MAX_PLAYERS) return null;
    const slot = this.players.length;
    const used = new Set(this.players.map((p) => p.charIdx));
    let idx = slot % PLAYER_CHARS.length;
    while (used.has(idx) && used.size < PLAYER_CHARS.length) idx = (idx + 1) % PLAYER_CHARS.length;
    const anchor = this.players.find((p) => p.alive);
    const x = anchor ? anchor.x : this.spawn.x, y = anchor ? anchor.y - 20 : this.spawn.y - 24;
    const p = new Player(slot, device, idx, x, y);
    p.vy = -2;
    this.players.push(p);
    this.popup(`${p.char.name} IST DABEI!`, p.cx, p.y - 16, p.color);
    this.sfx.play('rescue');
    return p;
  }
  tryRespawn(p: Player): void {
    if (this.state !== 'play') return;
    if (this.lives <= 0) { if (!this.players.some((q) => q.alive)) this.gameOver(); return; }
    this.lives--;
    const anchor = this.players.find((q) => q.alive);
    const x = anchor ? anchor.x : this.spawn.x, y = anchor ? anchor.y - 30 : this.spawn.y - 30;
    // Broforce-Regel: neuer Bro nach dem Tod
    let idx = (p.charIdx + 1 + Math.floor(Math.random() * (PLAYER_CHARS.length - 1))) % PLAYER_CHARS.length;
    const used = new Set(this.players.filter((q) => q !== p && q.alive).map((q) => q.charIdx));
    for (let i = 0; i < PLAYER_CHARS.length && used.has(idx); i++) idx = (idx + 1) % PLAYER_CHARS.length;
    p.charIdx = idx;
    p.respawnAt(x, y);
  }
  killPlayer(p: Player, cause: string): void {
    if (!p.alive || p.invuln > 0) return;
    p.die();
    this.gibs(p.cx, p.cy, p.char.id, cause === 'explosion' ? 5 : 3);
    this.blood(p.cx, p.cy, 24);
    this.camera.shake(0.45);
    this.freeze = 4;
    this.sfx.play('die');
    if (this.lives <= 0 && !this.players.some((q) => q.alive)) this.gameOver();
  }

  // --- Gegner --------------------------------------------------------------
  hurtMook(m: Mook, dmg: number, _b: Bullet | null, slot: number | null, how: 'shot' | 'melee' | 'explosion' = 'shot'): void {
    if (m.dead) return;
    m.hp -= dmg;
    this.blood(m.x + m.w / 2, m.y + m.h / 2, 4);
    if (m.hp > 0) { this.sfx.play('hit'); m.stun = 10; return; }
    m.dead = true;
    this.sfx.play('hit');
    this.gibs(m.x + m.w / 2, m.y + m.h / 2, m.data.atlas, how === 'explosion' ? 5 : 3);
    this.blood(m.x + m.w / 2, m.y + m.h / 2, 16);
    this.addScore(m.data.score, m.x, m.y);
    if (slot !== null && this.players[slot]) this.players[slot].kills++;
    // Umstehende geraten in Panik
    for (const o of this.mooks) {
      if (o.dead || o === m || o.state === 'panic') continue;
      if (Math.abs(o.x - m.x) < 70 && Math.abs(o.y - m.y) < 40 && Math.random() < 0.5) { o.state = 'panic'; o.timer = 90 + Math.random() * 60; o.facing = o.x < m.x ? -1 : 1; }
    }
  }

  // --- Welt-Effekte ----------------------------------------------------------
  damageTile(tx: number, ty: number, amount: number): void {
    this.map.damage(tx, ty, amount, (x, y, t) => {
      const colors = TILE_DEBRIS[t].map((c) => this.assets.color(c));
      this.particles.burst(x * T + 8, y * T + 8, 8, colors, 2.2);
      if (t === Tile.CRATE) { this.addScore(10, x * T, y * T, false); if (Math.random() < 0.35) this.items.push(new Item(x * T + 3, y * T + 2, (['bombs', 'heavy', 'flame', 'rocket', 'laser', 'life'] as ItemKind[])[(Math.random() * 6) | 0])); }
    });
  }
  explode(x: number, y: number, radius: number, owner: Owner, source: 'barrel' | 'weapon' = 'weapon'): void {
    const r2 = radius * radius;
    const tx0 = Math.floor((x - radius) / T), tx1 = Math.floor((x + radius) / T), ty0 = Math.floor((y - radius) / T), ty1 = Math.floor((y + radius) / T);
    for (let ty = ty0; ty <= ty1; ty++) for (let tx = tx0; tx <= tx1; tx++) {
      const dx = tx * T + 8 - x, dy = ty * T + 8 - y, d2 = dx * dx + dy * dy;
      if (d2 > r2) continue;
      this.damageTile(tx, ty, 12 * (1 - Math.sqrt(d2) / radius) + 2);
    }
    for (const m of this.mooks) if (!m.dead && dist2(m.x + m.w / 2, m.y + m.h / 2, x, y) < (radius + 6) ** 2) this.hurtMook(m, 9, null, owner.kind === 'player' ? owner.slot : null, 'explosion');
    for (const p of this.players) if (p.alive && dist2(p.cx, p.cy, x, y) < (radius * 0.85) ** 2) this.killPlayer(p, 'explosion');
    for (const b of this.barrels) if (!b.dead && dist2(b.x + 4, b.y + 6, x, y) < (radius + 10) ** 2) b.ignite(this, 4 + Math.random() * 8);
    for (const c of this.cages) if (!c.dead && dist2(c.x + 8, c.y + 12, x, y) < (radius + 6) ** 2) c.open(this, owner.kind === 'player' ? owner.slot : null);
    this.fx.push(new FxSprite('explosion', x, y, 3, Math.random() < 0.5));
    if (radius > 26) for (let i = 0; i < 2; i++) this.fx.push(new FxSprite('explosion', x + (Math.random() - 0.5) * radius, y + (Math.random() - 0.5) * radius, 3 + i, Math.random() < 0.5));
    this.particles.burst(x, y, 18, ['#ff8c2a', '#ffe678', '#e23b3b'], 4, { gravity: 0.08, life: 25 });
    this.particles.burst(x, y, 14, ['#6e6c87', '#9d9bb0', '#4a4866'], 1.5, { gravity: -0.02, bounce: false, life: 50 });
    this.camera.shake(source === 'barrel' ? 0.6 : 0.5);
    this.freeze = Math.max(this.freeze, 3);
    this.sfx.play('explosion');
  }
  rescue(c: Cage, rescuer: number | null): void {
    this.lives++;
    this.addScore(500, c.x, c.y - 10);
    this.popup('+1 LEBEN', c.x + 8, c.y - 22, '#ff3fa4');
    this.sfx.play('rescue');
    this.particles.burst(c.x + 8, c.y + 12, 12, ['#87a7c7', '#9d9bb0'], 2.5);
    const facing = rescuer !== null && this.players[rescuer] ? (this.players[rescuer].x < c.x ? 1 : -1) : 1;
    this.captives.push(new Captive(c.x + 3, c.y + 2, facing));
    const kinds: ItemKind[] = ['heavy', 'rocket', 'flame', 'laser', 'bombs'];
    this.items.push(new Item(c.x + 3, c.y + 4, kinds[(Math.random() * kinds.length) | 0]));
    if (rescuer !== null && this.players[rescuer]) {
      const p = this.players[rescuer];
      const used = new Set(this.players.filter((q) => q !== p && q.alive).map((q) => q.charIdx));
      let idx = (p.charIdx + 1) % PLAYER_CHARS.length;
      for (let i = 0; i < PLAYER_CHARS.length && (used.has(idx) || idx === p.charIdx); i++) idx = (idx + 1) % PLAYER_CHARS.length;
      if (idx !== p.charIdx) { p.switchCharacter(idx); this.puff(p.cx, p.cy); }
    }
  }
  addScore(n: number, x: number, y: number, show = true): void { this.score += n; if (show) this.popup(`+${n}`, x + 5, y - 6, '#ffe678'); }
  popup(text: string, x: number, y: number, color: string): void { this.popups.push(new Popup(text, x, y, color)); }
  flash(x: number, y: number, flip: boolean): void { this.fx.push(new FxSprite('muzzle', x, y, 2, flip)); }
  puff(x: number, y: number): void { this.fx.push(new FxSprite('smoke', x, y, 4)); }
  sparks(x: number, y: number, n: number, colors = ['gold', 'orange', 'white']): void { this.particles.burst(x, y, n, colors.map((c) => this.assets.color(c)), 2, { life: 14, gravity: 0.15 }); }
  dust(x: number, y: number, n: number): void { for (let i = 0; i < n; i++) this.particles.add({ x: x + (Math.random() - 0.5) * 8, y, vx: (Math.random() - 0.5) * 1.2, vy: -Math.random() * 0.6, life: 18, color: '#9d9bb0', size: 1, gravity: -0.01, bounce: false }); }
  blood(x: number, y: number, n: number): void {
    const colors = this.settings.gore ? ['#e23b3b', '#9d1b1b', '#e23b3b'] : ['#ff3fa4', '#38f0e6', '#ffe678', '#7fd04f'];
    this.particles.burst(x, y, n, colors, 3, { gravity: 0.22, life: 60 });
  }
  gibs(x: number, y: number, atlas: string, n: number): void {
    const names = ['gib_head', 'gib_torso', 'gib_leg', 'gib_leg', 'gib_torso'];
    for (let i = 0; i < n; i++) this.fx.push(new GibSprite(atlas, names[i % names.length], x, y, (Math.random() - 0.5) * 5, -2 - Math.random() * 3));
  }
  win(): void { if (this.state === 'play') { this.state = 'won'; this.stateT = 0; this.sfx.play('rescue'); } }
  gameOver(): void { if (this.state === 'play') { this.state = 'gameover'; this.stateT = 0; } }

  // --- Hauptschleife ---------------------------------------------------------
  update(): void {
    this.input.poll();
    if (this.input.pressedKeys.has('F1')) this.debug = !this.debug;
    if (this.freeze > 0) { this.freeze--; return; }
    this.tick++;
    if (this.state === 'play') {
      const assigned = new Set(this.players.map((p) => p.device));
      for (const d of this.input.joinRequests()) if (!assigned.has(d)) this.addPlayer(d);
    } else {
      this.stateT++;
      if (this.stateT > 90 && this.input.anyStartPressed()) this.onRestart();
    }
    for (const p of this.players) p.update(this);
    for (const m of this.mooks) if (!m.dead) m.update(this);
    for (const b of this.bullets) b.update(this);
    for (const g of this.grenades) g.update(this);
    for (const b of this.barrels) b.update(this);
    for (const c of this.captives) c.update(this);
    for (const i of this.items) i.update(this);
    for (const e of this.exits) e.update(this);
    for (const f of this.fx) f.update(this);
    for (const p of this.popups) p.update();
    this.particles.update(this.map);
    // Gegnerkontakt
    for (const p of this.players) if (p.alive && p.invuln <= 0) for (const m of this.mooks) if (!m.dead && m.state !== 'panic' && p.touches(m)) { this.killPlayer(p, 'touch'); break; }
    this.mooks = this.mooks.filter((m) => !m.dead);
    this.bullets = this.bullets.filter((b) => !b.dead);
    this.grenades = this.grenades.filter((g) => !g.dead);
    this.barrels = this.barrels.filter((b) => !b.dead);
    this.cages = this.cages.filter((c) => !c.dead);
    this.captives = this.captives.filter((c) => !c.dead);
    this.items = this.items.filter((i) => !i.dead);
    this.fx = this.fx.filter((f) => !f.dead);
    this.popups = this.popups.filter((p) => !p.dead);
    const alive = this.players.filter((p) => p.alive && this.state !== 'won').map((p) => ({ x: p.cx, y: p.cy }));
    if (!alive.length && this.state === 'won' && this.exits[0]) alive.push({ x: this.exits[0].x + 20, y: this.exits[0].y });
    if (!alive.length && !this.players.length) alive.push({ x: this.spawn.x, y: this.spawn.y });
    this.camera.follow(alive, this.map.pixelW, this.map.pixelH);
  }

  draw(r: Renderer): void {
    r.setCamera(this.camera.x + this.camera.shakeX, this.camera.y + this.camera.shakeY);
    r.ctx.drawImage(this.assets.image('bg_sky'), 0, 0);
    r.parallax(this.assets.image('bg_skyline'), 0.15, 110 - this.camera.y * 0.15);
    r.parallax(this.assets.image('bg_mid'), 0.4, 165 - this.camera.y * 0.4);
    this.map.draw(r, this.assets.atlas('tiles'));
    for (const c of this.cages) c.draw(r, this);
    for (const e of this.exits) e.draw(r, this);
    for (const b of this.barrels) b.draw(r, this);
    for (const i of this.items) i.draw(r, this);
    for (const c of this.captives) c.draw(r, this);
    for (const m of this.mooks) m.draw(r, this);
    const won = this.state === 'won';
    if (!won) for (const p of this.players) p.draw(r, this);
    for (const g of this.grenades) g.draw(r, this);
    for (const b of this.bullets) b.draw(r, this);
    for (const f of this.fx) f.draw(r, this);
    this.particles.draw(r);
    for (const p of this.popups) p.draw(r);
    drawHud(r, this);
    if (this.debug) {
      r.label(`TICK ${this.tick} ENT ${this.mooks.length + this.bullets.length + this.particles.list.length} CAM ${Math.round(this.camera.x)},${Math.round(this.camera.y)}`, 4, VIEW_H - 10, '#38f0e6');
      for (const b of this.barrels) r.worldStroke(b.x, b.y, b.w, b.h, '#ff8c2a');
      for (const c of this.cages) r.worldStroke(c.x, c.y, c.w, c.h, '#87a7c7');
    }
  }
}

function dist2(ax: number, ay: number, bx: number, by: number): number { const dx = ax - bx, dy = ay - by; return dx * dx + dy * dy; }
