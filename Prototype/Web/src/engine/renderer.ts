// Pixel-perfekter Renderer: 480x270 Offscreen-Buffer, Integer-Skalierung auf das Fenster.
import type { FrameDef } from './assets';
import { Font } from './font';

export const VIEW_W = 480;
export const VIEW_H = 270;

export class Renderer {
  readonly screen: HTMLCanvasElement;
  readonly buf: HTMLCanvasElement;
  readonly ctx: CanvasRenderingContext2D;
  private readonly sctx: CanvasRenderingContext2D;
  readonly font = new Font();
  camX = 0;
  camY = 0;
  scale = 1;

  constructor(parent: HTMLElement) {
    this.screen = document.createElement('canvas');
    parent.appendChild(this.screen);
    this.sctx = this.screen.getContext('2d', { alpha: false })!;
    this.buf = document.createElement('canvas');
    this.buf.width = VIEW_W; this.buf.height = VIEW_H;
    this.ctx = this.buf.getContext('2d', { alpha: false })!;
    this.ctx.imageSmoothingEnabled = false;
    window.addEventListener('resize', () => this.resize());
    this.resize();
  }

  resize(): void {
    const s = Math.max(1, Math.floor(Math.min(window.innerWidth / VIEW_W, window.innerHeight / VIEW_H)));
    this.scale = s;
    this.screen.width = VIEW_W * s; this.screen.height = VIEW_H * s;
    this.screen.style.width = `${VIEW_W * s}px`; this.screen.style.height = `${VIEW_H * s}px`;
    this.sctx.imageSmoothingEnabled = false;
  }

  setCamera(x: number, y: number): void { this.camX = Math.round(x); this.camY = Math.round(y); }

  clear(color: string): void { this.ctx.fillStyle = color; this.ctx.fillRect(0, 0, VIEW_W, VIEW_H); }

  present(): void {
    this.sctx.imageSmoothingEnabled = false;
    this.sctx.drawImage(this.buf, 0, 0, this.screen.width, this.screen.height);
  }

  /** Sprite in Bildschirmkoordinaten (Pixel, wird gerundet). */
  sprite(img: CanvasImageSource, f: FrameDef, x: number, y: number, flip = false): void {
    const c = this.ctx; x = Math.round(x); y = Math.round(y);
    if (x + f.w < 0 || y + f.h < 0 || x > VIEW_W || y > VIEW_H) return;
    if (!flip) { c.drawImage(img, f.x, f.y, f.w, f.h, x, y, f.w, f.h); return; }
    c.save(); c.translate(x + f.w, y); c.scale(-1, 1); c.drawImage(img, f.x, f.y, f.w, f.h, 0, 0, f.w, f.h); c.restore();
  }
  /** Sprite in Weltkoordinaten. */
  world(img: CanvasImageSource, f: FrameDef, wx: number, wy: number, flip = false): void {
    this.sprite(img, f, wx - this.camX, wy - this.camY, flip);
  }
  rect(x: number, y: number, w: number, h: number, color: string): void {
    this.ctx.fillStyle = color; this.ctx.fillRect(Math.round(x), Math.round(y), Math.round(w), Math.round(h));
  }
  worldRect(x: number, y: number, w: number, h: number, color: string): void { this.rect(x - this.camX, y - this.camY, w, h, color); }
  worldStroke(x: number, y: number, w: number, h: number, color: string): void {
    this.ctx.strokeStyle = color; this.ctx.lineWidth = 1;
    this.ctx.strokeRect(Math.round(x - this.camX) + 0.5, Math.round(y - this.camY) + 0.5, w - 1, h - 1);
  }
  /** Kachelbares Bild mit Parallax-Faktor horizontal wiederholen. */
  parallax(img: HTMLImageElement, factor: number, y: number): void {
    const off = Math.round(this.camX * factor) % img.width;
    for (let x = -off; x < VIEW_W; x += img.width) this.ctx.drawImage(img, x, Math.round(y));
  }
  text(s: string, x: number, y: number, color: string, align: 'left' | 'center' | 'right' = 'left', scale = 1): void {
    this.font.draw(this.ctx, s, x, y, color, align, scale);
  }
  /** Text mit 1px Schatten (für HUD). */
  label(s: string, x: number, y: number, color: string, align: 'left' | 'center' | 'right' = 'left', scale = 1, shadow = '#0b0a12'): void {
    this.text(s, x + scale, y + scale, shadow, align, scale);
    this.text(s, x, y, color, align, scale);
  }
}
