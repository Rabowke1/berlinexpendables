// Fester 60-Hz-Simulationsschritt, Rendering pro Display-Frame.
export const STEP = 1 / 60;

export function startLoop(update: () => void, render: () => void): void {
  let last = performance.now(), acc = 0;
  const frame = (now: number) => {
    acc += Math.min(0.1, (now - last) / 1000);
    last = now;
    let n = 0;
    while (acc >= STEP && n < 5) { update(); acc -= STEP; n++; }
    if (n === 5) acc = 0;
    render();
    requestAnimationFrame(frame);
  };
  requestAnimationFrame(frame);
}
