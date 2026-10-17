// "Berlin Nacht" – die eine Palette für das ganze Spiel (siehe docs/STYLEGUIDE.md).
export const PALETTE = {
  ink:        [11, 10, 18],     // Outline / tiefstes Schwarz
  night:      [27, 26, 46],     // Nachthimmel, Anzug dunkel
  navy:       [46, 44, 74],
  slate:      [74, 72, 102],    // Beton Schatten
  concrete:   [110, 108, 135],  // Beton
  ash:        [157, 155, 176],  // Beton hell
  fog:        [216, 214, 228],  // Fast weiß
  white:      [255, 255, 255],
  brickDark:  [90, 45, 45],
  brick:      [140, 59, 59],
  brickLight: [196, 92, 74],
  soilDark:   [58, 42, 30],
  soil:       [107, 74, 47],
  soilLight:  [163, 122, 79],
  mossDark:   [31, 74, 42],
  moss:       [63, 138, 58],
  mossLight:  [127, 208, 79],
  skin:       [241, 194, 125],
  skinShade:  [201, 138, 90],
  skinDark:   [128, 84, 52],
  bvgYellow:  [255, 207, 92],
  orange:     [255, 140, 42],   // Natriumdampflampen, Currywurst
  red:        [226, 59, 59],
  blood:      [157, 27, 27],
  blue:       [47, 128, 237],
  blueDark:   [27, 78, 166],
  neonPink:   [255, 63, 164],
  neonCyan:   [56, 240, 230],
  purple:     [163, 92, 255],
  beige:      [217, 180, 143],
  brown:      [92, 59, 30],
  cream:      [245, 230, 200],
  steel:      [135, 167, 199],
  steelDark:  [70, 94, 120],
  glass:      [120, 200, 230],
  gold:       [255, 230, 120],
} as const satisfies Record<string, readonly [number, number, number]>;

export type ColorName = keyof typeof PALETTE;
export const COLOR_NAMES = Object.keys(PALETTE) as ColorName[];

export function hex(c: ColorName): string {
  const [r, g, b] = PALETTE[c];
  return '#' + [r, g, b].map((v) => v.toString(16).padStart(2, '0')).join('');
}
