import { Renderer } from './engine/renderer';
import { Assets } from './engine/assets';
import { Input } from './engine/input';
import { Sfx } from './engine/audio';
import { startLoop } from './engine/loop';
import { World } from './game/world';
import { KREUZBERG_01 } from './data/levels/kreuzberg01';
import { PLAYER_CHARS } from './game/characters';

async function boot(): Promise<void> {
  const root = document.getElementById('game')!;
  const renderer = new Renderer(root);
  renderer.clear('#1b1a2e'); renderer.label('LADE KIEZ...', 240, 130, '#ffcf5c', 'center', 2); renderer.present();

  const assets = new Assets();
  await assets.load([...PLAYER_CHARS.map((c) => c.id), 'mook', 'security', 'captive', 'weapons', 'tiles', 'fx'], ['bg_sky', 'bg_skyline', 'bg_mid']);

  const input = new Input();
  const sfx = new Sfx();
  input.onAnyKey = () => sfx.unlock();
  window.addEventListener('pointerdown', () => sfx.unlock());
  window.addEventListener('gamepadconnected', () => sfx.unlock());

  const params = new URLSearchParams(location.search);
  let world = new World(assets, input, sfx, KREUZBERG_01);
  const configure = (w: World) => {
    w.settings.gore = params.get('blut') !== '0';
    w.onRestart = () => { const nw = new World(assets, input, sfx, KREUZBERG_01); configure(nw); const devices = world.players.map((p) => p.device); world = nw; devices.forEach((d) => nw.addPlayer(d)); };
    if (params.get('auto') === '1') w.addPlayer('kb1');     // für Screenshot-Tests
    (window as any).__world = w;
  };
  configure(world);

  startLoop(() => world.update(), () => { world.draw(renderer); renderer.present(); });
}

boot().catch((err) => {
  console.error(err);
  const el = document.createElement('pre');
  el.style.cssText = 'color:#e23b3b;padding:16px;white-space:pre-wrap';
  el.textContent = `Startfehler: ${err instanceof Error ? err.message : String(err)}`;
  document.body.appendChild(el);
});
