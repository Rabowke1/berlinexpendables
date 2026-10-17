// Spielbare Bros, Pickup-Waffen und Gegnerprofile. Werte in px/Frame bei 60 Hz.
import type { SfxName } from '../engine/audio';

export type Aim = 'h' | 'u' | 'd';
export type PickupKind = 'heavy' | 'rocket' | 'flame' | 'laser';

export interface BulletProfile {
  sprite: string;      // Frame/Animation im fx-Atlas
  speed: number;
  life: number;
  dmg: number;
  tileDmg: number;
  pierce: number;      // wie viele Gegner durchschlagen werden
  gravity: number;
  lob: number;         // zusätzlicher Aufwärtsimpuls (Wurf)
  explode: number;     // Explosionsradius bei Aufprall (0 = keine)
}
export interface WeaponProfile {
  sprite: string;      // Waffensprite in weapons.png (ohne _h/_u/_d)
  cooldown: number;
  count: number;       // Projektile pro Schuss
  spread: number;      // Streuung in rad
  recoil: number;
  shake: number;
  sfx: SfxName;
  bullet: BulletProfile;
}
export interface CharacterData {
  id: string;
  name: string;
  weapon: WeaponProfile;
  grenadeRadius: number;
  grenades: number;
  tagline: string;
}

const B = (p: Partial<BulletProfile> & { sprite: string; speed: number }): BulletProfile =>
  ({ life: 60, dmg: 1, tileDmg: 1, pierce: 0, gravity: 0, lob: 0, explode: 0, ...p });

export const PLAYER_CHARS: CharacterData[] = [
  {
    id: 'willi', name: 'CURRYWURST WILLI', tagline: 'MIT ALLES UND SCHARF!', grenades: 3, grenadeRadius: 34,
    weapon: { sprite: 'shotgun', cooldown: 26, count: 5, spread: 0.26, recoil: 1.4, shake: 0.25, sfx: 'shotgun', bullet: B({ sprite: 'pellet', speed: 5.5, life: 22 }) },
  },
  {
    id: 'doro', name: 'TECHNODORE', tagline: 'DU KOMMST HIER NICHT REIN.', grenades: 3, grenadeRadius: 30,
    weapon: { sprite: 'bass', cooldown: 18, count: 1, spread: 0, recoil: 0.4, shake: 0.12, sfx: 'bass', bullet: B({ sprite: 'wave', speed: 4.5, life: 60, pierce: 3, tileDmg: 2 }) },
  },
  {
    id: 'berti', name: 'BVG-BERTI', tagline: 'ZURUECKBLEIBEN BITTE!', grenades: 3, grenadeRadius: 30,
    weapon: { sprite: 'smg', cooldown: 5, count: 1, spread: 0.08, recoil: 0.15, shake: 0.05, sfx: 'smg', bullet: B({ sprite: 'smgbullet', speed: 8, life: 50 }) },
  },
  {
    id: 'sven', name: 'SPAETI-SVEN', tagline: 'PFAND IST NICHT DRIN.', grenades: 3, grenadeRadius: 30,
    weapon: { sprite: 'bottle', cooldown: 24, count: 1, spread: 0.04, recoil: 0.2, shake: 0.08, sfx: 'throw', bullet: B({ sprite: 'bottle', speed: 4.8, life: 140, gravity: 0.17, lob: 2.6, explode: 22, tileDmg: 1 }) },
  },
];

export const PICKUPS: Record<PickupKind, { name: string; ammo: number; weapon: WeaponProfile }> = {
  heavy: { name: 'HEAVY MACHINE GUN', ammo: 200, weapon: { sprite: 'heavy', cooldown: 4, count: 1, spread: 0.12, recoil: 0.25, shake: 0.08, sfx: 'smg', bullet: B({ sprite: 'heavybullet', speed: 9, dmg: 2, tileDmg: 2, life: 60 }) } },
  rocket: { name: 'ROCKET LAUNCHER', ammo: 15, weapon: { sprite: 'rocket', cooldown: 30, count: 1, spread: 0, recoil: 1.0, shake: 0.2, sfx: 'throw', bullet: B({ sprite: 'rocket', speed: 5, life: 90, explode: 38, tileDmg: 3 }) } },
  flame: { name: 'FLAME SHOT', ammo: 150, weapon: { sprite: 'flame', cooldown: 3, count: 1, spread: 0.15, recoil: 0.1, shake: 0.03, sfx: 'dig', bullet: B({ sprite: 'flame', speed: 3.6, life: 24, pierce: 99, tileDmg: 0.35, gravity: -0.03 }) } },
  laser: { name: 'LASER SHOT', ammo: 200, weapon: { sprite: 'laser', cooldown: 2, count: 1, spread: 0, recoil: 0, shake: 0.04, sfx: 'smg', bullet: B({ sprite: 'laserbeam', speed: 12, life: 40, pierce: 99, tileDmg: 1.5 }) } },
};

export interface EnemyData { id: string; atlas: string; weapon: string; burst: number; burstGap: number; cooldown: [number, number]; sight: number; speed: number; score: number; bullet: BulletProfile; hp: number }
export const ENEMIES: Record<'mook' | 'security', EnemyData> = {
  mook: { id: 'mook', atlas: 'mook', weapon: 'pistol', burst: 1, burstGap: 0, cooldown: [60, 110], sight: 150, speed: 0.5, score: 100, hp: 1, bullet: B({ sprite: 'mookbullet', speed: 3.2, life: 110, tileDmg: 0.5 }) },
  security: { id: 'security', atlas: 'security', weapon: 'rifle', burst: 3, burstGap: 7, cooldown: [80, 130], sight: 190, speed: 0.4, score: 150, hp: 2, bullet: B({ sprite: 'mookbullet', speed: 4, life: 110, tileDmg: 0.5 }) },
};
