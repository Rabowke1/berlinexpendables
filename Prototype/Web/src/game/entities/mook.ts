import type { World } from '../world';
import type { Renderer } from '../../engine/renderer';
import { ENEMIES, type EnemyData, type Aim } from '../characters';
import { GRAV, MAX_FALL } from '../constants';
import { Bullet } from './bullet';
import { drawWeaponInHand } from './player';

type State = 'patrol' | 'alert' | 'attack' | 'panic';

export class Mook {
  dead = false;
  w = 10; h = 22; vx = 0; vy = 0; onGround = false; dropThrough = 0;
  facing: 1 | -1 = -1;
  state: State = 'patrol';
  timer = 60 + Math.random() * 120;
  shootCd = 60;
  burstLeft = 0; burstTimer = 0;
  aim: Aim = 'h';
  hp: number;
  stun = 0;
  data: EnemyData;
  private animT = 0;
  constructor(public x: number, public y: number, kind: 'mook' | 'security') { this.data = ENEMIES[kind]; this.hp = this.data.hp; }

  private target(w: World) {
    let best: import('./player').Player | null = null, bd = Infinity;
    for (const p of w.players) {
      if (!p.alive) continue;
      const d = Math.abs(p.x - this.x);
      if (d < bd) { bd = d; best = p; }
    }
    return best;
  }

  update(w: World): void {
    this.animT++;
    if (this.stun > 0) { this.stun--; this.vx = 0; }
    const tgt = this.target(w);
    const dx = tgt ? tgt.x + tgt.w / 2 - (this.x + this.w / 2) : 0;
    const dy = tgt ? tgt.y - this.y : 0;
    const sees = !!tgt && Math.abs(dx) < this.data.sight && Math.abs(dy) < 56 && (Math.sign(dx) === this.facing || Math.abs(dx) < 36);

    if (this.stun <= 0) switch (this.state) {
      case 'patrol': {
        this.timer--;
        const walking = this.timer > 0 || this.timer < -60;
        if (this.timer < -60) this.timer = 90 + Math.random() * 120;
        this.vx = walking ? this.facing * this.data.speed : 0;
        if (walking && this.onGround) {
          const aheadX = this.facing > 0 ? this.x + this.w + 2 : this.x - 2;
          if (!w.map.solidAtPx(aheadX, this.y + this.h + 2) || w.map.solidAtPx(aheadX, this.y + this.h - 4)) this.facing *= -1;
        }
        if (sees) { this.state = 'alert'; this.timer = 22; this.facing = dx > 0 ? 1 : -1; this.vx = 0; w.sfx.play('alert', 0.5); }
        break;
      }
      case 'alert':
        this.vx = 0;
        if (--this.timer <= 0) { this.state = 'attack'; this.shootCd = 20; this.timer = 0; }
        break;
      case 'attack': {
        this.vx = 0;
        if (!tgt || Math.abs(dx) > this.data.sight + 40 || Math.abs(dy) > 90) { if (++this.timer > 60) this.state = 'patrol'; break; }
        this.timer = 0;
        if (this.burstLeft <= 0) this.facing = dx > 0 ? 1 : -1;
        this.aim = dy < -30 && Math.abs(dx) < 40 ? 'u' : 'h';
        if (this.burstLeft > 0) {
          if (--this.burstTimer <= 0) { this.fire(w); this.burstLeft--; this.burstTimer = this.data.burstGap; }
        } else if (--this.shootCd <= 0) {
          this.burstLeft = this.data.burst; this.burstTimer = 0;
          const [a, b] = this.data.cooldown; this.shootCd = a + Math.random() * (b - a);
        }
        break;
      }
      case 'panic': {
        this.vx = this.facing * 1.3;
        if (this.onGround) {
          const aheadX = this.facing > 0 ? this.x + this.w + 2 : this.x - 2;
          if (!w.map.solidAtPx(aheadX, this.y + this.h + 2) || w.map.solidAtPx(aheadX, this.y + this.h - 4)) this.facing *= -1;
        }
        if (--this.timer <= 0) this.state = 'patrol';
        break;
      }
    }

    this.vy = Math.min(MAX_FALL, this.vy + GRAV);
    if (w.map.moveX(this, this.vx)) { this.vx = 0; if (this.state === 'patrol' || this.state === 'panic') this.facing *= -1; }
    const wasGround = this.onGround;
    if (w.map.moveY(this, this.vy)) { this.onGround = this.vy > 0; this.vy = 0; } else this.onGround = false;
    if (!wasGround && this.onGround && this.state !== 'panic') this.vx = 0;
    if (this.y > w.map.pixelH + 40) this.dead = true;
  }

  private fire(w: World): void {
    const mz = this.muzzle(w);
    const dir = this.aim === 'u' ? [0, -1] : [this.facing, 0];
    const spread = (Math.random() - 0.5) * 0.12;
    const ang = Math.atan2(dir[1], dir[0]) + spread;
    const s = this.data.bullet.speed;
    w.bullets.push(new Bullet(mz[0], mz[1], Math.cos(ang) * s, Math.sin(ang) * s, this.data.bullet, { kind: 'enemy' }));
    w.flash(mz[0], mz[1], this.facing < 0);
    w.sfx.play('shoot', 0.5);
  }

  frameName(): string {
    if (this.state === 'panic') return `panic_h_${Math.floor(this.animT / 5) % 4}`;
    if (!this.onGround) return `${this.vy < 0 ? 'jump' : 'fall'}_${this.aim}_0`;
    if (Math.abs(this.vx) > 0.1) return `run_${this.aim}_${Math.floor(this.animT / 7) % 8}`;
    return `idle_${this.aim}_${Math.floor(this.animT / 14) % 4}`;
  }
  private muzzle(w: World): [number, number] {
    const a = w.assets.atlas(this.data.atlas), weapons = w.assets.atlas('weapons');
    const f = a.frames[this.frameName()], wf = weapons.frames[`${this.data.weapon}_${this.aim}`];
    const hand = f.hand ?? [22, 17];
    const lx = hand[0] - wf.grip![0] + wf.tip![0], ly = hand[1] - wf.grip![1] + wf.tip![1];
    const ox = this.x - 11, oy = this.y - 8;
    return [this.facing > 0 ? ox + lx : ox + 32 - lx, oy + ly];
  }

  draw(r: Renderer, w: World): void {
    const a = w.assets.atlas(this.data.atlas);
    const name = this.frameName();
    const f = a.frames[name];
    const ox = this.x - 11, oy = this.y - 8;
    const flip = this.facing < 0;
    if (this.stun > 0 && (w.tick & 2)) r.ctx.globalAlpha = 0.6;
    r.world(a.image, f, ox, oy, flip);
    if (this.state !== 'panic') drawWeaponInHand(r, w, this.data.weapon, this.aim, f, ox, oy, flip, a.meta.skin as string);
    r.ctx.globalAlpha = 1;
    if (this.state === 'alert') { const fx = w.assets.atlas('fx'); r.world(fx.image, fx.frames['alert'], this.x + 4, this.y - 12 - (this.timer > 14 ? 2 : 0)); }
    if (this.stun > 0) r.label('?', this.x + 5 - r.camX, this.y - 14 - r.camY, '#38f0e6', 'center');
    if (w.debug) r.worldStroke(this.x, this.y, this.w, this.h, '#e23b3b');
  }
}
