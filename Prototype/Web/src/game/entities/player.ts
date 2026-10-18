import type { World } from '../world';
import type { Renderer } from '../../engine/renderer';
import type { DeviceId } from '../../engine/input';
import type { FrameDef } from '../../engine/assets';
import { PLAYER_CHARS, PICKUPS, type Aim, type CharacterData, type PickupKind, type WeaponProfile } from '../characters';
import { GRAV, MAX_FALL, RUN, ACC, ACC_AIR, JUMP, COYOTE, JUMP_BUFFER, INVULN_FRAMES, RESPAWN_FRAMES, PLAYER_COLORS, T } from '../constants';
import { Bullet } from './bullet';
import { Grenade } from './grenade';

/** Zeichnet die Waffe am Hand-Anker eines Körper-Frames (Spieler und Gegner). */
export function drawWeaponInHand(r: Renderer, w: World, weapon: string, aim: Aim, bodyFrame: FrameDef, ox: number, oy: number, flip: boolean, skin: string): void {
  if (!bodyFrame.hand) return;
  const weapons = w.assets.atlas('weapons');
  const wf = weapons.frames[`${weapon}_${aim}`];
  if (!wf) return;
  const lx = bodyFrame.hand[0] - wf.grip![0], ly = bodyFrame.hand[1] - wf.grip![1];
  const dx = flip ? 32 - lx - wf.w : lx;
  r.world(weapons.image, wf, ox + dx, oy + ly, flip);
  // Hand über den Griff
  const hx = flip ? 32 - bodyFrame.hand[0] - 2 : bodyFrame.hand[0];
  r.worldRect(ox + hx, oy + bodyFrame.hand[1], 2, 2, skin);
}

export class Player {
  x: number; y: number; w = 10; h = 22; vx = 0; vy = 0;
  onGround = false; dropThrough = 0;
  facing: 1 | -1 = 1;
  aim: Aim = 'h';
  crouch = false;
  alive = true; deadTimer = 0; invuln = INVULN_FRAMES;
  coyote = 0; jumpBuf = 0;
  fireCd = 0; muzzleT = 0; muzzlePos: [number, number] = [0, 0];
  grenades: number; specialCd = 0;
  meleeT = 0;
  pickup: { kind: PickupKind; ammo: number } | null = null;
  banner = 150;
  private animT = 0;
  kills = 0;

  constructor(public slot: number, public device: DeviceId, public charIdx: number, x: number, y: number) {
    this.x = x; this.y = y; this.grenades = this.char.grenades;
  }
  get char(): CharacterData { return PLAYER_CHARS[this.charIdx]; }
  get color(): string { return PLAYER_COLORS[this.slot]; }
  get weapon(): WeaponProfile { return this.pickup ? PICKUPS[this.pickup.kind].weapon : this.char.weapon; }
  get cx(): number { return this.x + this.w / 2; }
  get cy(): number { return this.y + this.h / 2; }

  switchCharacter(idx: number): void { this.charIdx = idx; this.grenades = Math.max(this.grenades, this.char.grenades); this.banner = 150; }

  private setCrouch(on: boolean): void {
    if (on === this.crouch) return;
    if (on) { this.y += 5; this.h = 17; this.crouch = true; }
    else {
      // Nur aufstehen, wenn oben Platz ist
      const top = this.y - 5;
      const x0 = Math.floor(this.x / T), x1 = Math.floor((this.x + this.w - 1) / T), ty = Math.floor(top / T);
      let blocked = false;
      for (let tx = x0; tx <= x1; tx++) blocked ||= this.world!.map.isSolid(this.world!.map.tile(tx, ty));
      if (blocked) return;
      this.y = top; this.h = 22; this.crouch = false;
    }
  }
  private world: World | null = null;

  update(w: World): void {
    this.world = w;
    this.animT++;
    if (this.banner > 0) this.banner--;
    if (!this.alive) { if (--this.deadTimer <= 0) w.tryRespawn(this); return; }
    if (this.invuln > 0) this.invuln--;
    const inp = w.input.read(this.device);

    // Ducken & Zielen
    this.setCrouch(inp.down && this.onGround && !inp.up && this.meleeT <= 0);
    this.aim = inp.up ? 'u' : inp.down && !this.onGround ? 'd' : 'h';

    // Horizontal
    let ax = 0; if (inp.left) ax -= 1; if (inp.right) ax += 1;
    const max = this.crouch ? 0.55 : RUN;
    if (ax !== 0) { this.facing = ax > 0 ? 1 : -1; this.vx += ax * (this.onGround ? ACC : ACC_AIR); }
    else if (this.onGround) { this.vx *= 0.68; if (Math.abs(this.vx) < 0.05) this.vx = 0; }
    else this.vx *= 0.97;
    if (Math.abs(this.vx) > max) this.vx = Math.sign(this.vx) * Math.max(max, Math.abs(this.vx) * 0.9);

    // Springen (Coyote + Buffer, variable Höhe, durch Plattformen fallen)
    this.coyote = this.onGround ? COYOTE : this.coyote - 1;
    this.jumpBuf = inp.jumpPressed ? JUMP_BUFFER : this.jumpBuf - 1;
    if (this.jumpBuf > 0 && this.coyote > 0) {
      if (inp.down && w.map.onPlatformOnly(this)) { this.dropThrough = 10; this.setCrouch(false); this.y += 1; }
      else { this.setCrouch(false); this.vy = JUMP; w.sfx.play('jump'); w.dust(this.cx, this.y + this.h, 4); }
      this.coyote = 0; this.jumpBuf = 0; this.onGround = false;
    }
    if (!inp.jump && this.vy < -2.4) this.vy = -2.4;
    if (this.dropThrough > 0) this.dropThrough--;
    this.vy = Math.min(MAX_FALL, this.vy + GRAV);

    // Bewegung
    const wasGround = this.onGround;
    if (w.map.moveX(this, this.vx)) this.vx = 0;
    if (w.map.moveY(this, this.vy)) { this.onGround = this.vy > 0; if (this.vy > 2.5 && this.onGround) w.dust(this.cx, this.y + this.h, 3); this.vy = 0; }
    else this.onGround = false;
    if (!wasGround && this.onGround) w.sfx.play('land', 0.4);
    // Im Kamerabild bleiben (Koop: niemand rennt davon)
    const minX = w.camera.left + 1, maxX = w.camera.right - this.w - 1;
    if (this.x < minX) { this.x = minX; if (this.vx < 0) this.vx = 0; }
    if (this.x > maxX) { this.x = maxX; if (this.vx > 0) this.vx = 0; }
    if (this.y > w.map.pixelH + 30) { w.killPlayer(this, 'fall'); return; }

    // Nahkampf (automatisch bei Gegnerkontakt, Metal-Slug-Style, oder per Taste)
    if (this.meleeT > 0) {
      this.meleeT--;
      if (this.meleeT === 7) this.meleeHit(w);
    } else if (inp.meleePressed || this.enemyInReach(w)) { this.meleeT = 12; w.sfx.play('melee'); }

    // Feuern
    if (this.fireCd > 0) this.fireCd--;
    if (this.muzzleT > 0) this.muzzleT--;
    if (inp.fire && this.fireCd <= 0 && this.meleeT <= 0) this.fire(w);
    // Special: Granate
    if (this.specialCd > 0) this.specialCd--;
    if (inp.specialPressed && this.specialCd <= 0) {
      if (this.grenades > 0) {
        this.grenades--; this.specialCd = 18;
        const vx = this.facing * 3.1 + this.vx * 0.5, vy = this.aim === 'u' ? -6 : -4.2;
        w.grenades.push(new Grenade(this.cx - 2, this.y + 4, vx, vy, this.char.grenadeRadius, { kind: 'player', slot: this.slot }));
        w.sfx.play('throw');
      } else { w.popup('KEINE GRANATEN', this.cx, this.y - 10, '#9d9bb0'); this.specialCd = 30; }
    }
  }

  private enemyInReach(w: World): boolean {
    for (const m of w.mooks) {
      if (m.dead) continue;
      const inFront = this.facing > 0 ? m.x + m.w > this.x + this.w - 2 && m.x < this.x + this.w + 12 : m.x < this.x + 2 && m.x + m.w > this.x - 12;
      if (inFront && Math.abs(m.y - this.y) < 16) return true;
    }
    return false;
  }
  private meleeHit(w: World): void {
    const bx = this.facing > 0 ? this.x + this.w - 2 : this.x - 16, by = this.y - 2, bw = 18, bh = this.h + 4;
    let hitSomething = false;
    for (const m of w.mooks) {
      if (m.dead) continue;
      if (m.x < bx + bw && m.x + m.w > bx && m.y < by + bh && m.y + m.h > by) { w.hurtMook(m, 3, null, this.slot, 'melee'); hitSomething = true; }
    }
    for (const b of w.barrels) if (!b.dead && b.x < bx + bw && b.x + b.w > bx && b.y < by + bh && b.y + b.h > by) b.ignite(w, 1);
    for (const c of w.cages) if (!c.dead && c.x < bx + bw && c.x + c.w > bx && c.y < by + bh && c.y + c.h > by) c.hurt(w, 3, this.slot);
    // Graben: Tile vor dem Spieler beschädigen
    const tx = Math.floor((this.facing > 0 ? this.x + this.w + 5 : this.x - 5) / T), ty = Math.floor((this.y + this.h / 2) / T);
    if (w.map.isSolid(w.map.tile(tx, ty))) { w.damageTile(tx, ty, 2.5); w.sfx.play('dig'); hitSomething = true; }
    if (hitSomething) w.camera.shake(0.15);
  }

  frameName(): string {
    if (this.meleeT > 0) return `melee_h_${this.meleeT > 8 ? 0 : this.meleeT > 3 ? 1 : 2}`;
    if (this.crouch) return `crouch_${this.aim === 'u' ? 'u' : 'h'}_0`;
    if (!this.onGround) return `${this.vy < 0 ? 'jump' : 'fall'}_${this.aim}_0`;
    if (Math.abs(this.vx) > 0.2) return `run_${this.aim}_${Math.floor(this.animT / 5) % 8}`;
    return `idle_${this.aim}_${Math.floor(this.animT / 12) % 4}`;
  }
  private originY(): number { return this.y - (this.crouch ? 13 : 8); }

  muzzle(w: World): [number, number] {
    const a = w.assets.atlas(this.char.id), weapons = w.assets.atlas('weapons');
    const f = a.frames[this.frameName()], wf = weapons.frames[`${this.weapon.sprite}_${this.aim}`];
    const hand = f.hand ?? [22, 17];
    const lx = hand[0] - wf.grip![0] + wf.tip![0], ly = hand[1] - wf.grip![1] + wf.tip![1];
    const ox = this.x - 11, oy = this.originY();
    return [this.facing > 0 ? ox + lx : ox + 32 - lx, oy + ly];
  }

  private fire(w: World): void {
    const prof = this.weapon;
    this.fireCd = prof.cooldown; this.muzzleT = 5;
    const mz = this.muzzle(w); this.muzzlePos = mz;
    const dir = this.aim === 'u' ? [0, -1] : this.aim === 'd' ? [0, 1] : [this.facing, 0];
    const base = Math.atan2(dir[1], dir[0]);
    for (let i = 0; i < prof.count; i++) {
      const ang = base + (prof.count > 1 ? (i / (prof.count - 1) - 0.5) * 2 * prof.spread : 0) + (Math.random() - 0.5) * prof.spread * 0.6;
      const s = prof.bullet.speed * (prof.count > 1 ? 0.85 + Math.random() * 0.3 : 1);
      const b = new Bullet(mz[0], mz[1], Math.cos(ang) * s + (this.aim === 'h' ? this.vx * 0.3 : 0), Math.sin(ang) * s, prof.bullet, { kind: 'player', slot: this.slot });
      if (prof.bullet.lob && this.aim === 'u') { b.vy -= 1; }
      w.bullets.push(b);
    }
    if (this.aim === 'h') this.vx -= this.facing * prof.recoil * (this.onGround ? 0.5 : 1);
    else if (this.aim === 'd' && !this.onGround) this.vy -= prof.recoil * 1.6;   // Ketchup-Jump
    w.camera.shake(prof.shake);
    w.sfx.play(prof.sfx);
    if (this.pickup && --this.pickup.ammo <= 0) { this.pickup = null; w.popup('LEER!', this.cx, this.y - 12, '#9d9bb0'); }
  }

  draw(r: Renderer, w: World): void {
    if (!this.alive) return;
    const blink = this.invuln > 0 && (w.tick & 2) !== 0;
    const a = w.assets.atlas(this.char.id);
    const f = a.frames[this.frameName()];
    const ox = this.x - 11, oy = this.originY();
    const flip = this.facing < 0;
    if (!blink) {
      r.world(a.image, f, ox, oy, flip);
      if (this.meleeT <= 0) drawWeaponInHand(r, w, this.weapon.sprite, this.aim, f, ox, oy, flip, a.meta.skin as string);
    }
    if (this.muzzleT > 0) {
      const fx = w.assets.atlas('fx');
      const mf = fx.frames[`muzzle_${Math.min(2, 5 - this.muzzleT)}`];
      const [mx, my] = this.muzzlePos;
      const c = r.ctx;
      c.save(); c.translate(Math.round(mx - r.camX), Math.round(my - r.camY));
      if (this.aim === 'u') c.rotate(-Math.PI / 2); else if (this.aim === 'd') c.rotate(Math.PI / 2); else if (flip) c.scale(-1, 1);
      c.drawImage(fx.image, mf.x, mf.y, mf.w, mf.h, 0, -4, mf.w, mf.h); c.restore();
    }
    // Spielermarker über dem Kopf
    const my = oy + (this.crouch ? 3 : -2) + Math.round(Math.sin(w.tick / 8));
    r.worldRect(this.cx - 3, my, 6, 1, this.color); r.worldRect(this.cx - 2, my + 1, 4, 1, this.color); r.worldRect(this.cx - 1, my + 2, 2, 1, this.color);
    if (w.debug) { r.worldStroke(this.x, this.y, this.w, this.h, this.color); const [mx, mz] = this.muzzle(w); r.worldRect(mx - 1, mz - 1, 2, 2, '#ffffff'); }
  }

  /** Gegnerkontakt tötet (außer unverwundbar). */
  touches(e: { x: number; y: number; w: number; h: number }): boolean {
    return this.x < e.x + e.w && this.x + this.w > e.x && this.y < e.y + e.h && this.y + this.h > e.y;
  }
  die(): void { this.alive = false; this.deadTimer = RESPAWN_FRAMES; this.pickup = null; this.meleeT = 0; this.setCrouchRaw(false); }
  private setCrouchRaw(on: boolean): void { if (!on && this.crouch) { this.y -= 5; this.h = 22; this.crouch = false; } }
  respawnAt(x: number, y: number): void {
    this.alive = true; this.x = x; this.y = y; this.vx = 0; this.vy = -3; this.invuln = INVULN_FRAMES; this.banner = 150; this.grenades = Math.max(this.grenades, this.char.grenades);
  }
}
