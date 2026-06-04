// Shunt — the browser side of the boundary. We own the clock and choose the
// canvas size; the wasm owns the rules. We tick the app on a fixed clock and
// send clicks straight in, then mirror its framebuffer to the canvas. Loaded
// as a module from templates/shunt.html.

const TPS = 60;                          // ticks per second — our fixed clock
const MS_PER_TICK = 1000 / TPS;
const MAX_CATCHUP = 5;                   // cap ticks/frame so a stalled tab can't spiral

const canvas = document.getElementById('screen');
const ctx = canvas.getContext('2d');

const bytes = await(await fetch('/shunt/shunt.wasm')).arrayBuffer();
const { instance } = await WebAssembly.instantiate(bytes, {});
const wasm = instance.exports;

// The seed makes each game unique-ish but reproducible. Logged so you can
// replay a run; override with ?seed=123 in the URL to reproduce one.
const seed = (Number(new URLSearchParams(location.search).get('seed')) || (Math.random() * 0xffffffff)) >>> 0;
console.log('Shunt seed:', seed);

wasm.init(seed);
const w = wasm.fb_width(), h = wasm.fb_height();   // actual size the app accepted (post-clamp)
canvas.width = w; canvas.height = h;
const ptr = wasm.framebuffer();          // address of fb[] in wasm linear memory

// Click -> straight into the app, the instant it lands. No queue, no batching.
canvas.addEventListener('click', e => {
  const r = canvas.getBoundingClientRect();
  const x = Math.floor((e.clientX - r.left) / r.width * w);
  const y = Math.floor((e.clientY - r.top) / r.height * h);
  wasm.click(x, y);
});

function blit() {
  // Re-view each frame: if wasm memory ever grows, an old view detaches and
  // reads come back empty. Cheap to recreate, so we don't cache it.
  const fb = new Uint8Array(wasm.memory.buffer, ptr, w * h * 4);
  ctx.putImageData(new ImageData(new Uint8ClampedArray(fb), w, h), 0, 0);
}

// Fixed timestep: run as many ticks as real elapsed time owes, so game speed
// is decoupled from display refresh rate.
let acc = 0, last = performance.now();
function frame(now) {
  acc += now - last; last = now;
  if (acc > MAX_CATCHUP * MS_PER_TICK) acc = MAX_CATCHUP * MS_PER_TICK;
  while (acc >= MS_PER_TICK) {
    wasm.tick();
    acc -= MS_PER_TICK;
  }
  blit();
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
