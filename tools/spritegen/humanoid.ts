// Humanoid-Rig: zeichnet Spieler & Gegner als 32x32-Frames aus einem Rezept (Palette + Props).
// Koordinaten: Figur blickt nach rechts, Füße auf y=29 (Outline bis 30), Körpermitte ~x=15.
// Waffen werden NICHT in den Körper gezeichnet (Metal-Slug-Waffenpickups): jeder Frame exportiert
// stattdessen den Hand-Anker, an den das Spiel die Waffe aus weapons.png hängt.
import { Pix, type Px } from './png.js';
import type { ColorName } from './palette.js';

export type HairStyle = 'cap' | 'bald' | 'mohawk' | 'beanie' | 'bandana' | 'hair' | 'longhair' | 'hood' | 'slick' | 'helmet' | 'bun';
export type Aim = 'h' | 'u' | 'd';

export interface Recipe {
  skin: ColorName;
  skinShade: ColorName;
  hair: ColorName;
  hairStyle: HairStyle;
  shirt: ColorName;
  shirtShade: ColorName;
  pants: ColorName;
  shoes: ColorName;
  accent: ColorName;      // Charakterfarbe (Spielermarker, Details)
  hat?: ColorName;        // Farbe für Kappe/Mütze/Bandana/Helm (sonst Haarfarbe)
  glasses?: boolean;      // Sonnenbrille
  beard?: boolean;
  apron?: boolean;        // Schürze (Willi)
  vest?: ColorName;       // Weste/Streifen über dem Shirt
  tie?: ColorName;        // Krawatte (Mooks)
  tattoo?: boolean;
}

export const FRAME = 32;
/** Kollisionsbox innerhalb des Frames (Blick nach rechts), stehend / geduckt. */
export const BODY_BOX = { x: 11, y: 8, w: 10, h: 22 };
export const CROUCH_BOX = { x: 11, y: 13, w: 10, h: 17 };

export interface Pose {
  bob: number;          // vertikaler Versatz von Kopf+Torso+Armen
  legs: { back: [number, number, number]; front: [number, number, number] }; // [dxHüfte, dxFuß, Verkürzung]
  aim: Aim;
  melee?: 0 | 1 | 2;    // Nahkampf-Phase
  armsUp?: boolean;     // Panik (Mooks)
  crouch?: boolean;
}

export interface Rendered { pix: Pix; hand: [number, number] | null }

export function darker(c: ColorName): ColorName {
  const map: Partial<Record<ColorName, ColorName>> = {
    blue: 'blueDark', navy: 'night', slate: 'navy', concrete: 'slate', brown: 'soilDark', soil: 'soilDark',
    red: 'blood', purple: 'navy', bvgYellow: 'orange', beige: 'brown', night: 'ink', steel: 'steelDark',
    moss: 'mossDark', cream: 'beige', ash: 'concrete', neonPink: 'purple', skin: 'skinShade', orange: 'brickLight',
    neonCyan: 'steel', gold: 'orange', white: 'fog', fog: 'ash', blueDark: 'night', brick: 'brickDark',
  };
  return map[c] ?? 'ink';
}

export function drawHumanoid(r: Recipe, pose: Pose): Rendered {
  const p = new Pix(FRAME, FRAME);
  const bob = pose.bob;
  const legTop = 22;

  const drawLeg = (dxHip: number, dxFoot: number, shorten: number, shade: boolean) => {
    const pants: Px = shade ? darker(r.pants) : r.pants;
    const h = 6 - shorten;
    const x0 = 13 + dxHip, x1 = 13 + dxFoot;
    const upper = Math.max(1, (h / 2) | 0);
    p.rect(x0, legTop, 3, upper, pants);
    p.rect(x1, legTop + upper, 3, h - upper, pants);
    p.rect(x1, legTop + h, 4, 2, r.shoes);        // Schuh
    p.set(x1 + 3, legTop + h + 1, 'ink');
  };
  const drawCrouchLegs = () => {
    p.rect(10, 26, 4, 2, darker(r.pants)); p.rect(9, 28, 4, 2, r.shoes);
    p.rect(16, 26, 4, 2, r.pants); p.rect(18, 28, 4, 2, r.shoes); p.set(21, 29, 'ink');
  };

  if (pose.crouch) drawCrouchLegs();
  else drawLeg(pose.legs.back[0], pose.legs.back[1], pose.legs.back[2], true);

  // Hinterer Arm
  const armY = 16 + bob;
  if (pose.armsUp) {
    p.rect(10, armY - 7, 2, 7, r.shirtShade);
    p.rect(10, armY - 9, 2, 2, r.skinShade);
  } else {
    p.rect(10, armY, 2, 5, r.shirtShade);
    p.rect(10, armY + 5, 2, 2, r.skinShade);
  }

  // Torso
  const torsoY = 15 + bob;
  p.rect(11, torsoY, 8, 7, r.shirt);
  p.vline(11, torsoY, 7, r.shirtShade);
  p.hline(12, torsoY + 6, 7, r.shirtShade);
  if (r.vest) { p.rect(12, torsoY, 2, 7, r.vest); p.rect(17, torsoY, 2, 7, r.vest); }
  if (r.apron) { p.rect(13, torsoY + 1, 5, 6, 'cream'); p.hline(13, torsoY + 1, 5, 'beige'); p.set(15, torsoY, 'cream'); }
  if (r.tie) { p.vline(15, torsoY + 1, 4, r.tie); p.hline(14, torsoY, 3, 'white'); }
  if (r.tattoo) { p.set(10, armY + 2, r.accent); p.set(11, armY + 3, r.accent); }

  if (!pose.crouch) drawLeg(pose.legs.front[0], pose.legs.front[1], pose.legs.front[2], false);

  // Kopf
  const headY = 8 + bob;
  p.rect(11, headY, 8, 7, r.skin);
  p.vline(11, headY + 1, 6, r.skinShade);
  p.hline(12, headY + 6, 6, r.skinShade);
  p.set(13, headY + 6, r.skin);
  if (r.glasses) { p.rect(14, headY + 3, 5, 1, 'ink'); p.set(16, headY + 3, 'navy'); p.set(18, headY + 3, 'navy'); }
  else { p.set(17, headY + 3, 'ink'); p.set(16, headY + 3, 'white'); }
  if (r.beard) { p.rect(13, headY + 5, 6, 2, r.hair); p.set(18, headY + 6, r.hair); }
  drawHair(p, r, headY);

  // Vorderer Arm; Hand-Anker für die Waffe
  let hand: [number, number] | null = null;
  if (pose.armsUp) {
    p.rect(17, armY - 7, 2, 8, r.shirt);
    p.rect(17, armY - 9, 2, 2, r.skin);
  } else if (pose.melee !== undefined) {
    const phase = pose.melee;
    if (phase === 0) { p.rect(17, armY - 4, 2, 6, r.shirt); p.rect(17, armY - 6, 2, 2, r.skin); p.vline(18, armY - 9, 3, 'fog'); }
    if (phase === 1) { p.rect(17, armY + 1, 7, 2, r.shirt); p.rect(23, armY + 1, 2, 2, r.skin); p.hline(25, armY + 1, 5, 'fog'); p.set(29, armY, 'white'); p.set(29, armY + 2, 'white'); }
    if (phase === 2) { p.rect(17, armY + 1, 5, 2, r.shirt); p.rect(21, armY + 1, 2, 2, r.skin); p.hline(23, armY + 1, 3, 'fog'); }
  } else if (pose.aim === 'h') {
    p.rect(17, armY + 1, 5, 2, r.shirt); hand = [22, armY + 1];
  } else if (pose.aim === 'u') {
    p.rect(17, armY - 6, 2, 8, r.shirt); hand = [17, armY - 8];
  } else {
    p.rect(17, armY + 2, 2, 7, r.shirt); hand = [17, armY + 9];
  }
  if (hand) { p.rect(hand[0], hand[1], 2, 2, r.skin); }

  p.outline('ink');
  return { pix: p, hand };
}

function drawHair(p: Pix, r: Recipe, headY: number): void {
  const hatStyles: HairStyle[] = ['cap', 'beanie', 'bandana', 'helmet'];
  const h = hatStyles.includes(r.hairStyle) ? (r.hat ?? r.hair) : r.hair;
  switch (r.hairStyle) {
    case 'cap':
      p.rect(11, headY - 2, 8, 3, h);
      p.rect(19, headY, 3, 1, h);
      p.set(11, headY - 2, darker(h));
      p.hline(12, headY, 7, darker(h));
      p.set(15, headY - 1, r.accent);
      break;
    case 'bald':
      p.hline(12, headY, 6, r.skinShade);
      p.set(13, headY, 'white');
      break;
    case 'mohawk':
      p.rect(11, headY, 8, 1, r.skinShade);
      p.rect(13, headY - 4, 3, 5, h);
      p.set(14, headY - 5, h); p.set(13, headY - 3, r.accent);
      break;
    case 'beanie':
      p.rect(11, headY - 3, 8, 4, h);
      p.hline(11, headY, 8, darker(h));
      p.set(14, headY - 4, h); p.set(15, headY - 4, h);
      p.set(15, headY - 2, r.accent);
      break;
    case 'bandana':
      p.rect(11, headY - 1, 8, 2, h);
      p.rect(8, headY, 3, 1, h); p.rect(8, headY + 1, 2, 1, h);
      p.set(16, headY - 1, r.accent);
      p.hline(12, headY + 1, 6, darker(h));
      break;
    case 'hair':
      p.rect(11, headY - 2, 8, 3, h);
      p.rect(10, headY, 1, 3, h);
      p.set(12, headY - 2, 0); p.set(18, headY - 2, 0);
      break;
    case 'longhair':
      p.rect(11, headY - 2, 8, 3, h);
      p.rect(9, headY, 2, 9, h);
      p.rect(11, headY + 1, 1, 6, h);
      p.set(18, headY - 2, 0);
      break;
    case 'hood':
      p.rect(10, headY - 3, 10, 4, r.shirt);
      p.rect(9, headY, 2, 8, r.shirt);
      p.rect(19, headY + 1, 1, 6, r.shirt);
      p.hline(12, headY + 1, 6, r.skinShade);
      p.set(11, headY - 3, 0); p.set(19, headY - 3, 0);
      break;
    case 'slick':
      p.rect(11, headY - 1, 8, 2, h);
      p.rect(10, headY, 1, 3, h);
      p.set(18, headY - 1, 0); p.set(19, headY, h);
      break;
    case 'helmet':
      p.rect(10, headY - 3, 10, 5, 'steelDark');
      p.hline(11, headY - 3, 7, 'steel');
      p.rect(10, headY + 1, 10, 1, 'ink');
      p.set(15, headY - 1, r.accent);
      break;
    case 'bun':
      p.rect(11, headY - 2, 8, 3, h);
      p.rect(9, headY - 3, 3, 3, h);
      p.set(18, headY - 2, 0);
      break;
  }
}

export type AnimName = 'idle' | 'run' | 'jump' | 'fall' | 'crouch' | 'melee' | 'panic';

export function poseFor(anim: AnimName, frame: number, aim: Aim): Pose {
  switch (anim) {
    case 'idle': {
      const bob = frame >= 2 ? 1 : 0;
      return { bob, aim, legs: { back: [-1, -1, 0], front: [1, 1, 0] } };
    }
    case 'run': {
      const ph = (frame / 8) * Math.PI * 2;
      const f = Math.round(Math.sin(ph) * 3);
      const b = Math.round(Math.sin(ph + Math.PI) * 3);
      const bob = frame % 4 === 1 || frame % 4 === 2 ? -1 : 0;
      const fs = Math.sin(ph) > 0.3 ? 2 : 0, bs = Math.sin(ph + Math.PI) > 0.3 ? 2 : 0;
      return { bob, aim, legs: { back: [Math.round(b / 2), b, bs], front: [Math.round(f / 2), f, fs] } };
    }
    case 'jump':
      return { bob: -1, aim, legs: { back: [-1, -2, 2], front: [1, 3, 3] } };
    case 'fall':
      return { bob: 0, aim, legs: { back: [-2, -3, 0], front: [1, 2, 1] } };
    case 'crouch':
      return { bob: 4, aim, crouch: true, legs: { back: [0, 0, 0], front: [0, 0, 0] } };
    case 'melee':
      return { bob: frame === 1 ? 1 : 0, aim: 'h', melee: frame as 0 | 1 | 2, legs: { back: [-1, -2, 0], front: [1, 3, 1] } };
    case 'panic': {
      const f = frame % 2 === 0 ? 3 : -3;
      return { bob: frame % 2, aim: 'h', armsUp: true, legs: { back: [Math.round(-f / 2), -f, frame % 2 ? 2 : 0], front: [Math.round(f / 2), f, frame % 2 ? 0 : 2] } };
    }
  }
}

export interface SheetFrame { name: string; pix: Pix; hand: [number, number] | null }

/** Baut die komplette Frame-Liste für eine Figur. */
export function buildCharacterFrames(r: Recipe, withPanic = false): SheetFrame[] {
  const frames: SheetFrame[] = [];
  const add = (anim: AnimName, count: number, aim: Aim) => {
    for (let i = 0; i < count; i++) {
      const d = drawHumanoid(r, poseFor(anim, i, aim));
      frames.push({ name: `${anim}_${aim}_${i}`, pix: d.pix, hand: d.hand });
    }
  };
  for (const aim of ['h', 'u', 'd'] as Aim[]) {
    add('idle', 4, aim); add('run', 8, aim); add('jump', 1, aim); add('fall', 1, aim);
  }
  add('crouch', 1, 'h'); add('crouch', 1, 'u');
  add('melee', 3, 'h');
  if (withPanic) add('panic', 4, 'h');

  const full = drawHumanoid(r, poseFor('idle', 0, 'h')).pix;
  const portrait = new Pix(16, 16);
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) portrait.set(x, y, full.get(x + 7, y + 3));
  frames.push({ name: 'portrait', pix: portrait, hand: null });
  const head = new Pix(10, 10); for (let y = 0; y < 10; y++) for (let x = 0; x < 10; x++) head.set(x, y, full.get(x + 10, y + 5));
  frames.push({ name: 'gib_head', pix: head, hand: null });
  const torso = new Pix(8, 6); torso.rect(1, 1, 6, 4, r.shirt); torso.set(2, 2, r.shirtShade); torso.outline('ink');
  frames.push({ name: 'gib_torso', pix: torso, hand: null });
  const leg = new Pix(5, 8); leg.rect(1, 1, 3, 5, r.pants); leg.rect(1, 5, 3, 2, r.shoes); leg.outline('ink');
  frames.push({ name: 'gib_leg', pix: leg, hand: null });
  return frames;
}
