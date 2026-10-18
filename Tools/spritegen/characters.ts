// Rezepte der Bros und Gegner. Palette-Swaps auf demselben Rig = konsistenter Look.
import type { Recipe } from './humanoid.js';
import type { WeaponKind } from './weapons.js';

export interface CharacterDef {
  id: string;
  name: string;
  recipe: Recipe;
  weapon: WeaponKind;
  player: boolean;
}

export const CHARACTERS: CharacterDef[] = [
  {
    id: 'willi', name: 'CURRYWURST WILLI', player: true, weapon: 'shotgun',
    recipe: { skin: 'skin', skinShade: 'skinShade', hair: 'brown', hat: 'red', hairStyle: 'cap', beard: true,
      shirt: 'fog', shirtShade: 'ash', pants: 'navy', shoes: 'brown', accent: 'red', apron: true },
  },
  {
    id: 'doro', name: 'TECHNODORE', player: true, weapon: 'bass',
    recipe: { skin: 'beige', skinShade: 'skinShade', hair: 'neonPink', hairStyle: 'mohawk', glasses: true, tattoo: true,
      shirt: 'night', shirtShade: 'navy', pants: 'navy', shoes: 'night', accent: 'neonPink' },
  },
  {
    id: 'berti', name: 'BVG-BERTI', player: true, weapon: 'smg',
    recipe: { skin: 'skinDark', skinShade: 'brown', hair: 'ink', hat: 'bvgYellow', hairStyle: 'cap', beard: true,
      shirt: 'blueDark', shirtShade: 'night', pants: 'navy', shoes: 'night', accent: 'bvgYellow', tie: 'bvgYellow' },
  },
  {
    id: 'sven', name: 'SPAETI-SVEN', player: true, weapon: 'bottle',
    recipe: { skin: 'skin', skinShade: 'skinShade', hair: 'gold', hairStyle: 'hood',
      shirt: 'purple', shirtShade: 'navy', pants: 'slate', shoes: 'fog', accent: 'neonCyan' },
  },
  {
    id: 'mook', name: 'ANZUG-SCHERGE', player: false, weapon: 'pistol',
    recipe: { skin: 'skin', skinShade: 'skinShade', hair: 'night', hairStyle: 'slick', glasses: true,
      shirt: 'navy', shirtShade: 'night', pants: 'navy', shoes: 'night', accent: 'red', tie: 'red' },
  },
  {
    id: 'security', name: 'SECURITY', player: false, weapon: 'rifle',
    recipe: { skin: 'beige', skinShade: 'skinShade', hair: 'brown', hairStyle: 'bald', glasses: true,
      shirt: 'night', shirtShade: 'ink', pants: 'night', shoes: 'night', accent: 'bvgYellow', vest: 'bvgYellow' },
  },
  {
    id: 'captive', name: 'GEFANGENER', player: false, weapon: 'pistol',
    recipe: { skin: 'skin', skinShade: 'skinShade', hair: 'brown', hairStyle: 'hair',
      shirt: 'cream', shirtShade: 'beige', pants: 'brown', shoes: 'soilDark', accent: 'white' },
  },
];
