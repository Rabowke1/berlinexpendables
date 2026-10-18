import type { World } from '../world';
import type { Renderer } from '../../engine/renderer';
import type { Owner } from './bullet';

export class Grenade {
  dead = false;
  w = 5; h = 5;
  fuse = 70;
  dropThrough = 0;
  constructor(public x: number, public y: number, public vx: number, public vy: number, public radius: number, public owner: Owner) {}
  update(w: World): void {
    this.vy = Math.min(6, this.vy + 0.25);
    if (w.map.moveX(this, this.vx)) this.vx *= -0.5;
    if (w.map.moveY(this, this.vy)) {
      if (this.vy > 0) { this.vy = -this.vy * 0.45; this.vx *= 0.75; if (Math.abs(this.vy) < 0.9) this.vy = 0; }
      else this.vy = 0;
    }
    if (--this.fuse <= 0 || this.y > w.map.pixelH + 40) { this.dead = true; w.explode(this.x + 2, this.y + 2, this.radius, this.owner); }
    if (this.fuse % 10 === 0) w.particles.add({ x: this.x + 2, y: this.y, vx: 0, vy: -0.3, life: 15, color: '#9d9bb0', size: 1, gravity: 0, bounce: false });
  }
  draw(r: Renderer, w: World): void {
    const fx = w.assets.atlas('fx');
    r.world(fx.image, fx.frames['grenade'], this.x - 1, this.y - 2, this.vx < 0);
    if (this.fuse < 25 && (this.fuse & 4)) r.worldRect(this.x + 1, this.y - 2, 3, 2, '#ffffff');
  }
}
