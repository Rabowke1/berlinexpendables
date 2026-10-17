// Smoke-Test: baut nichts, erwartet laufenden Server (npm run preview) und macht Screenshots nach docs/screenshots.
// Aufruf: node scripts/shot.mjs [baseUrl]
import { chromium } from 'playwright';
import { mkdirSync } from 'node:fs';

const base = process.argv[2] ?? 'http://127.0.0.1:4173';
mkdirSync('docs/screenshots', { recursive: true });
const browser = await chromium.launch({ executablePath: process.env.CHROMIUM_PATH || undefined });
const page = await browser.newPage({ viewport: { width: 960, height: 540 } });
const errors = [];
page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
page.on('console', (m) => { if (m.type() === 'error') errors.push(`console: ${m.text()}`); });

await page.goto(`${base}/?auto=1`, { waitUntil: 'load' });
await page.waitForFunction(() => window.__world && window.__world.players.length > 0, null, { timeout: 15000 });
const shot = (name) => page.screenshot({ path: `docs/screenshots/${name}.png` });
const state = () => page.evaluate(() => { const w = window.__world; return { tick: w.tick, players: w.players.map((p) => ({ alive: p.alive, x: Math.round(p.x), y: Math.round(p.y), char: p.char.id })), mooks: w.mooks.length, lives: w.lives, score: w.score, state: w.state, bullets: w.bullets.length, particles: w.particles.list.length }; });

await page.waitForTimeout(400);
await shot('01-start');
console.log('start', JSON.stringify(await state()));

// Rennen + schießen
await page.keyboard.down('KeyD');
await page.waitForTimeout(700);
await page.keyboard.down('KeyJ');
await page.waitForTimeout(500);
await shot('02-run-shoot');
await page.keyboard.up('KeyJ');
await page.waitForTimeout(600);
// Springen und Granate werfen
await page.keyboard.press('KeyK');
await page.waitForTimeout(200);
await page.keyboard.press('KeyL');
await page.keyboard.up('KeyD');
await page.waitForTimeout(1250);
await shot('03-explosion');
console.log('after-grenade', JSON.stringify(await state()));
// Spieler 2 (Pfeiltasten-Layout) steigt ein
await page.keyboard.press('Period');
await page.waitForTimeout(300);
await page.keyboard.down('KeyD'); await page.keyboard.down('ArrowRight');
await page.waitForTimeout(900);
await page.keyboard.down('KeyJ'); await page.keyboard.down('Comma');
await page.waitForTimeout(400);
await shot('04-coop');
await page.keyboard.up('KeyJ'); await page.keyboard.up('Comma');
await page.keyboard.press('KeyK'); await page.keyboard.press('Period');
await page.waitForTimeout(1500);
await page.keyboard.up('KeyD'); await page.keyboard.up('ArrowRight');
await page.keyboard.down('KeyS');
await page.waitForTimeout(300);
await shot('05-crouch');
await page.keyboard.up('KeyS');
console.log('end', JSON.stringify(await state()));
await browser.close();
if (errors.length) { console.error('FEHLER:\n' + errors.join('\n')); process.exit(1); }
console.log('Screenshots ok, keine Fehler.');
