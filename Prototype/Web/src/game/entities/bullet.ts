import type { World } from '../world';
import type { Renderer } from '../../engine/renderer';
import type { BulletProfile } from '../characters';
import { T } from '../constants';

export type Owner = { kind: 'player'; slot: number } | { kind: 'enemy' };

export class Bullet {
  dead = false;
  life: number;
  pierce: number;
  w = 4; h = 4;
  private t = 0;
  private hit = new Set<object>();
  constructor(public x: number, public y: number, public vx: number, public vy: number, public prof: BulletProfile, public owner: Owner) {
    this.life = prof.life; this.pierce = prof.pierce;
    if (prof.lob) this.vy -= prof.lob;
  }
  update(w: World): void {
    this.t++;
    this.vy += this.prof.gravity;
    const steps = Math.max(1, Math.ceil(Math.max(Math.abs(this.vx), Math.abs(this.vy)) / 4));
    for (let s = 0; s < steps && !this.dead; s++) {
      this.x += this.vx / steps; this.y += this.vy / steps;
      const tx = Math.floor(this.x / T), ty = Math.floor(this.y / T);
      if (w.map.isSolid(w.map.tile(tx, ty))) {
        if (this.prof.tileDmg > 0) w.damageTile(tx, ty, this.prof.tileDmg);
        w.sparks(this.x - this.vx / steps, this.y - this.vy / steps, 3);
        this.impact(w);
        break;
      }
      if (this.owner.kind === 'player') {
        for (const m of w.mooks) {
          if (m.dead || this.hit.has(m)) continue;
          if (this.x >= m.x - 2 && this.x <= m.x + m.w + 2 && this.y >= m.y - 2 && this.y <= m.y + m.h + 2) {
            this.hit.add(m);
            w.hurtMook(m, this.prof.dmg, this, this.owner.slot);
            if (this.pierce-- <= 0) { this.impact(w); break; }
          }
        }
        for (const b of w.barrels) if (!b.dead && !this.hit.has(b) && this.inside(b)) { this.hit.add(b); b.ignite(w, 2); if (this.pierce-- <= 0) { this.impact(w); break; } }
        for (const c of w.cages) if (!c.dead && !this.hit.has(c) && this.inside(c)) { this.hit.add(c); c.hurt(w, this.prof.dmg, this.owner.slot); if (this.pierce-- <= 0) { this.impact(w); break; } }
      } else {
        for (const p of w.players) {
          if (!p.alive || p.invuln > 0) continue;
          if (this.x >= p.x && this.x <= p.x + p.w && this.y >= p.y && this.y <= p.y + p.h) { w.killPlayer(p, 'shot'); this.dead = true; break; }
        }
      }
    }
    if (--this.life <= 0) { if (this.prof.explode) this.impact(w); else this.dead = true; }
    if (this.y > w.map.pixelH + 50 || this.x < -50 || this.x > w.map.pixelW + 50) this.dead = true;
  }
  private inside(b: { x: number; y: number; w: number; h: number }): boolean { return this.x >= b.x && this.x <= b.x + b.w && this.y >= b.y && this.y <= b.y + b.h; }
  private impact(w: World): void {
    this.dead = true;
    if (this.prof.explode) w.explode(this.x, this.y, this.prof.explode, this.owner);
    else if (this.prof.sprite === 'bottle') w.sparks(this.x, this.y, 6, ['brown', 'glass']);
  }
  draw(r: Renderer, w: World): void {
    const fx = w.assets.atlas('fx');
    const anim = fx.animations[this.prof.sprite];
    const name = anim ? anim[Math.floor(this.t / 4) % anim.length] : this.prof.sprite;
    const f = fx.frames[name];
    if (!f) return;
    const vertical = Math.abs(this.vy) > Math.abs(this.vx) * 1.5 && this.prof.sprite !== 'bottle' && this.prof.sprite !== 'grenade';
    const c = r.ctx;
    if (vertical) {
      c.save(); c.translate(Math.round(this.x - r.camX), Math.round(this.y - r.camY)); c.rotate(this.vy < 0 ? -Math.PI / 2 : Math.PI / 2);
      c.drawImage(fx.image, f.x, f.y, f.w, f.h, -Math.round(f.w / 2), -Math.round(f.h / 2), f.w, f.h); c.restore();
    } else {
      r.world(fx.image, f, this.x - f.w / 2, this.y - f.h / 2, this.vx < 0);
    }
  }
}
