export const T = 16;                 // Tilegröße
export const GRAV = 0.32;
export const MAX_FALL = 6.5;
export const RUN = 1.7;
export const ACC = 0.35;
export const ACC_AIR = 0.2;
export const JUMP = -5.9;
export const COYOTE = 6;
export const JUMP_BUFFER = 6;
export const START_LIVES = 5;
export const RESPAWN_FRAMES = 80;
export const INVULN_FRAMES = 120;
export const MAX_PLAYERS = 4;

export const PLAYER_COLORS = ['#2f80ed', '#e23b3b', '#7fd04f', '#ffcf5c'];
export const PLAYER_LABELS = ['P1', 'P2', 'P3', 'P4'];

export enum Tile { AIR = 0, DIRT = 1, BRICK = 2, STEEL = 3, CONCRETE = 4, PLATFORM = 5, CRATE = 6 }
export const TILE_HP: Record<Tile, number> = { [Tile.AIR]: 0, [Tile.DIRT]: 3, [Tile.BRICK]: 6, [Tile.STEEL]: Infinity, [Tile.CONCRETE]: 9, [Tile.PLATFORM]: 4, [Tile.CRATE]: 2 };
export const TILE_DEBRIS: Record<Tile, string[]> = {
  [Tile.AIR]: [], [Tile.DIRT]: ['soil', 'soilDark', 'soilLight'], [Tile.BRICK]: ['brick', 'brickDark', 'brickLight'], [Tile.STEEL]: ['steel'],
  [Tile.CONCRETE]: ['concrete', 'slate', 'ash'], [Tile.PLATFORM]: ['slate', 'ash'], [Tile.CRATE]: ['brown', 'soilLight'],
};
export enum Back { NONE = 0, PLASTER = 1, WINDOW = 2, BRICK = 3, GRAFFITI = 4 }
export const BACK_FRAME: Record<Back, string> = { [Back.NONE]: '', [Back.PLASTER]: 'back_plaster', [Back.WINDOW]: 'back_window', [Back.BRICK]: 'back_brick', [Back.GRAFFITI]: 'back_graffiti' };
