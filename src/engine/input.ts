// Eingabe für bis zu 4 Spieler: 2 Tastatur-Layouts + bis zu 4 Gamepads. Join per Sprung/Start.
export type DeviceId = 'kb1' | 'kb2' | `pad${number}`;

export interface Buttons {
  left: boolean; right: boolean; up: boolean; down: boolean;
  jump: boolean; fire: boolean; special: boolean; melee: boolean; start: boolean;
}
export interface PlayerInput extends Buttons {
  jumpPressed: boolean; firePressed: boolean; specialPressed: boolean; meleePressed: boolean; startPressed: boolean;
}

const EMPTY: Buttons = { left: false, right: false, up: false, down: false, jump: false, fire: false, special: false, melee: false, start: false };

const KB: Record<'kb1' | 'kb2', Record<keyof Buttons, string[]>> = {
  kb1: { left: ['KeyA'], right: ['KeyD'], up: ['KeyW'], down: ['KeyS'], fire: ['KeyJ'], jump: ['KeyK', 'Space'], special: ['KeyL'], melee: ['KeyU', 'KeyI'], start: ['Enter'] },
  kb2: { left: ['ArrowLeft'], right: ['ArrowRight'], up: ['ArrowUp'], down: ['ArrowDown'], fire: ['Comma', 'Numpad1'], jump: ['Period', 'Numpad2'], special: ['Slash', 'Numpad3'], melee: ['KeyM', 'Numpad0'], start: ['NumpadEnter', 'ShiftRight'] },
};

export class Input {
  private keys = new Set<string>();
  private prev = new Map<DeviceId, Buttons>();
  private cur = new Map<DeviceId, Buttons>();
  /** Tasten, die genau in diesem Frame gedrückt wurden (für Debug/Menü). */
  pressedKeys = new Set<string>();
  private pendingKeys = new Set<string>();
  onAnyKey: (() => void) | null = null;

  constructor() {
    window.addEventListener('keydown', (e) => {
      if (['Space', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Slash', 'Tab'].includes(e.code) || e.code.startsWith('F')) e.preventDefault();
      if (!this.keys.has(e.code)) this.pendingKeys.add(e.code);
      this.keys.add(e.code);
      this.onAnyKey?.();
    });
    window.addEventListener('keyup', (e) => this.keys.delete(e.code));
    window.addEventListener('blur', () => this.keys.clear());
  }

  /** Einmal pro Simulationsschritt aufrufen. */
  poll(): void {
    this.pressedKeys = this.pendingKeys; this.pendingKeys = new Set();
    this.prev = this.cur; this.cur = new Map();
    for (const id of ['kb1', 'kb2'] as const) {
      const m = KB[id]; const b: Buttons = { ...EMPTY };
      // Auch Tasten zählen, die zwischen zwei Schritten kurz gedrückt wurden (sehr kurze Tipper gehen sonst verloren)
      (Object.keys(m) as (keyof Buttons)[]).forEach((k) => { b[k] = m[k].some((code) => this.keys.has(code) || this.pressedKeys.has(code)); });
      this.cur.set(id, b);
    }
    const pads = typeof navigator.getGamepads === 'function' ? navigator.getGamepads() : [];
    for (let i = 0; i < pads.length && i < 4; i++) {
      const p = pads[i]; if (!p) continue;
      const bt = (n: number) => !!p.buttons[n]?.pressed;
      const ax = p.axes[0] ?? 0, ay = p.axes[1] ?? 0;
      this.cur.set(`pad${i}`, {
        left: ax < -0.5 || bt(14), right: ax > 0.5 || bt(15), up: ay < -0.5 || bt(12), down: ay > 0.5 || bt(13),
        jump: bt(0), fire: bt(2) || bt(7), special: bt(3) || bt(5), melee: bt(1) || bt(4) || bt(6), start: bt(9),
      });
    }
  }

  get devices(): DeviceId[] { return [...this.cur.keys()]; }

  read(id: DeviceId): PlayerInput {
    const c = this.cur.get(id) ?? EMPTY, p = this.prev.get(id) ?? EMPTY;
    return {
      ...c,
      jumpPressed: c.jump && !p.jump, firePressed: c.fire && !p.fire, specialPressed: c.special && !p.special,
      meleePressed: c.melee && !p.melee, startPressed: c.start && !p.start,
    };
  }

  /** Geräte, auf denen gerade Sprung oder Start neu gedrückt wurde. */
  joinRequests(): DeviceId[] {
    return this.devices.filter((d) => { const r = this.read(d); return r.jumpPressed || r.startPressed; });
  }
  anyStartPressed(): boolean { return this.devices.some((d) => this.read(d).startPressed || this.read(d).jumpPressed); }
}
