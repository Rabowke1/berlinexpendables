import type { Renderer } from '../../engine/renderer';
import type { World } from '../world';

/** Einmalige Sprite-Animation (Explosion, Rauch, Mündungsfeuer). */
export class FxSprite {
  dead = false;
  protected t = 0;
  constructor(public anim: string, public x: number, public y: number, public speed = 3, public flip = false, public loop = false) {}
  update(_w: World): void { this.t++; }
  draw(r: Renderer, w: World): void {
    const fx = w.assets.atlas('fx');
    const frames = fx.animations[this.anim];
    if (!frames) { this.dead = true; return; }
    let i = Math.floor(this.t / this.speed);
    if (i >= frames.length) { if (this.loop) i %= frames.length; else { this.dead = true; return; } }
    const f = fx.frames[frames[i]];
    r.world(fx.image, f, this.x - f.w / 2, this.y - f.h / 2, this.flip);
  }
}

/** Körperteil mit Physik (Gibs) – nutzt die Charakter-Atlanten. */
export class GibSprite extends FxSprite {
  private life = 140;
  private body = { x: 0, y: 0, w: 4, h: 4, dropThrough: 0 };
  constructor(private atlas: string, private frame: string, x: number, y: number, private vx: number, private vy: number) { super('', x, y); }
  override update(w: World): void {
    this.vy = Math.min(6, this.vy + 0.3);
    const b = this.body; b.x = this.x; b.y = this.y;
    if (w.map.moveX(b, this.vx)) this.vx *= -0.5;
    if (w.map.moveY(b, this.vy)) { if (this.vy > 1) this.vy = -this.vy * 0.4; else this.vy = 0; this.vx *= 0.8; }
    this.x = b.x; this.y = b.y;
    if (--this.life <= 0 || this.y > w.map.pixelH + 40) this.dead = true;
  }
  override draw(r: Renderer, w: World): void {
    const a = w.assets.atlas(this.atlas);
    const f = a.frames[this.frame];
    if (!f || (this.life < 30 && (this.life & 2))) return;
    r.world(a.image, f, this.x - f.w / 2, this.y - f.h / 2, this.vx < 0);
  }
}

/** Schwebender Text ("+100", "DANKE!"). */
export class Popup {
  dead = false;
  life = 50;
  constructor(public text: string, public x: number, public y: number, public color: string) {}
  update(): void { this.y -= 0.4; if (--this.life <= 0) this.dead = true; }
  draw(r: Renderer): void {
    if (this.life < 12 && (this.life & 2)) return;
    r.label(this.text, this.x - r.camX, this.y - r.camY, this.color, 'center');
  }
}
