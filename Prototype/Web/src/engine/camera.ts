import { VIEW_W, VIEW_H } from './renderer';

export class Camera {
  x = 0; y = 0;
  private trauma = 0;
  shakeX = 0; shakeY = 0;
  private seed = 1;

  snapTo(cx: number, cy: number, levelW: number, levelH: number): void {
    this.x = cx - VIEW_W / 2; this.y = cy - VIEW_H / 2; this.clamp(levelW, levelH);
  }
  /** Folgt der Bounding-Box aller Punkte; Spieler bleiben so gemeinsam im Bild. */
  follow(points: { x: number; y: number }[], levelW: number, levelH: number): void {
    if (points.length) {
      let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
      for (const p of points) { minX = Math.min(minX, p.x); maxX = Math.max(maxX, p.x); minY = Math.min(minY, p.y); maxY = Math.max(maxY, p.y); }
      const tx = (minX + maxX) / 2 - VIEW_W / 2;
      const ty = (minY + maxY) / 2 - VIEW_H / 2 - 20;   // etwas Luft nach oben
      this.x += (tx - this.x) * 0.12;
      this.y += (ty - this.y) * 0.1;
    }
    this.clamp(levelW, levelH);
    // Shake (Trauma²), deterministisch genug per LCG
    this.trauma = Math.max(0, this.trauma - 0.035);
    const t = this.trauma * this.trauma;
    this.seed = (this.seed * 1103515245 + 12345) & 0x7fffffff;
    const r1 = (this.seed / 0x7fffffff) * 2 - 1;
    this.seed = (this.seed * 1103515245 + 12345) & 0x7fffffff;
    const r2 = (this.seed / 0x7fffffff) * 2 - 1;
    this.shakeX = Math.round(r1 * 6 * t); this.shakeY = Math.round(r2 * 4 * t);
  }
  private clamp(levelW: number, levelH: number): void {
    this.x = Math.max(0, Math.min(levelW - VIEW_W, this.x));
    this.y = Math.max(0, Math.min(Math.max(0, levelH - VIEW_H), this.y));
  }
  shake(amount: number): void { this.trauma = Math.min(1, this.trauma + amount); }
  get left(): number { return this.x; }
  get right(): number { return this.x + VIEW_W; }
}
