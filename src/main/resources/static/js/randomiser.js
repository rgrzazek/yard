// Reusable random picker with pluggable canvas animation.
//
// Usage:
//   import { Randomiser } from '/js/randomiser.js';
//   new Randomiser(items, { animation: myFn })
//     .on('result', winner => ...)
//     .start();
//
// items: string[] or { label, value }[]
//
// animation: (canvas, items, winner, done) => void
//   canvas  — full-screen overlay canvas, already in the DOM
//   items   — normalised { label, value }[] list
//   winner  — the pre-chosen winner (same shape)
//   done()  — call when animation ends; Randomiser removes canvas and fires 'result'
//
// The default export spinAnimation is also exported so custom animations
// can reference colours / helpers without reimplementing them.

export class Randomiser {
  constructor(items, options = {}) {
    this.items = items.map(i => typeof i === 'string' ? { label: i, value: i } : i);
    this.animation = options.animation ?? spinAnimation;
    this._onResult = null;
  }

  on(event, fn) {
    if (event === 'result') this._onResult = fn;
    return this;
  }

  start() {
    if (this.items.length === 0) return;
    if (this.items.length === 1) {
      this._onResult?.(this.items[0]);
      return;
    }
    const winner = this.items[Math.floor(Math.random() * this.items.length)];
    const canvas = createOverlay();
    this.animation(canvas, this.items, winner, () => {
      canvas.remove();
      this._onResult?.(winner);
    });
  }
}

function createOverlay() {
  const canvas = document.createElement('canvas');
  Object.assign(canvas.style, {
    position: 'fixed', top: '0', left: '0',
    width: '100vw', height: '100vh',
    zIndex: '9999', cursor: 'pointer'
  });
  canvas.width = window.innerWidth;
  canvas.height = window.innerHeight;
  document.body.appendChild(canvas);
  return canvas;
}

// Shared palette — matches site CSS variables so custom animations can import these
export const CHROME = '#21382d';
export const BG     = '#f5efe0';
export const ACCENT = 'coral';

export function spinAnimation(canvas, items, winner, done) {
  const ctx = canvas.getContext('2d');
  const cx = canvas.width / 2;
  const cy = canvas.height / 2;

  const SPIN_MS   = 2600;
  const SETTLE_MS = 1000;

  let startTime   = null;
  let lastSwitch  = null;
  let currentItem = items[0];
  let phase       = 'spinning'; // 'settling'
  let settleStart = null;
  let raf;

  function easeOut(t) { return 1 - (1 - t) ** 3; }

  function finish() {
    cancelAnimationFrame(raf);
    canvas.removeEventListener('click', onClick);
    done();
  }

  function onClick() {
    if (phase === 'spinning') {
      // Skip to result immediately
      phase = 'settling';
      settleStart = performance.now();
      currentItem = winner;
    } else {
      finish();
    }
  }
  canvas.addEventListener('click', onClick);

  function tick(ts) {
    if (!startTime) { startTime = ts; lastSwitch = ts; }

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = 'rgba(0,0,0,0.78)';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    if (phase === 'spinning') {
      const t        = Math.min((ts - startTime) / SPIN_MS, 1);
      const progress = easeOut(t);

      // Interval between item swaps: 35 ms (fast) → 380 ms (slow)
      const interval = 35 + progress * 345;
      if (ts - lastSwitch > interval) {
        currentItem = items[Math.floor(Math.random() * items.length)];
        lastSwitch  = ts;
      }

      drawSpinPanel(ctx, cx, cy, currentItem.label, progress);

      if (t >= 1) {
        phase       = 'settling';
        settleStart = ts;
        currentItem = winner;
      }
    } else {
      const st = Math.min((ts - settleStart) / SETTLE_MS, 1);
      drawResultPanel(ctx, cx, cy, winner.label, st);
      if (st >= 1) { finish(); return; }
    }

    raf = requestAnimationFrame(tick);
  }

  raf = requestAnimationFrame(tick);
}

const PANEL_W = 480;
const PANEL_H = 110;

function drawSpinPanel(ctx, cx, cy, label, progress) {
  const W = Math.min(PANEL_W, ctx.canvas.width - 60);
  const x = cx - W / 2;
  const y = cy - PANEL_H / 2;

  ctx.fillStyle = 'rgba(255,255,255,0.92)';
  ctx.fillRect(x, y, W, PANEL_H);

  // Text fades in as items slow — opacity tracks slowness
  const alpha = 0.3 + progress * 0.7;
  ctx.fillStyle = `rgba(33,56,45,${alpha})`;
  ctx.font = 'bold 24px sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(clamp(label, 38), cx, cy);

  ctx.fillStyle = 'rgba(33,56,45,0.35)';
  ctx.font = '12px sans-serif';
  ctx.fillText('choosing…', cx, y + PANEL_H + 20);
}

function drawResultPanel(ctx, cx, cy, label, t) {
  const W = Math.min(PANEL_W, ctx.canvas.width - 60);
  const x = cx - W / 2;
  const y = cy - PANEL_H / 2;

  ctx.fillStyle = CHROME;
  ctx.fillRect(x, y, W, PANEL_H);

  // Accent border draws in left-to-right
  ctx.strokeStyle = ACCENT;
  ctx.lineWidth = 3;
  ctx.strokeRect(x, y, W * t, PANEL_H);

  ctx.fillStyle = BG;
  ctx.font = 'bold 26px sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(clamp(label, 38), cx, cy);

  ctx.fillStyle = ACCENT;
  ctx.font = '12px sans-serif';
  ctx.globalAlpha = Math.min(1, t * 3);
  ctx.fillText('click to continue', cx, y + PANEL_H + 20);
  ctx.globalAlpha = 1;
}

function clamp(str, max) {
  return str.length > max ? str.slice(0, max - 1) + '…' : str;
}
