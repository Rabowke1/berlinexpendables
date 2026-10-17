import { VIEW_W, VIEW_H, type Renderer } from '../engine/renderer';
import type { World } from './world';
import { PLAYER_LABELS, MAX_PLAYERS } from './constants';
import { PICKUPS } from './characters';

export function drawHud(r: Renderer, w: World): void {
  const c = r.ctx;
  // Spielerkarten
  w.players.forEach((p, i) => {
    const x = 4 + i * 118, y = 4;
    c.fillStyle = 'rgba(11,10,18,0.6)'; c.fillRect(x, y, 112, 22);
    r.rect(x, y, 2, 22, p.color);
    const a = w.assets.atlas(p.char.id);
    r.sprite(a.image, a.frames['portrait'], x + 4, y + 3);
    r.label(PLAYER_LABELS[p.slot], x + 22, y + 2, p.color);
    r.label(p.char.name.split(' ').pop()!, x + 38, y + 2, '#d8d6e4');
    if (!p.alive) {
      r.label(w.lives > 0 ? `RESPAWN ${Math.ceil(p.deadTimer / 60)}` : 'TOT', x + 22, y + 12, '#9d9bb0');
    } else {
      const fx = w.assets.atlas('fx');
      for (let g = 0; g < Math.min(p.grenades, 6); g++) r.sprite(fx.image, fx.frames['grenade'], x + 22 + g * 7, y + 12);
      if (p.grenades > 6) r.label(`x${p.grenades}`, x + 22 + 6 * 7 + 2, y + 13, '#d8d6e4');
      if (p.pickup) r.label(`${PICKUPS[p.pickup.kind].name.split(' ')[0][0]} ${p.pickup.ammo}`, x + 70, y + 13, '#ffcf5c');
    }
  });
  // Leben & Punkte
  r.label(`LEBEN ${w.lives}`, VIEW_W / 2, 30, w.lives <= 1 ? '#e23b3b' : '#ffffff', 'center');
  r.label(`${String(w.score).padStart(7, '0')}`, VIEW_W - 4, 30, '#ffe678', 'right');
  // Join-Hinweis
  if (w.players.length < MAX_PLAYERS && w.state === 'play' && (w.tick % 120) < 90) {
    const txt = w.players.length === 0 ? 'SPRUNG ODER START DRUECKEN ZUM MITSPIELEN' : `P${w.players.length + 1}: SPRUNG/START DRUECKEN ZUM BEITRETEN`;
    r.label(txt, VIEW_W / 2, VIEW_H - 12, '#9d9bb0', 'center');
  }
  // Namens-Banner bei Spawn
  for (const p of w.players) {
    if (p.banner <= 0 || !p.alive) continue;
    const t = p.banner;
    const slide = t > 130 ? (t - 130) * 12 : t < 20 ? (20 - t) * 12 : 0;
    const y = 60 + p.slot * 2;
    c.fillStyle = 'rgba(11,10,18,0.7)'; c.fillRect(0, y - 4, VIEW_W, 30);
    r.rect(0, y - 4, VIEW_W, 1, p.color); r.rect(0, y + 25, VIEW_W, 1, p.color);
    r.label(p.char.name, VIEW_W / 2 + slide, y, p.color, 'center', 2);
    r.label(p.char.tagline, VIEW_W / 2 - slide, y + 16, '#d8d6e4', 'center');
  }
  // Titel, solange niemand spielt
  if (w.players.length === 0) {
    c.fillStyle = 'rgba(11,10,18,0.55)'; c.fillRect(0, 70, VIEW_W, 90);
    r.label('BERLIN', VIEW_W / 2, 80, '#ffcf5c', 'center', 4);
    r.label('EXPENDABLES', VIEW_W / 2, 112, '#ffffff', 'center', 3);
    r.label(w.levelName, VIEW_W / 2, 140, '#9d9bb0', 'center');
    c.fillStyle = 'rgba(11,10,18,0.55)'; c.fillRect(0, VIEW_H - 46, VIEW_W, 34);
    r.label('P1 TASTATUR: WASD + J FEUER  K SPRUNG  L GRANATE  U NAHKAMPF', VIEW_W / 2, VIEW_H - 42, '#9d9bb0', 'center');
    r.label('P2 TASTATUR: PFEILE + , FEUER  . SPRUNG  / GRANATE  M NAHKAMPF', VIEW_W / 2, VIEW_H - 32, '#9d9bb0', 'center');
    r.label('GAMEPAD: X FEUER  A SPRUNG  Y GRANATE  B NAHKAMPF  START BEITRETEN', VIEW_W / 2, VIEW_H - 22, '#9d9bb0', 'center');
  }
  // Ende
  if (w.state !== 'play') {
    const a = Math.min(0.75, w.stateT / 60);
    c.fillStyle = `rgba(11,10,18,${a})`; c.fillRect(0, 0, VIEW_W, VIEW_H);
    if (w.stateT > 30) {
      const won = w.state === 'won';
      r.label(won ? 'KIEZ BEFREIT!' : 'GAME OVER', VIEW_W / 2, 90, won ? '#7fd04f' : '#e23b3b', 'center', 3);
      r.label(`PUNKTE ${w.score}`, VIEW_W / 2, 125, '#ffe678', 'center', 2);
      const best = w.players.slice().sort((p, q) => q.kills - p.kills)[0];
      if (best) r.label(`MVP: ${PLAYER_LABELS[best.slot]} ${best.char.name} (${best.kills} KILLS)`, VIEW_W / 2, 150, best.color, 'center');
      if (w.stateT > 90 && (w.tick % 60) < 40) r.label('START/SPRUNG = NOCHMAL', VIEW_W / 2, 180, '#d8d6e4', 'center');
    }
  }
}
