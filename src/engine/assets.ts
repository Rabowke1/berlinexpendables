// Atlas-Loader für das TexturePacker-Hash-Format (+ unsere Zusatzfelder).
export interface FrameDef {
  x: number; y: number; w: number; h: number;
  hand?: [number, number];
  grip?: [number, number];
  tip?: [number, number];
}
export interface Atlas {
  name: string;
  image: HTMLImageElement;
  frames: Record<string, FrameDef>;
  animations: Record<string, string[]>;
  meta: Record<string, any>;
}

const BASE = import.meta.env.BASE_URL.replace(/\/?$/, '/');

export function loadImage(url: string): Promise<HTMLImageElement> {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => resolve(img);
    img.onerror = () => reject(new Error(`Bild konnte nicht geladen werden: ${url}`));
    img.src = url;
  });
}

export async function loadAtlas(name: string): Promise<Atlas> {
  const res = await fetch(`${BASE}assets/${name}.json`);
  if (!res.ok) throw new Error(`Atlas fehlt: ${name}.json – bitte \`npm run assets\` ausführen.`);
  const json = await res.json();
  const image = await loadImage(`${BASE}assets/${json.meta.image}`);
  const frames: Record<string, FrameDef> = {};
  for (const [k, v] of Object.entries<any>(json.frames)) {
    frames[k] = { x: v.frame.x, y: v.frame.y, w: v.frame.w, h: v.frame.h, hand: v.hand, grip: v.grip, tip: v.tip };
  }
  return { name, image, frames, animations: json.animations ?? {}, meta: json.meta ?? {} };
}

export class Assets {
  atlases = new Map<string, Atlas>();
  images = new Map<string, HTMLImageElement>();
  palette: Record<string, string> = {};

  async load(atlasNames: string[], imageNames: string[]): Promise<void> {
    const [atlases, images, pal] = await Promise.all([
      Promise.all(atlasNames.map(loadAtlas)),
      Promise.all(imageNames.map((n) => loadImage(`${BASE}assets/${n}.png`))),
      fetch(`${BASE}assets/palette.json`).then((r) => r.json()),
    ]);
    atlases.forEach((a) => this.atlases.set(a.name, a));
    images.forEach((img, i) => this.images.set(imageNames[i], img));
    this.palette = pal;
  }
  atlas(name: string): Atlas {
    const a = this.atlases.get(name);
    if (!a) throw new Error(`Atlas nicht geladen: ${name}`);
    return a;
  }
  image(name: string): HTMLImageElement {
    const i = this.images.get(name);
    if (!i) throw new Error(`Bild nicht geladen: ${name}`);
    return i;
  }
  color(name: string): string { return this.palette[name] ?? '#ff00ff'; }
}
