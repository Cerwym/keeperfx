# Vita Advanced Port Analysis: vitaQuakeIII & d3es-vita

Companion document to `vita-subsystem-reference.md`. Where that file compares
vitaQuake1/vitaQuake2/KeeperFX, this file documents findings from two more
advanced Vita homebrew ports used as engineering references.

| Codebase        | Root                                      |
|-----------------|-------------------------------------------|
| vitaQuakeIII    | `C:\Users\peter\source\repos\vita\vitaQuakeIII\code\psp2\` |
| d3es-vita       | `C:\Users\peter\source\repos\vita\d3es-vita\neo\` |

---

## 1. Graphics

### 1.1 vitaGL Initialisation

#### vitaQuakeIII — `code/psp2/sys_main.c`

```c
// Quake 3 / OpenArena
vglInitExtended(0, width, height, 0x1800000, SCE_GXM_MULTISAMPLE_4X);
vglUseVram(GL_TRUE);

// Urban Terror (tighter memory budget)
vglInitExtended(0, width, height, 0x1000000, SCE_GXM_MULTISAMPLE_NONE);
vglUseVram(GL_TRUE);
vglUseExtraMem(GL_FALSE);
```

Notable differences from KeeperFX:
- Q3/OA uses a **24 MB RAM pool** (vs KeeperFX's 16 MB) and enables **4× MSAA**.
- Urban Terror drops to 16 MB + no MSAA, showing the trade-off is known and the
  code handles it with a compile-time switch.
- Neither sets `vglSetVertexPoolSize` or `vglSetVDMBufferSize` — those are
  KeeperFX improvements pulled from vitaQuake1 and vitaQuake2 respectively.

#### d3es-vita — `neo/sys/glimp.cpp:176`

```c
vglInitWithCustomThreshold(0,
    glConfig.vidWidthReal,   // 960
    glConfig.vidHeightReal,  // 544
    10 * 1024 * 1024,        // GPU-visible RAM threshold (10 MB)
    0, 0, 0,
    SCE_GXM_MULTISAMPLE_4X);
```

D3 uses `vglInitWithCustomThreshold` — a newer vitaGL API that separately
specifies GPU-visible RAM, CDRAM, and physically contiguous RAM thresholds.
The 10 MB GPU threshold is lower than Q1/Q2/KeeperFX's 16 MB because D3
routes all heap allocations through vitaGL (see §4 Memory Management), so the
threshold only needs to cover GPU-exclusive data rather than a large independent
pool.

---

### 1.2 Shader Approach

| Aspect | vitaQuakeIII | d3es-vita |
|--------|-------------|---------|
| Language | GLSL (OpenGL 1.x compat + GLSL 2.0) | GLSL ES 2.0 |
| Compilation | **Runtime** via `libshacccg.suprx` | **Runtime** GLSL ES |
| Shader programs | Fixed-function + GLSL fallbacks | ~10 named programs (interaction, phong, fog, blend, z-fill, diffuse, cubemap, reflection, stencil shadow) |
| Shader check | `SceCommonDialog` error if `libshacccg.suprx` absent | N/A |

Both depart from KeeperFX's approach of pre-compiled `.gxp` binaries. Runtime
compilation requires `libshacccg.suprx` to be present on the device; it gives
flexibility at the cost of a startup penalty.

---

### 1.3 Texture Formats (d3es-vita)

Doom 3 uses several 16-bit compressed formats to fit a large asset library in
limited VRAM:

```c
// neo/renderer/Image_load.cpp:54-68
GL_RGBA4       // 4 bits per channel — general RGBA textures
GL_RGB5_A1     // 5:5:5:1 — textures with binary alpha
ETC1           // Block compression (OpenGL ES standard, ~6:1 ratio)
               // controlled by r_useETC1 cvar; cached to disk via r_useETC1Cache
```

ETC1-compressed textures are cached to `ux0:data/dhewm3/` on first use,
eliminating the decompression cost on subsequent loads.

**KeeperFX note:** KeeperFX already uses a more aggressive strategy — full
palette-indexed 8-bit rendering with a 256-entry CLUT shader — achieving
75% VRAM savings vs RGBA. ETC1 is not directly applicable, but the
disk-caching principle could be reused for pre-processed asset data.

---

### 1.4 Vertex & Index Buffers (vitaQuakeIII)

```c
// code/renderercommon/qgl.h
#define MAX_INDICES      4096
#define VERTEXARRAYSIZE  18360

// code/psp2/psp2_glimp.c — allocated once at startup
vertex_buffer   = malloc(0x100000);  // 1 MB
color_buffer    = malloc(0x100000);  // 1 MB
texcoord_buffer = malloc(0x100000);  // 1 MB
index_array     = malloc(MAX_INDICES * 2);  // 8 KB

// Zero-copy index submission
vglIndexPointerMapped(indices);
```

`vglIndexPointerMapped()` tells vitaGL that the index buffer is already in GPU-
accessible memory and skips the internal copy. This is a small but free win for
index-heavy geometry.

---

## 2. Audio

### 2.1 vitaQuakeIII — `code/psp2/psp2_snd.c`

| Parameter | Value |
|-----------|-------|
| Sample rate | 48 000 Hz |
| Buffer size | 16 384 bytes (`AUDIOSIZE`) |
| DMA frame | 8 192 samples (`AUDIOSIZE / 2`) |
| Channels | Mono (`SCE_AUDIO_OUT_MODE_MONO`) |
| Sample format | 16-bit signed integer |
| Port | `SCE_AUDIO_OUT_PORT_TYPE_MAIN` |
| Thread priority | `0x10000100` |
| Thread stack | 64 KB (`0x10000`) |

```c
// Audio thread — psp2_snd.c
static int audio_thread(SceSize args, void *argp) {
    while (1) {
        sceAudioOutOutput(chn, audiobuffer);
    }
}
```

The engine submits a pre-mixed buffer; the audio thread only DMA's it out.
Sample position is tracked via `SceRtcTick`:

```c
float tickRate = 1.0f / sceRtcGetTickResolution();
// SNDDMA_GetDMAPos:
samplepos = deltaSecond * SAMPLE_RATE;
```

**Comparison with KeeperFX:** KeeperFX's audio is considerably more capable —
16-channel software mixer, OGG Vorbis streaming, and a dedicated FMV audio
thread with sub-millisecond PTS sync. Q3's audio is simple by comparison;
no new techniques are applicable here.

### 2.2 d3es-vita — `neo/sound/`

| Parameter | Value |
|-----------|-------|
| Library | OpenAL (`AL/al.h`, `AL/alc.h`, `AL/alext.h`) |
| Max channels | 8 (`SOUND_MAX_CHANNELS`) |
| Sample rate | 44 100 Hz (`PRIMARYFREQ`) |
| Thread model | None (NOMT build flag) — OpenAL mixes internally |
| Pause | `alcSuspendContext` / `alcProcessContext` |
| Decompression limit | 6 s max (`s_decompressionLimit`) |
| DSP | `speexdsp` for resampling |

Real-time Vorbis decoding is enabled (`s_realTimeDecoding = 1`). Unlike
KeeperFX's manual mixer, OpenAL handles mixing, spatial positioning, and output
internally. The trade-off is less control over per-voice timing.

---

## 3. Input

### 3.1 vitaQuakeIII — `code/psp2/psp2_input.c`

#### Initialisation

```c
void IN_Init(void *windowData) {
    sceMotionReset();
    sceMotionStartSampling();
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
}
```

#### `SCE_CTRL_MODE_ANALOG_WIDE` + Circular Deadzone

Standard mode (`SCE_CTRL_MODE_ANALOG`) reads sticks as `uint8_t` 0–255 with
centre at ~127. `ANALOG_WIDE` enables the full analog report path across all
controller configurations.

The true improvement is the processing pipeline that follows:

```c
// Scale 8-bit raw value to ±32640 pseudo-16-bit space
int right_x = (keys.rx - 127) * 256;
int right_y = (keys.ry - 127) * 256;

// Apply circular deadzone (not axis-independent)
IN_RescaleAnalog(&right_x, &right_y, 7680);

// Accumulate sub-pixel movement across frames
hires_x += right_x;
hires_y += right_y;
Com_QueueEvent(SE_MOUSE, hires_x / cl_analog_slowdown, hires_y / cl_analog_slowdown, ...);
hires_x %= cl_analog_slowdown;  // carry fractional part
```

```c
void IN_RescaleAnalog(int *x, int *y, int dead) {
    float magnitude = sqrtf((float)(*x * *x + *y * *y));
    if (magnitude >= (float)dead) {
        float scale = 32768.0f / magnitude * (magnitude - dead) / (32768.0f - dead);
        *x = (int)(*x * scale);
        *y = (int)(*y * scale);
    } else {
        *x = *y = 0;
    }
}
```

A naive axis-independent deadzone (`if (abs(x) < dead) x = 0`) creates a
square dead zone. Diagonal stick pushes near the corner of the square feel
choppy because one axis passes the threshold before the other. The circular
(radial) deadzone treats the stick as a 2D vector; the full vector must leave
the dead circle before any movement is reported, and the live range is linearly
scaled from zero to the stick edge. The result is smooth motion in all
directions, including diagonals.

#### Button Mapping

| Vita button | Engine key |
|-------------|-----------|
| START | K_ESCAPE |
| SELECT | K_ENTER |
| D-pad | K_UPARROW / DOWNARROW / LEFTARROW / RIGHTARROW |
| Triangle | K_AUX4 |
| Square | K_AUX3 |
| Circle | K_AUX2 |
| Cross | K_AUX1 |
| L Trigger | K_AUX5 |
| R Trigger | K_AUX6 |
| Left stick (digital) | K_AUX7–K_AUX10 (threshold: <80 / >160) |

#### Gyroscope

```c
// Per-frame, when cl_gyroscope cvar is enabled
SceMotionState motionstate;
sceMotionGetState(&motionstate);

// angularVelocity in rad/s
mouse_x += 10 * motionstate.angularVelocity.y * cl_gyro_h_sensitivity->value;
mouse_y += 10 * motionstate.angularVelocity.x * cl_gyro_v_sensitivity->value;
```

Requires `SceMotion_stub` linked and `sceMotionReset()`/`sceMotionStartSampling()`
called at init. Sensitivity is configurable via in-game cvars. This is
an additive input layer — combined with analog stick movement if both are active.

### 3.2 d3es-vita — `neo/sys/events.cpp`

- Delegates to SDL2 `SDL_GameController` API over SCE_CTRL for portability.
- Touch is **disabled by default** (both panels stopped) — reduces overhead.
- Right analog stick emulates mouse; raw value divided by 256 for normalisation.
- Gyroscope is linked (`-lSceMotion_stub`) but used only for optional features.

---

## 4. Memory Management

### 4.1 CPU Clock Tuning (vitaQuakeIII)

The single largest and simplest optimisation in vitaQuakeIII: lock the Vita's
CPU, bus, and GPU to their maximum frequencies at startup.

```c
// code/psp2/sys_main.c
scePowerSetArmClockFrequency(444);     // CPU: 444 MHz (default ~333 MHz)
scePowerSetBusClockFrequency(222);     // Bus: 222 MHz
scePowerSetGpuClockFrequency(222);     // GPU: 222 MHz
scePowerSetGpuXbarClockFrequency(166); // GPU crossbar: 166 MHz
```

The Vita's default clock is conservative to save battery. These four calls bring
the hardware to maximum rated speed. The only cost is increased power draw
(relevant for battery life, not for docked/charging use). This is the first
thing to add to `PlatformVita.cpp`.

### 4.2 `sceKernelPowerTick` (vitaQuakeIII)

```c
// code/psp2/sys_main.c:504-509
while (1) {
    sceKernelPowerTick(0);  // prevent screen sleep
    Com_Frame();
}
```

The Vita's power manager counts down from the last "user activity event"
(button press, touch event) to decide when to dim and then lock the screen.
A game running in its main loop generates **no such events** unless the player
is actively pressing buttons — so cutscenes, map loading screens, and moments
when the player uses only the analog stick (which is polled, not an event) all
risk triggering the timeout.

`sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT /* = 0 */)` manually resets
the countdown each frame. It tells the kernel "this process is actively running;
do not idle." Cost is negligible — one syscall per frame.

### 4.3 Heap Size

```c
// vitaQuakeIII — sys_main.c
unsigned int _newlib_heap_size_user = 256 * 1024 * 1024;  // 256 MB (Q3/OA)
unsigned int _newlib_heap_size_user = 305 * 1024 * 1024;  // 305 MB (Urban Terror)

// d3es-vita — neo/sys/linux/main.cpp
unsigned int _newlib_heap_size_user = 256 * 1024 * 1024;  // 256 MB
```

Both reserve 256–305 MB of the Vita's ~420 MB user-available RAM. KeeperFX
should evaluate its own heap size — the default newlib heap may be unnecessarily
small.

### 4.4 vitaGL Malloc Wrapping (d3es-vita)

Standard Vita programs run with **two separate memory allocators**:

- The C stdlib heap (`malloc/free`) — managed by newlib, used by game code.
- vitaGL's internal GPU pool — fed by `vglInit*`, used for vertex buffers,
  texture uploads, and GPU command lists.

These pools are siloed. Each fragments independently. A large texture upload
can fail even when total free memory is sufficient, because vitaGL's pool ran
out while the C heap still has headroom.

d3es-vita solves this via GCC **linker wrapping** (`--wrap` flag). The following
five functions in `neo/sys/linux/main.cpp` intercept every stdlib heap call in
the entire binary at link time:

```cpp
// neo/sys/linux/main.cpp:53-59
extern "C" {
void *__wrap_calloc (uint32_t n, uint32_t sz)       { return vglCalloc(n, sz); }
void  __wrap_free   (void *addr)                    { vglFree(addr); }
void *__wrap_malloc (uint32_t sz)                   { return vglMalloc(sz); }
void *__wrap_memalign(uint32_t align, uint32_t sz)  { return vglMemalign(align, sz); }
void *__wrap_realloc(void *ptr, uint32_t sz)        { return vglRealloc(ptr, sz); }
}
```

Paired with linker flags (extracted from `neo/Makefile`):
```makefile
-Wl,--wrap,malloc,--wrap,memalign,--wrap,free,--wrap,calloc,--wrap,realloc
```

With this in place vitaGL manages a **single unified pool**. It can satisfy GPU
buffer requests from memory that game objects just freed, and vice versa. The
`vglInitWithCustomThreshold` 10 MB threshold only needs to cover GPU-exclusive
data; everything else comes from the shared pool.

---

## 5. Threading

### 5.1 vitaQuakeIII — SMP Render Thread

```c
// code/psp2/psp2_glimp.c — thread creation
SceUID renderThreadId = sceKernelCreateThread(
    "Render Thread", render_thread, 0x10000100, 0x200000, 0, 0, NULL);
sceKernelStartThread(renderThreadId, 0, NULL);

// Game thread wakes renderer
void GLimp_WakeRenderer(void) {
    sceKernelSignalSema(renderCommandsEvent, 1);
    sceKernelWaitSema(renderActiveEvent, 1, NULL);
}

// Renderer acknowledges and sleeps
void GLimp_RendererSleep(void) {
    sceKernelWaitSema(renderCommandsEvent, 1, NULL);
    sceKernelSignalSema(renderActiveEvent, 1);
}
```

| Thread | Priority | Stack | Purpose |
|--------|----------|-------|---------|
| Main / game | `0x40` | 2 MB | Game logic + `Com_Frame()` |
| Render | `0x10000100` | 2 MB | OpenGL command submission |
| Audio | `0x10000100` | 64 KB | DMA audio output loop |

The semaphore pair (`renderCommandsEvent` / `renderActiveEvent`) provides a
producer-consumer handshake: the game thread prepares a command buffer and
signals the renderer; the renderer acknowledges (wakes event), processes, then
sleeps waiting for the next frame. This is a textbook SMP renderer split.

### 5.2 d3es-vita — Frontend / Backend Split

```c
// neo/sys/threads.cpp:407-411
SceUID t = sceKernelCreateThread(
    name, vita_thread, 0x40, 2 * 1024 * 1024, 0, 0, NULL);
```

All threads: priority `0x40`, stack 2 MB, wrapped by `vita_thread()` which
marshals arguments through a 32-bit pointer array.

Synchronisation:

```c
// Mutex
SceUID mu = sceKernelCreateMutex(name, 0, 0, NULL);

// Condition variable linked to mutex
SceUID cv = sceKernelCreateCond(name, 0, mu, NULL);
```

The game logic (frontend) and renderer (backend) run in separate threads.
According to the d3es-vita README this produces a **~40% performance gain**
over single-threaded execution by hiding the CPU cost of game logic behind
the GPU's rendering time.

---

## 6. NEON / SSE2NEON (d3es-vita)

### What NEON is

The Vita's ARM Cortex-A9 contains a **SIMD unit** called **NEON**: a 128-bit
wide execution lane that can process **four 32-bit floats in a single clock
cycle** instead of one. The x86 equivalent is SSE (Streaming SIMD Extensions).

For 3D game code — matrix multiplications, vector dot products, lighting
calculations, physics — this is a direct ≤4× throughput multiplier on the CPU
side of the rendering pipeline.

### How sse2neon.h works

The Doom 3 engine was written for x86 with SSE intrinsics
(`_mm_add_ps`, `_mm_mul_ps`, etc.). `neo/sys/sse2neon.h` provides inline
wrappers that map every SSE intrinsic to its NEON equivalent with zero function-
call overhead:

```c
// neo/sys/sse2neon.h
FORCE_INLINE __m128 _mm_mul_ps(__m128 a, __m128 b) {
    return vreinterpretq_m128_f32(
        vmulq_f32(vreinterpretq_f32_m128(a), vreinterpretq_f32_m128(b)));
}
```

| SSE intrinsic | NEON equivalent | Operation |
|---|---|---|
| `_mm_add_ps(a,b)` | `vaddq_f32(a,b)` | Add 4×float |
| `_mm_sub_ps(a,b)` | `vsubq_f32(a,b)` | Subtract 4×float |
| `_mm_mul_ps(a,b)` | `vmulq_f32(a,b)` | Multiply 4×float |
| `_mm_max_ps(a,b)` | `vmaxq_f32(a,b)` | Per-lane max 4×float |
| `_mm_load_ps(p)` | `vld1q_f32(p)` | Load 4×float from memory |
| `_mm_store_ps(p,a)` | `vst1q_f32(p,a)` | Store 4×float to memory |

In practice — a matrix-vector multiply from `neo/idlib/math/Simd_SSE.cpp`:

```c
xmm0 = _mm_mul_ps(xmm0, xmm4);  // 4 multiplies — 1 cycle
xmm1 = _mm_mul_ps(xmm1, xmm5);
xmm0 = _mm_add_ps(xmm0, xmm7);  // 4 adds — 1 cycle
xmm0 = _mm_add_ps(xmm0, xmm1);
```

### Required compiler flags

```makefile
# neo/Makefile
-mfpu=neon -mtune=cortex-a9 -D__SSE__ -D__SSE2__ -D__SSE3__ -ffast-math
```

- `-mfpu=neon`: enables the NEON instruction set.
- `-D__SSE*__`: satisfies `#ifdef __SSE__` guards that gate SIMD code paths.
- `-ffast-math`: allows the compiler's own auto-vectoriser to apply SIMD to
  eligible loops without explicit intrinsics.

### `-lmathneon`

A prebuilt ARM NEON–optimised replacement for `libm` functions including
`sqrtf`, `sinf`, `cosf`, and reciprocal estimates. Linked via `-lmathneon`.
Any call to these in hot paths (rendering, physics, audio DSP) is automatically
faster with no source changes.

### Applicability to KeeperFX

- `-mfpu=neon -mcpu=cortex-a9 -ffast-math` should be added to the Vita build
  flags regardless — even without sse2neon they enable the compiler's
  auto-vectoriser.
- `-lmathneon` is a free win for any `sqrtf`/`sinf`/`cosf` in hot paths.
- `sse2neon.h` itself is most valuable if x86-targeting SIMD code exists or is
  introduced. KeeperFX's sprite renderer and fixed-point colour blending are
  candidates for explicit SIMD later.

---

## 7. Build System Summary

### vitaQuakeIII

- **Tool**: GNU Makefile, `arm-vita-eabi` cross-compiler
- **Flags**: `-D__PSP2__ -mfpu=neon -mcpu=cortex-a9 -O3 -ffast-math -fno-short-enums`
- **Key libs**: `-lvitaGL -lvitashark -lSceGxm_stub -lSceAudio_stub -lSceCtrl_stub -lSceTouch_stub -lSceMotion_stub -lSceRtc_stub`
- **Audio codecs**: `-lvorbisfile -lmpg123`

### d3es-vita

- **Tool**: GNU Makefile (`neo/Makefile`), `arm-vita-eabi`
- **Flags**: `-O3 -flto -ffast-math -mtune=cortex-a9 -mfpu=neon -DVITA -D__SSE__ -D__SSE2__ -D__SSE3__`
- **Wrap flags**: `-Wl,--wrap,malloc,--wrap,memalign,--wrap,free,--wrap,calloc,--wrap,realloc`
- **Key libs**: `-lvitaGL -lvitashark -lSceGxm_stub -lopenal -lSceAudio_stub -lSceCtrl_stub -lSceTouch_stub -lSceMotion_stub -lmathneon -lspeexdsp`
- **Net/TLS**: `-lcurl -lssl -lcrypto -lSceNet_stub`

---

## 8. Gap Analysis vs KeeperFX

Features present in vitaQuakeIII or d3es-vita that KeeperFX does not yet have:

| Feature | Source | Effort | Impact | Notes |
|---------|--------|--------|--------|-------|
| CPU clock tuning (`scePowerSet*`) | Q3 | Low | **High** | 4 lines in `PlatformVita.cpp`; unlocks max 444/222/222/166 MHz |
| `sceKernelPowerTick(0)` per frame | Q3 | Low | Medium | 1 line in game loop; prevents screen sleep |
| `SCE_CTRL_MODE_ANALOG_WIDE` + circular deadzone | Q3 | Low | Medium | Smooth camera pan; eliminates choppy diagonals |
| Gyroscope camera support | Q3 | Medium | Medium | Optional; useful for top-down pan |
| vitaGL malloc wrapping | D3 | Low | Medium | Unified heap; 5 `__wrap_*` functions + linker flags |
| Compiler flag uplift (`-mfpu=neon -ffast-math`) + `-lmathneon` | D3 | Low | Medium | Free win for math-heavy paths |
| `sse2neon.h` for explicit SIMD paths | D3 | Medium | Medium | Most useful when/if explicit vectorised code is added |
| Frontend/backend render split | D3 | High | **High** | ~40% throughput; significant architecture change |
| IME on-screen keyboard | Q3 | Medium | Low | Useful for save names; needs UTF-8↔UTF-16 conversion |
| ETC1 / 16-bit texture formats | D3 | High | Low | KeeperFX palette approach already superior for its use case |
