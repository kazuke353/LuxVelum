# Luxvelum

Atmospheric Immersion for Project Reforged — WoW 3.3.5.

Luxvelum is a fork of [CoAVolFog](https://github.com/jealous-sound/coa-vfog): volumetric fog and modern water for the
Ascension/CoA 3.3.5a (build 12340) Direct3D 9 client. It installs as `Luxvelum.dll` next to `Wow.exe`, its settings
`WTF\Luxvelum.ini` and its data `Data\Luxvelum\fogdata.bin`, `Data\Luxvelum\waterdata.bin` and
`Data\Luxvelum\zones.ini`, and writes
`Logs\Luxvelum.log` (`CoAVolFog.*` before TEST Build-24-11, `Luxveil.*` in TEST Build-24-11 and 24-12, all next to
`Wow.exe` in earlier packages); without a `WTF\Luxvelum.ini` it takes over an existing `Luxvelum.ini` next to `Wow.exe`,
else `Luxveil.ini` or `CoAVolFog.ini`.

It consists of a loader, a D3D9 device wrapper with a readable depth buffer, world-render hooks, a per-pixel
ray-marching fog renderer and a water renderer. Fog layers come from the Classic client's own
`LightDataGlobalVolumeFog` data wherever its lights cover the map; Outland, Northrend and the other maps Classic never
had get lights converted from the 3.3.5 client's own light tables, and the remaining maps use layers derived from the
3.3.5 day/night lighting. Water is shaded like WoW Forever's PBR water, from its liquid presets, wave spectra and foam
textures.

## About this fork

**Please note:** I am not the author or maintainer of CoAVolFog. The project belongs to
[jealous-sound](https://github.com/jealous-sound/coa-vfog); this fork, Luxvelum, only tries to adapt it to Project
Reforged. I have no background in volumetric lighting or fog effects, so while I do my best, I cannot guarantee that a
change here does not make things worse instead of better. If in doubt, leave Luxvelum disabled (`Enable=0` in
`WTF\Luxvelum.ini`).

This fork ([Stormhand-dev/Luxvelum](https://github.com/Stormhand-dev/Luxvelum), development on `reforged-dev`,
releases from `reforged-main`) builds on [jealous-sound/coa-vfog](https://github.com/jealous-sound/coa-vfog) and fixes
fog lights that flickered or went out:

- **LightStay** (`LightStay=1`, on by default) keeps the client from switching off a light whose model was not drawn,
  the same byte as patch No. 70 "lightstay" of St0ny's AIO WoW EXE Patcher.
- **Steady light slots**: the eight fog light slots are held with hysteresis instead of being re-ranked every frame,
  and a jittering table read no longer drops the whole capture.
- **Lights in view**: a light stays on at any distance while its sphere meets the view frustum, instead of a fixed
  distance limit.
- **Smooth swaps**: lights fade in and out over about a second when they take or give up a slot; a light entering
  from the edge of the view shines at once.
- **Storm fog over water**: no more bright lines on grazing water in noisy fog near point lights.
- **Grain in the fog**: the temporal filter no longer takes the march jitter for a light change, so the dot pattern
  at low quality is filtered away.
- **Fog in the rain**: where the Forever kit has no storm fog for a light, the clear fog stays when it rains.
- **Releases**: every push to `reforged-main` publishes the version in `VERSION` as `v<version>` and keeps only the
  newest release:
  [Luxvelum-Win32.zip](https://github.com/Stormhand-dev/Luxvelum/releases/latest/download/Luxvelum-Win32.zip).
  A push to `reforged-dev` that `reforged-main` does not contain yet publishes a pre-release
  `v<version>-test.<k>` for testing the next version, `<k>` counting the test builds since the last release.

## What it draws

- **Distance haze** that thickens toward the horizon and fades with altitude, tinted by the zone's
  stock fog colour.
- **Ground mist** that hugs the terrain around the player; zones with short stock fog get more.
- **Distance fog** that replaces the stock linear fog on maps without Classic data: it turns opaque where
  the stock fog did, is lit by the direct light (warm at sunset), and fades into a horizon band on the sky.
  With Classic layers it returns only across the edge of Classic coverage and where the Classic layers are
  too thin to hide the far clip. Sky seen below the horizon, such as the void under the edges of Netherstorm, takes
  the fog of the sky mirrored above it; levelled to the horizon, its rays crossed the whole fog range and showed a
  white wall.
- **Forward scattering** around the visible sun or moon sprite (Henyey–Greenstein phase).
- **Local lights**: up to eight native point lights in view scatter into the medium with the client's constant,
  linear and quadratic attenuation. Light/ray intersections preserve small light volumes between march samples.
  Every light scatters with one shared phase (`LocalLightPhase`), and one brighter than 1 keeps its intensity.
- **Interior transitions**: the camera's native WMO blend reduces outdoor layers and sunlight while preserving
  the interior's native fog colour and range.
- **Authored noise** (on in the shipped INI, off when the key is missing): drifting fog banks where the Classic
  layers carry the modern client's noise, mostly in storms (`ClassicNoise`).
- **Density variation**: a two-octave field anchored in world space modulates the scene layers around their
  authored mean and drifts along world +X; the distance fog stays smooth so it cannot uncover the far clip. This
  is an artistic control, not a reconstruction of Classic's authored noise.
  Layers with authored noise use that instead.
- **God rays** (optional): shafts of light through gaps in trees, buildings and terrain in front of the sun, from a
  radial blur of the bright sky; an open sky, its clouds included, gets none.
- **Modern water** on lakes, rivers, the sea and indoor pools: FFT waves from Forever's wave tiles, refraction,
  depth absorption and in-scattering, foam on wave crests, along shores and in shallow water, a sun or moon glint,
  and reflections of the scene with the sky as a fallback. The zone's own water colours can tint it
  (`WaterZoneColors`). Magma, slime, custom non-water liquids and water seen from below keep the client's look.
  Players, creatures, pets and mounts that wade or swim drag a V-shaped wake and stir the water as they start and
  stop (`WaterRipples`), in place of the client's flat splash and wake sprites (`WaterClientSplashes`).
- **Multisampling**: the game's Multisampling option is kept where the DLL can copy the multisampled depth
  (`Multisampling`); fog and water edges are resolved per sample.
- **See-through effects in the fog** (`TransparentFog=1`, on by default): the fog is drawn before particles, spell
  effects and other see-through models, which take a linear fog fitted to it by their own distance, as in the stock
  client, instead of the fog of the scene behind them; lamp glows fade with distance (see Transparent fog).
- **Opt-in** (all off when the key is missing; the shipped INI grades at `ColorGrading=0.5`): Forever's per-light glow
  amount (`ForeverGlow`) and colour curve (`ColorGrading`) where Classic lights cover the camera, and an
  energy-normalised phase for the Classic layers' sun and moon scattering (`ClassicPhase`).

## How it renders

The march runs at quarter or half resolution (`Quality`). Each step integrates only its overlap with a layer's
start and end distances, so thin layers and partial boundary steps stay consistent across quality levels. The
march and composite use shader variants with light code only while a point light is uploaded.
Variants that sample authored noise run only while a Classic layer carries it.

With `Temporal` above 0, samples vary between frames and a temporal filter accumulates them. It rejects history
from a different surface depth or depth class (world, distant terrain, sky) and lowers its weight where animated
lighting changes the radiance, judged on the mean of the current 3×3 low-resolution neighbourhood so the per-texel
step jitter does not read as a lighting change and stays filtered; settings, map, screen-effect slot, projection
and large camera changes discard it. With `Temporal=0`, samples stay at fixed midpoints so a stationary frame does
not shimmer, and the temporal passes are skipped.
Where no history applies (a newly exposed surface, the screen edge while turning, the frame after a discard), the
filter returns the average of the current samples in the 3×3 low-resolution neighbourhood on the pixel's surface.

The full-resolution composite upsamples the march through depth-validated taps. Ground seen at a grazing angle
is interpolated from the current frame's march where the taps lie on the pixel's plane, the march is linear
across them and no light's sphere meets the ray; thin silhouettes that every tap misses are marched at full
resolution. Under authored noise the lit noisy composite interpolates the filtered fog wherever the taps lie on the
pixel's plane, lights and curvature aside, and marches only the rest through the noise (see Authored noise).

Before the lit composites march a pixel again with every lamp, they look at the twelve low-resolution taps around
the bilinear footprint: the fog of those within the 2 % surface test, weighted by depth difference and distance, or
else the fog of the tap of the same depth class nearest in depth if it lies within 25 % of the pixel's depth. Only a
pixel that neither finds marches. In foliage, distant trees and grazing ground almost every pixel misses its four
bilinear taps, and the misses are scattered single pixels; each makes its whole group of neighbouring pixels on the
GPU wait through a march with every lamp, so the cost followed the foliage's area. At Darkshire at night with eight
lamps (Quality 2, 2560x1080, MSAA 4x) the fog took 8.53 ms with the lit composite against 2.33 ms with the unlit one
(TEST Build-2-15), 6.79 ms with the surrounding taps (Build-2-16) and 3.21 ms with the nearest-depth tap as well
(Build-2-18, 48 to 69 FPS against a 72 FPS cap), and the tester saw no change in the image. The lit march itself with no
lamp uploaded cost 0.08 ms more than the unlit one, and the lit composite with no lamp 1.1 ms more than the unlit one.
`DebugView=4` paints the pixels that still march magenta over the transmittance.

Every full-screen pass draws one triangle whose edges lie outside the viewport (NDC −1.5 and 4.5), so it covers every
4x sample of the edge columns; Direct3D 9 maps NDC −1 onto the first column's centres.

**Local lights.** The captured diffuse includes the M2 light's animated intensity (see World lights), so a light
brighter than 1 carries it in the colour. In linear light a colour whose brightest channel is above 1 decodes only its
chromaticity, `(c/peak)^2.2 · peak`, as the modern client scales light colours linearly (a choice; the kit lacks its CPU
packing). Every point light scatters with one Henyey–Greenstein phase normalised to 1 toward the light, g =
`LocalLightPhase`, as the modern fog-light kernel 6227851 reads one g for all local lights. A layer start below 50 yd is
the client's clearance around the camera, not distance fog, so the lamps scatter in that layer from the camera on,
without its extinction: at Falcon Watch in Hellfire Peninsula the ground fog starts at 15 yd and the glow of a lamp the
player stood next to vanished (TEST Build-2-4); starts of 50 yd and more, such as the 100 to 3000 yd of distance layers,
keep the lamps out. The lights scatter with a gain of the region's intensity (`AzerothLocalLights`,
`OutlandLocalLights`, `NorthrendLocalLights`, `TBCStartLocalLights`) over its density (`AzerothDensity`,
`OutlandDensity`, `NorthrendDensity`, `TBCStartDensity`, at least 0.05), here and in the fitted stock fog of see-through
models, so a region thinned to 0.15 keeps the glow of its lamps. Outland and Northrend start at 8 (the shipped INI sets
Outland to 4; 0..128, the overlay's logarithmic sliders reach 96) and the Burning Crusade starting zones at 2.5: their
converted layers are far thinner than the old world's (Borean Tundra at night 0.00013/yd of ground fog against
Tirisfal's 0.0045 in a tester's log), and the tester needed the slider's former maximum of 8 there to match the old
world's lamps. Each light's cutoff radius, where its glow fades out, is that of its colour times the region's intensity,
as for a lamp that much brighter; Azeroth at 1 keeps its radius. A tester found 24 barely stronger than 8 in TEST
Build-24-10: in Wine's Direct3D on llvmpipe at Amberpine Lodge the march's glow at a lamp grew 3× from 8 to 24 but the
displayed glow above the scene only 1.85× (one orange lamp at 20 yd, core 55 → 89 in 8-bit codes, 167 at 96), and the
halo of a dim lamp at 50 yd stopped at the cutoff of its unscaled colour.

Twin lights are merged before the stabiliser chooses from the candidate pool (below). The capture ranks up to 48
candidates, twice the pool, so that both lights of a pair at the pool's edge are seen; after the merge the 24 strongest
stay. Torches at Light's Hope Chapel come in pairs: in a tester's logs (2026-11) the two lights of one torch lay 0.01 to
1.82 yd apart with the same colour and attenuation (0, 0.7, 0.03), separate torches at least 6.29 yd. Candidates within
2.5 yd of each other whose attenuation coefficients agree within 10 % and whose colours, each divided by its brightest
channel, agree within 0.15 form a cluster; a cluster whose lights span more than 5 yd stays apart. A cluster becomes one
light at the brightness-weighted mean of its positions, with the summed uploaded colour, the weighted mean attenuation,
the colour and node address of its lowest address, and the larger of every member's cutoff and that of the sum; the pool
is then ranked again. Two equal torch lights 2 yd apart merged at their midpoint light a point 3 yd from it within about
12 % of the pair (6 yd: 3 %), and the pair takes one slot instead of two, so the eight slots reach twice as many paired
torches. The log's pool summary counts the merged lights, and each uploaded light that stands for several says so. A
cluster that loses a member at the edge of the view or of the 48 captured candidates can change its lowest address, or,
with three or more lights of unequal brightness, move its weighted position beyond the stabiliser's identity radius, and
so fades over to a new light as any light that changes address does.

The eight uploaded lights are chosen by a stabiliser rather than re-ranked from scratch every frame. Every capture ranks
the lights whose sphere meets the view frustum, at any distance, by their irradiance at the camera into a pool of up to
24 candidates (captured from up to 48, see twin lights above), equal irradiance ranking the nearer light first; the view
test precedes the ranking, so a light whose sphere lies wholly outside the view takes no slot. A light behind or beside
the camera still competes while its sphere reaches into the view, which a sphere of up to 200 yd does from as far away.
A candidate is the nearest light already known with the same node address within 2 yd plus 40 yd for each second since
the last capture that was not held (see below), so a colour or attenuation that animates between captures stays one
light, and so does a light that moves at up to 40 yd/s at any frame rate, also across held captures that span up to 1 s,
while an address the client reuses 10 yd away within one frame at 60 fps is a new light. A free slot goes first to a
challenger that evicted its holder, otherwise to the visible candidate with the highest smoothed priority (0.3 s). A
light that takes a slot fades in over 1 s along a smoothstep curve, so neither end of the fade jumps.
It shines at once in the first capture after a reset (map change, start, a gap of more than 1 s) and when
it enters the candidates with its centre outside the view, its sphere reaching in from the edge as when the camera
turns back toward it. An occupied slot changes hands only when a candidate's smoothed priority exceeds 1.5 times that
of the weakest slot for three captures in a row, and a light that leaves the client's table, is evicted, dims to
nothing or is crowded out of the candidate pool fades out over 1 s along the same curve instead of vanishing. The
evicted light fades out before its challenger takes the slot and fades in; swaps run side by side while every slot
that is fading out has a challenger waiting for it, so a light that left holds back further swaps until its slot is
free. An eviction that no challenger waits for any more, because the challenger left or took a slot freed elsewhere
first, passes to the strongest other visible candidate whose smoothed priority exceeds 1.5 times the evicted light's,
such as the challenger back after missing a capture; with no such candidate for three captures in a row it is called
off, and the evicted light ramps back up instead of fading out and in again. A light that has faded keeps its smoothed
priority while it is still captured inside the view and fades back in if it wins a slot again; the slot it lost goes
to its challenger, so the challenger is not undercut by its momentary rank. A slotted light that is not evicted keeps
its slot through two missed captures in a row even at nothing, so a light that has just taken a slot and drops out of
one capture does not hand it back to the light it replaced.
A slotted light whose last captured sphere lies more than 5 yd outside the view is forgotten at once, freeing its slot
the same step, and re-enters as a new light; one that only jitters across the view's edge, or moves out of the view
while its last captured position still meets it, dims over 1 s and ramps back if it returns within that time, so a
sphere on the edge does not hand its slot back and forth. A capture that walked nothing (damaged table, table changed,
read fault) keeps the previous set for up to eight captures. The stabiliser steps once per captured world frame; when
the frame inputs are read a second time in the same frame (the early composite and the whole-frame pass can both read
them) the stabilised set is replayed, so hysteresis and holds count frames. There is no camera-distance bound: a light
stays while its sphere meets the view and the client keeps it in the table, which the client's own doodad loading
bounds; the march evaluates a far light's scattering only on the steps where the ray crosses its sphere. `LightStay`
keeps the client from disabling a light whose model is not drawn (see World lights), which otherwise also ends its fog
glow the moment the source leaves the frame.

Fog renders once per frame, before screen effects and the UI: after the world, including its late geometry and the
native sun/moon glare, or, with `TransparentFog=1` (the default) above water, at the end of the liquid pass, after the
glare and before the see-through models (see Transparent fog). While it draws, the stock fog is pushed out of range for
the world render and restored afterwards; frames without volumetric fog keep the stock fog, and an unexpected draw
failure restores it on the next frame. The client's glow (`screen + g·blur²`, with `g` from the day/night light) runs
afterwards and would bleach bright fog to white, so fogged pixels are pre-compensated with the live glow amount while
the fog blends in linear light over a scene copy (`ColorSpace=1`). The gamma blends (`ColorSpace=0`) and the
fixed-function blend without a scene copy leave the fog uncompensated; only the god rays are compensated there. God rays
use the remaining display highlight range after the glow and need a scene copy and the half-resolution copy of the world
viewport their mask is drawn from; without either they are omitted, and a failed copy is logged once.

**God-ray mask.** The rays are blurred at a quarter of the viewport's resolution, so each mask texel stands for a 4x4
block of screen pixels. The mask used to test that block for sky with one depth tap and take its colour from a 4:1
linear copy of the scene, which reads the block's central 2x2 pixels: a black branch one pixel wide in front of bright
sky took none, half, all or none of the texel as it crossed the block's four columns, and the radial blur drew that
change along the whole ray, so the rays trembled while the camera moved and stood still when it stopped, as a tester
reported. The mask now takes its colour from a copy of the world viewport at half resolution, four texels per mask
texel, each the linear average of a 2x2 block, passes each through the brightness threshold and weights it by the sky
share of its four pixels from four depth taps. The same branch now takes 3/8 of the texel wherever it crosses the block,
the average of what the old mask took, so the rays keep their strength and lose the flicker; since the colour average
already contains the branch, a dark occluder weighs more than its area, as before. The mask grows from 47 to 213
instruction slots and 2 to 20 texture reads at a sixteenth of the screen's pixels, and the copy adds one 8-bit target of
a quarter of the viewport's area.

**God-ray shafts.** A tester found that the God rays slider brightened the whole sky around the sun like Sun
scatter (2026-10-27): the radial blur of an open sky is a round glow, and the rays only showed as the edges of the
occluders cut into it. The mask now also writes each texel's sky share into its alpha, which the radial blur carries
along with the colour, and a third pass after the blur keeps only the shafts. For each ray texel it reads the blurred
sky share along an arc around the sun, twelve taps 2.5 degrees apart on each side, takes the smallest on each side
and keeps the part of the texel's rays that its sky share holds above the larger of the two. An open sky, clear or
brighter toward the horizon, and the one-sided edge of a horizon or a mountain keep nothing; a gap between trunks or
leaves, with shadow on both sides, keeps its shaft. The arc compares the sky share and not the rays' colour, because
a sky that brightens toward the horizon, as at dusk, peaks along the arc above the skyline and brought back up to
20/255 of glow in the harness. Clouds and the skybox's models write no depth, so they count as open sky: gaps between
clouds give no shafts. So God rays never darken and leave the sun's glow to Sun scatter and the client; within
30 degrees of a gap that spans the whole arc, next to the sun, they add nothing either. The pass reads 25 bilinear taps
at a sixteenth of the screen's pixels and writes into the blur's first target, which both composites read. The
harness renders the sun over a clear sky, over a dusk sky brightening toward the skyline below it, behind thirteen
trunks and behind a canopy of leaves at God rays 0, 1 and 4 and saves the images: at 4 the clear sky changed by
41/255 on average before (up to 94/255) and the dusk sky by 47/255 (up to 109/255), both by 0/255 now, while the
shafts between the trunks reach 0.35 in luma and nothing darkens (Wine's Direct3D on llvmpipe).

**The own character.** A tester found rays on and around the own character that moved with it, and none from the sun
over the open plains of the Barrens and Durotar (2026-10-28). The character stands in front of the sun more often than
any tree: the mask counted it as an occluder like any other, and the shaft pass kept the open sky on both sides of it as
a gap, so rays fanned out around the character wherever it walked, and over an open plain they were all there was. The
DLL now reads the position of the unit the camera follows at `0xCD7778` (the camera position when the code that stores
it there differs, see the evidence below) and treats an upright cylinder around it as the character: 3 yards in radius,
from half a yard below the focus to 5 yards above. Each test rebuilds the world position of a depth sample from its
pixel and depth and checks it against the cylinder, after a screen rectangle around the projected box that holds the
cylinder; when a corner of that box comes within a quarter yard of the camera's plane, the rectangle is the whole
viewport. The mask writes a second target, the sky share with the character's samples counted as open, which the radial
blur carries along with the first; the shaft pass takes the minima along its arcs from it and the texel's own share from
the first, so the character no longer makes a gap, and trunks behind and beside it keep their shafts. Both composites
draw no rays on the character's pixels, so those shafts do not shine through it. A camera zoomed in closer than 3 yards
beside the focus, within the cylinder's height, narrows the cylinder to stay a quarter yard clear of it; below three
quarters of a yard, as in first person, beyond 100 yards and for an unreadable focus nothing is excluded. The harness
renders a character 8 yards ahead, just below the sun, 2.5 yards beside it over the ground, in front of the thirteen
trunks and between trunks 4 yards to either side at its own distance, and 2 yards ahead, at God rays 4: without the
focus, which is the old behaviour, the sky around it changed by up to 42/255 and 93/255 and the character brightened by
up to 113/255, and at 2 yards by up to 96/255 when the cylinder switched off instead of narrowing; with it all four stay
at 0/255, the far trunks' shafts still reach 0.35 in luma, and the near trunks' shafts reach 0.059 with or without the
character. A first version tested a screen rectangle and a band of depth instead of the cylinder; that region reaches
far beyond 3 yards at the sides and covers every surface near the camera when zoomed in, and the near trunks' shafts
fell to 0.012. The mask grows from 216 to 729 instruction slots at a sixteenth of the screen's pixels, the blur from 42
to 66, the late composite from 51 to 94 and the composite from 1005 to 1040 (medium quality), and the second share takes
two more 8-bit targets of a sixteenth of the viewport's area; the state block now also restores `c100`–`c102`. On a
device without two simultaneous render targets the character casts shafts again, which the log says; only the rays on
its own pixels stay off. Mounts and vehicles larger than the cylinder, other players and creatures still cast shafts,
ground and leaves inside the cylinder count as open sky, and the composites test one depth per pixel, so under
multisampling the rays' edge along the character is not antialiased. An open plain with nothing in front of the sun has
no shafts by design; the glow around the sun there is Sun scatter's and the client's.

**Hidden sun and moon.** A tester saw the moon's glow and the sun's glint on water shine through mountains
(2026-11). The fog's forward scattering and the water's glint followed the light's direction alone; only the god rays
saw occluders. The DLL now traces the client's own sight line every 0.05 s, with the world intersection the glare's
visibility test uses (see Sun and moon sight line): from half a yard ahead of the camera toward the sun or moon over
the far clip and, when that is blocked, back from the far end to find the farthest occluder. The hidden share fades
over 0.2 s, takes a new light (sun or moon) or map at once, and stays 0 when the trace is unavailable; it is scaled by
the light's height above the horizon. Fog samples nearer than the farthest occluder lose the part of each layer's
phase above its value 60 degrees from the light, so the halo flattens to its edge instead of going dark, isotropic
and back-scattering layers keep their light, and fog beyond the occluder, as on a sky ray past the ridge, keeps its
halo. The water drops its glint where the surface is nearer than the occluder, and the fitted stock fog of see-through
models follows the fog's rule. The share and the distance go to `c79`, the four layers' edge phases, computed on the
CPU, to `c80`, the water's share and distance to `c68`. The harness marches a layer with g = 0.8 toward a light
ahead: hidden, the halo on a surface falls from 0.075 to 0.0016, the edge share, and a sky ray past a ridge 40 yd away
keeps 1.23 of its 1.30 (low quality), with the old shaders unchanged at 0.075 and 1.30. The unlit march grows from
537 to 557 instruction slots and the composite from 1040 to 1060; the unlit noisy composites, which would exceed the
32 temporary registers, march their full-resolution fallback through the layer loop instead of unrolling it (867
slots instead of 1122). One ray from the camera decides for the whole screen. The trace uses the glare's hit flags
`0x200112`, and which models they include has not been checked in the game, so trees may not hide the light; only
loaded terrain counts, and the halo of the fog reflected in water stays.

**Display dither.** The fog is marched and filtered in 16-bit float and blended in 32-bit float, but the composite
writes 8-bit codes into the client's back buffer. Where a gradient falls slowly, as in the dark outer halo of the sun
and the moon, one code spans many pixels (about 11 px at code 40 and 18 px at code 33 in a tester's moon screenshot), so
an undithered write draws rings, and red, green and blue cross their codes at different radii. The composite's final
colour (over the scene copy and for the fixed-function blend, after the glow compensation and the god rays) and the late
god-ray composite therefore add an interleaved-gradient-noise offset keyed to the pixel before the write rounds: 0.99 of
a code peak to peak, one value for R, G and B. The offset is static, so a still frame stays identical and the temporal
history never sees it, and every sample of a multisampled pixel and both sides of a split silhouette take the same
offset. It stays 0.005 code inside the rounding edges, so a pixel without fog keeps its exact code through the gamma
round trip. The debug views and the sun marker are not dithered while `ColorGrading=0`; with grading on, the grade runs
over them too and dithers its own output. Without a scene copy the dither is added to the fog's own colour, the source
of the fixed-function blend, and the blend unit adds and rounds the scene's share `dest·(1−a)`: a blender that rounds
the source to 8 bits first leaves that share undithered (Wine's Direct3D on llvmpipe does: thin black fog over code 200
keeps 0.36 codes rms of column-mean error with the dither and without it, against 0.013 over the scene copy), and since
the target clamps the source at 0, a float blender dithers it only in part where the fog adds less than about half a
code. On the tester's halo profile (simulated, high-passed column means) the coherent banding falls from 0.29 to 0.008
codes rms without the client's glow, with no coherent edge left of 21 in the outer halo below code 50, and from 0.37 to
0.22 codes rms at a glow of 0.5, with 2 such edges left of 18. The glow pass that runs afterwards
(`lerp(screen, blur, z) + g·blur²`) rounds to 8 bits again without a dither, so where the day/night glow is above 0
weaker steps from that rounding remain, mostly in the brighter inner halo, and a colour transform after `Present` (a
driver filter or the capture) can stretch the dithered steps again. The dither adds 14 instruction slots to each
composite (both blend paths) and 6 to the late god-ray composite and to the grading, and no temporary register.

**Glow and colour grading.** With `ForeverGlow=1` the DLL rewrites the glow byte the client hands the glow composite
(see Glow feed) once per frame, before the fog composite, to `lerp(client byte/255, Forever glow, t)` with `t =
saturate((coverage − 0.5)/0.5)` from the Classic lights' share of the blend, and restores it at the frame end; it never
writes the day/night glow, and writes nothing under the ghost effect or an unexpected effect graph. `GlowCompensation`
uses the byte the composite receives, so a glow above 1, which wraps, is compensated as drawn.
`ColorGrading` passes a copy of the world viewport through the blended 32-entry curve per channel at the end of the
world render, after the world-name text (`0x4F9218`) and the glow, before the later name/icon pass (`0x4FB042`) and the
interface, writing `lerp(scene, curve(scene), ColorGrading·t)` with the display dither; the ghost view, the view under
water and a view without a grading key are left alone. The grading rereads the 8-bit frame, and its curve (0.97 to 1.22
codes per code between codes 25 and 60 in all eight shipped curves) would round the dithered fog into steps again: on
the halo profile without the client's glow, grading at strength 1 without a dither of its own left 0.155 codes rms and
about 7 coherent edges per curve, with it 0.046 and none.

**Transparent fog.** Fog drawn once after the world gives every see-through effect the fog of the surface behind
it, where the stock client fogs it by its own distance. With `TransparentFog=1`, above water, the glare and the fog
are drawn at the end of the liquid pass instead, before the see-through models on the camera's side, and every later
M2 batch fog call gets a linear fog with exponent 1: a least-squares line in planar view depth, the depth the client's
M2 shaders fog by, through the volumetric transmittance along 15 rays within 100 yd, with the fitted in-scatter,
point lights included, as the colour where the batch uses its lighting colour (additive, modulate and modulate-2x
batches keep black, white and grey). When those 100 yd keep more than 90% of the light, the fitted range grows, smoothly
with how clear they are, toward the depth where the transmittance along the view axis halves, reached at 99.5%, and
never past the far clip; when even the far clip keeps 99.5%, no line is fitted and the effects stay unfogged. God rays
are added at the end of the world render. Under water, with `StockFog=0`, a debug view or the sun marker, or when the
early composite cannot run, the fog is drawn after the world.
Since TEST Build-2-26 each see-through model takes its own fog instead of the screen's line where its position can be
read (see M2 model placement): the volumetric transmittance and in-scatter along the ray from the camera to the
model's origin, point lights included, through a line from fog 1 at the camera to that transmittance at the model's
planar depth, with that in-scatter as its lighting colour. Where the composite blends in linear light, the client's
fog, a lerp in display gamma, takes instead the opacity and colour that give the composite's result over black and
over white (exact there, within 0.1 over mid grey): the volumetric opacity with the plain fog colour left dark models
far darker than the fog around them, which is why the banners still looked unfogged with their own fog (Darkshire,
TEST Build-2-29: a banner 43.5 yd away with transmittance 0.56 and in-scatter per opacity 0.17 0.14 0.19, as the depth
probe read 0.13 to 0.18 0.13 to 0.14 0.18 to 0.20 around 41 to 49 yd). Banners in front of a lamp-lit doorway in
Darkshire at night
had taken the screen's mean colour (`383D46`) while the fog around them was lit by the lamps, and looked unfogged
(2026-11-22); the screen's line had matched the volumetric opacity at 15 to 100 yd within 0.1. A model behind the
camera, an attached model, or one past the 256 models a frame keeps the screen's line, and at `LogLevel=2` the
transparent fog line counts the batches fogged at their model.
Batches the client draws unfogged (an M2 material with the unfogged flag, a lighting without fog, blend mode 7), among
them lamp glows, wisps and other glowing effects, take the same line with the colour of their blend mode: drawn after
the composite they had no fog at all and shone through any fog at any distance, where the fog drawn after the world
dims them by the fog behind them (a tester's screenshots of lamps in Duskwood in rain and a lantern in fog, 2026-10-25).
It is on by default (`TransparentFog=1`) since a tester played TEST Build-2-2 with it in Duskwood, Darkshore and
Orgrimmar (2026-10-27) and reported only improvements: rain, the Darkshore wisps and the Orgrimmar fire pits stayed
visible in the fog, and lamp glows faded with distance. `TransparentFog=0` remains the fallback if distant effects look
wrongly fogged.

**Water.** The client draws every water and ocean surface, and nothing else, inside one call of its liquid
renderer. Before that call the DLL copies the scene colour and its linear depth, clears the stencil and turns on
stencil writes of 0 and depth writes for the whole call. Each water draw is classified from its liquid type (lake,
river, ocean, indoor) and tags its pixels with that class in the stencil. After the call the DLL simulates the waves
of the classes drawn this frame, copies the water surface's depth, and draws one stencil-tested full-screen pass per
class that replaces the client's water colour. It runs before weather, alpha models and the fog, so the fog is
integrated against the water surface. The waves are a GPU FFT (Tessendorf/Phillips spectra with Forever's tile
parameters, 256² or 128² per tile, normalised to 256² so the wave height does not depend on the resolution) that
yields slopes, slope variance and a persistent foam state per tile. The shading ports Forever's deferred water pass
(Beer–Lambert absorption, Henyey–Greenstein in-scattering, the EnvBRDF reflection weight, foam compositing); its
unknown sun lookup table is replaced by a GGX lobe and its reflection probe by a screen-space march against the
pre-water copies. Frames without queued water skip the pass entirely.

**Reflection rays.** The reflection ray leaves the surface along a normal whose wave slope is scaled by
`WaterReflectionWaviness` (shipped 0.35), while the Fresnel weight, the roughness and the sun's glint keep the full
normal. With the full slope (1, the behaviour before 2026-11-25) neighbouring pixels of rough water sent their rays to
unrelated places, so the scene's colour and the sky fallback alternated pixel by pixel: the owner's Booty Bay
screenshots of 2026-11-25 showed the harbour lamps' orange broken into specks with cyan sky between them, stronger at
higher `WaterReflections`. A ray that finds no surface within the thickness allowance no longer falls back to the
synthetic sky gradient (the client's sky colours, cyan in Booty Bay) when it went behind the scene: it takes the
scene's colour where it first did. The owner's screenshots of 2026-11-25 showed cyan bands under the harbour's piers,
where the reflected rays pass behind the planks whose undersides the screen does not hold. Only geometry deeper than
nine tenths of the water's own depth counts, as a hit or as an occluder: a reflected ray moves away from the camera,
so it cannot meet the player's character standing in front of the water, whose staff and shoulders otherwise showed
in the water behind him. A ray that stays in front of the scene takes the pixel at its vanishing point or where it
leaves the screen. Every screen colour fades into the gradient over the outer 15% of the screen on each side, as the
vertical fade before it did: two test builds of 2026-11-25 without that fade, one taking the edge pixel's colour and
one taking it by how close the ray came to the edge's depth, made the reflections near the top switch abruptly
between the scene and the sky while the camera tilted, and stretched the piers along the top edge down the water.
The march stops at the left and right edges too: before, a ray past them read the clamped edge column, so the ship
at the screen's edge ghosted in the water while the camera turned. The gradient also fades in as a ray turns
parallel to the screen, and takes over for rays toward the camera.

**Ripples.** Units in the water drive Forever's local displacement recurrence, `next = 0.97·edge·(0.5·(L + D + R +
U) − C.g)`, output `(next, C.r)` (R the height, G the previous one, edge `saturate(16·min(u, 1 − u, v, 1 − v))`), on
a G16R16F ping-pong map that follows the camera target by whole texels. Every step holds the footprint of each unit
whose world entity reports the liquid surface crossing its body (see Water contacts) along its path, `R = lerp(R,
level, k)` then `G = lerp(G, R, k)` with `k` fading over the footprint, the level raised with immersion and speed. So a
standing unit leaves the water still, a unit faster than the ripples drags a V, starting and stopping each release one
ring, and wading in splashes once. The shading adds Forever's one-map slope (prepass 7552035) of `lerp(G, R, w)`, `w`
the elapsed fraction of the next step. With `WaterClientSplashes=0`, while ripples can run on shaded water, the DLL
holds the client's `waterRipples` value at 0, so its `splash.blp` and `wake.blp` sprites stop, and puts the value
back when the hold ends.

**Antialiasing.** D3D9 textures cannot be multisampled, so with the game's Multisampling option the depth the
passes read is a copy. `CheckDeviceMultiSampleType` offers the game's sample counts only while `Multisampling=1`
and a copy method exists: `NvAPI_D3D9_StretchRectEx` on native NVIDIA D3D9, or the driver's `RESZ` resolve where
`CheckDeviceFormat` reports it (AMD, Intel). Since 2.6.2, DXVK reports `RESZ` only for an AMD or Intel vendor id, so an
NVIDIA card behind DXVK has neither method unless DXVK reports AMD (`d3d9.customVendorId = 1002` in `dxvk.conf`).
`DxvkAmdVendor=1`, the default, does that without the file: when Luxvelum attaches, with the multisampling kept, an
NVIDIA driver installed (`nvapi.dll` in the system folder; the display enumeration of user32 does not belong in
`DllMain`), `d3d9.dll` not loaded yet and no `d3d9.customVendorId` in `DXVK_CONFIG`, it appends that option to the
process's `DXVK_CONFIG`, which DXVK reads with the same syntax as `dxvk.conf`; the log names the decision. The game and
DXVK then see an AMD card; without DXVK nothing reads the variable. Setting the variable just before the first
`Direct3DCreate9` was too late: DXVK 3.1.1 had read its settings already, its log named only `dxvk.conf` and the adapter
kept vendor `0x10DE` (owner's RTX 4060, 2026-11-23). Set while Luxvelum attaches, it works there: DXVK's log reads
`Found config env: d3d9.customVendorId = 1002`, the adapter reports vendor `0x1002` with `aticfx32.dll`, the game offers
its Multisampling choices and 4x is kept with `RESZ` copying the depth, with the line in `dxvk.conf` commented out (same
day, TEST Build-2-34). The multisampled depth is created as D24S8, since the game's D24X8 has no stencil for the water
tags and the silhouette split, and a self-test copies two known depths before the game sees the device; if it fails the
device stays single-sampled. The depth is copied before the water pass, after the water surfaces and before the fog, one
sample per pixel; scene copies become resolves, and water is tagged and shaded per sample. Where a pixel's 3×3 copied
depths are not planar, a pass marks its near and far samples in the stencil with an `oDepth` halfway between them
(clamped to the viewport, so the split uses MinZ 0 and MaxZ 1), and the composite fogs each side at its own depth, so a
pixel half tree and half sky blends both fogs by coverage.

**View distance.** Ascension's Extensions.dll detours the far-clip clamp (`0x780770`) and caps maps 0, 1, 530
and 571 at 791.66 yd; the engine allows 1583.33 and instances use it. With `FarClipMax` set, the DLL's calls
to the clamp lift that cap. Terrain loading, the chunk pool, the WDL horizon and the fog follow the far clip;
placed objects keep their own size-class culling (`environmentDetail`), and creatures the server's visibility
distance.

## Classic fog data

`tools/convert_classic_fog.py` converts the WoW Forever fog and lighting kit (build 1.60.1.70009, a folder or its
zip: the decoded `LightData`, `LightDataGlobalVolumeFog`, `LightParams` and `ZoneLightPoint` tables and the
colour-grading LUTs, checked against the kit's `SHA256SUMS`) into `data/fogdata.bin`. The kit has no decrypted `Light`
and `ZoneLight`, so lights and zone lights are carried byte for byte from a previous `fogdata.bin` (the shipped file
keeps those of `9bf495a`, from 69876) or read from the Classic client's `Light.csv` and `ZoneLight.csv` exports. It
checks that the fog table's layout hash is still `24290E20`, which keeps the column numbers valid:

```powershell
python tools/convert_classic_fog.py <kit folder or zip> --placements data/fogdata.bin data/fogdata.bin
```

The converter keeps the fog rows the Classic client selects (flag bit 3) at their layer index (0–2). At run
time the DLL blends the Classic lights around the camera: spheres at full weight inside the falloff start and
linear to the falloff end, the rest to the zone light whose outline holds the camera (fading in over 100 yd,
innermost outline on top), and otherwise to the map's global light. Each light uses the condition slot the
client uses for its stock lighting: the slot a screen effect forces (the ghost effect forces slot 4, death),
otherwise clear weather (slot 0) blended toward storm (slot 2) by the client's storm weight. The two time keys
around the current time are interpolated and layers are paired by their Classic layer index. Storm light params
without fog keys (31 lights, among them light 42 in the Eastern Plaguelands) keep the clear layers through the
storm instead of fading the fog out with the rain.

Then the Classic transforms apply: density ×0.01, heights relative to the player when flag bit 1 is set, shadow
colour and density for flag bit 0 while the light is below the horizon, `1 + strength·((d − start)/range)^exponent`
over the `MaxDistance` fog range, and scatter intensities up to 50 in linear light. The sun scattering is scaled
by the client's direct-light luminance over Classic's, capped at 1, which keeps the fog consistent with the older
client's darker lighting; this is a compatibility calibration, not a reproduction of the modern renderer.

Classic data applies on maps where any Classic light has fog, wherever Classic lights hold at least half of the
blend weight. A light without fog in the active slot counts with zero density, so the fog thins smoothly into it
and the distance fog hides the far clip there. Other maps use the derived layers.

Classic never had Outland (map 530), Northrend (map 571) or the Burning Crusade and Wrath dungeons, so the kit holds
no fog for them. `tools/convert_client_lights.py` adds them to `fogdata.bin` from the 3.3.5 client's `Light`,
`LightIntBand`, `LightFloatBand`, `LightParams` and `LightSkybox` tables (the Reforged HD client's `DBFilesClient`,
folder or zip):

```powershell
python tools/convert_client_lights.py data/fogdata.bin <DBFilesClient with darker nights> data/fogdata.bin
python tools/convert_client_lights.py data/fogdata.bin <DBFilesClient without darker nights> `
    data/fogdata-without-darker-nights.bin
```

The tool drops the lights, light params and zone lights an earlier run added before it adds them again, so running it
on its own output gives the same file.

Every client light on a map without Classic lights becomes a light of its own (id + 1000000), placed as the client
places it: world X = 17066.666 − z/36, Y = 17066.666 − x/36, Z = y/36, falloff /36, a light at the origin without
falloff stays map-wide. 257 Classic lights have a twin with the same id and position in the client table (light 77 at
the Stormwind harbour: (593586, 0, 931236)/36 → (−8801.0, 578.2), as in the Classic placement). Those with fog in their
clear slot are the templates: each new light takes the layers of the template whose 3.3.5 lighting is most alike,
compared every hour over the fog, sky top, sky middle, direct and ambient bands (linear) and the log of the fog
distance. Per condition slot and template key the layers keep their shape and grading curve; absolute heights move by
the new light's height above the template light, in 25 yd steps, so the ground fog of Terokkar Forest's Bone Wastes (its
lights 170 to 250 yd below their template) stays below the ground as the template's does instead of filling the valley.
Map-wide lights and lights at Z 0 keep the template's heights, as Z 0 marks a light placed without a height (234 Classic
lights, light 77 at the harbour among them). The Classic noise is left out, as `ClassicNoise=0` draws it: it showed
nowhere in Outland or Northrend and made Netherstorm look worse. The emissive, shadow and noise colours take the hue of
the new light's fog band (band 7) at the template's luminance scaled by the ratio of the two lights' fog-band luminance
(0.5–2), the sun scattering and the direct light likewise from the direct band (band 0), and the density follows the
ratio of the fog distances (float band 0, 0.5–2, median 0.87 over Outland and Northrend). The glow is the Forever glow
of the template's params in the same slot: the 3.3.5 `LightParams` glow drives the old client's bloom, is far higher
(1.0 in Ashenvale where Forever has 0) and washed the colours out when a test build used it on every map. Recipes shared
by several lights are written once; the run first drops the lights, params and zone lights it generated (ids from
1000000 up), so it is repeatable on its own output and leaves the Classic lights and zone lights byte for byte. Run it
again after `convert_classic_fog.py`, which keeps only the kit's light params and the zone lights the kit has points
for. 445 lights on 47 maps come from 194 templates, and one more light is a copy for Northrend's zone outlines (below;
3.2 MB in all).

The converted layers carry flag bit 4 where the client draws its sky without sun and moon: the light params' skybox
(`LightParams` column 2) lacks `LightSkybox` flag 0x2, which adds the procedural sky to the skybox model, or the sun
colour (`LightIntBand` band 9) is black all day. That holds for Hellfire Peninsula, Netherstorm, Shadowmoon Valley,
Zangarmarsh (`shadowmoon_sky_forest01.mdx` in the Reforged HD client), Blade's Edge, the Bone Wastes, Icecrown, the
Storm Peaks, Wintergrasp, Zul'Drak, Dragonblight, the Howling Fjord, Crystalsong Forest and Northrend's global light;
Shattrath City, Terokkar Forest, Eversong Woods and the Netherstorm eco-domes keep the sun, and Grizzly Hills, Sholazar
Basin and Borean Tundra keep it by the exceptions below. There the scene layers and the distance fog scatter neither sun
nor moon and no god rays are drawn. The Ghostlands carry the bit although the client draws a sun over them, as a tester
asked to take the sun scattering out of the zone (2026-10-31): the nine lights on map 530 whose clear slot is one of the
zone's own light params carry it in every slot. Those are 399 in six lights, among them 443, whose sphere (1889 to 2029
yd) covers the whole zone, 433 in lights 450 and 474 in its south, whose storm slot is the sunless
`legionnexus_volcanoskybox01.mdx`, and 448 in light 472 at the Zul'Aman gate; no other light on map 530 uses them. The
fade follows the client's own Ghostlands lighting: sampled every 50 yd at 18:00 in clear weather along north-south lines
from Y −5900 to −7700, the share rises from 0 to 1 between X 8250 and 8000 (from X 8400 at Y −6300, where light 465's
sphere overlaps Eversong's light 470), while Silvermoon City, Eversong Woods, Sunstrider Isle and the Isle of Quel'Danas
keep the sun. The 17 lights with Hellfire Peninsula's skybox (`hellfireskybox.mdx`) keep only the ground fog, at 0.3 of
the template's density, and carry flag bit 5: testers found the template's haze and far wall too strong there and its
fog on the sky out of place. Under that clear sky the distance fog is off and the scene layers fall off on sky pixels by
`exp(−upward·falloff)` with `falloff=−ln(1−clear)/0.005` (1842 fully clear, so the ground fog is gone from the sky a
tenth of a degree above the horizon; at a light's edge the share fades with the blend).

Northrend's global light is 1406. Four client lights on map 571 sit at the origin, which wowdev.wiki's `Light` page
calls the fallback where no other light applies, but 752 among them has a falloff (32.6 to 46.1 client units). The
converter and the DLL take only an origin light without falloff for map-wide, so 752 becomes a sphere of about 1 yd at
(17066.7, 17066.7), the corner of the map grid, that reaches no reachable position, and the DLL's background is the
first map-wide light in file order, 1406 (1407 and 1408 repeat it; which origin light the client itself shows is not
known). 1406 borrows the Howling Fjord's sky, `fiord.mdx`, whose sun band is black, and is the only light over 90% of
Grizzly Hills, 63% of Sholazar Basin and 88% of Borean Tundra, so the rule above took the sun from those zones, which
testers missed. Two exceptions on map 571 give it back. The skyboxes `shalozar.mdx`, `shalozar(red).mdx`,
`ashrand_sky01.mdx`, `8riv_stormsongsky01.mdx` and `10drg_sky01.mdx` (Coldarra, part of Borean Tundra) keep the sun in
every slot that uses them, ahead of both tests; the 32 lights with them have their centres in those three zones
(`10drg_sky01.mdx` also serves the Oculus, map 578, which keeps the rule), but Borean Tundra's light 1386
(`ashrand_sky01.mdx`, falloff 247 to 347 yd) reaches about 240 yd into Wintergrasp and Dragonblight and causes most of
their change below. And two zone outlines, Borean Tundra with Sholazar Basin as one, so that no sunless seam opens at
their shared border, and Grizzly Hills, hold light 2001406, a copy of 1406 whose clear and storm slots keep the sun: the
same layers, colours, densities, glow and grading, only flag bit 4 dropped, while its other slots, the ghost's spirit
sky among them, are 1406's own. The copy sits at the client origin's world position without falloff, so it is not
map-wide and its sphere reaches no reachable position; it acts only through the outlines (zone lights 1000000 and
1000001, heights −64000 to 64000 as the Classic outlines have, vertices traced from the 3.3.5 `WorldMapOverlay` subzone
boxes and simplified to 150 yd, good to about 150–300 yd at the inner borders). `fiord.mdx` stays sunless, as the
Howling Fjord's own sky, and so does light 1411, the Howling Fjord's 611 yd highland light over Camp Winterhoof. Sampled
every 50 yd at an estimated ground height (+5 yd), at 18:00 in clear weather, the DLL's blend now leaves Grizzly Hills
12.6% sunless (99.0% before: 1411 on the southern rim, the 100 yd fade inside the outline and the Zul'Drak cliff
lights), Sholazar Basin 7.5% (100%) and Borean Tundra 4.7% (99.3%), while Icecrown (96.4%, from 96.6%), the Storm Peaks
(99.8%), Dragonblight (99.7%, from 100%), the Howling Fjord (96.2%, from 96.4%) and Wintergrasp (98.7%, from 100%) stay
sunless and Outland is unchanged. These shares are an offline estimate over zones labelled by the overlay boxes, not a
measurement in the game.

`OutlandDensity` and `NorthrendDensity` start at 0.15 when the key is missing, as the fog read as too strong there in
play (the shipped INI sets 0.5 and 0.7), and `OutlandClassicFog=0` or `NorthrendClassicFog=0` brings back the derived
layers with the client's glow. `AzerothSkyColor`, `OutlandSkyColor` and `NorthrendSkyColor` (0 when the key is missing;
the shipped INI sets 0.5, 1 and 1) mix the scene layers' emissive and shadow colours at run time toward the hue of the
client's current stock fog colour, which blends the scene into its sky and follows the weather, at each colour's own
luminance: the stock colour itself is far darker than the Classic emissive (0x282A2A against about 0.5 at dusk in the
Bone Wastes), so taking it whole would blacken the fog.

The region of these settings follows the camera, not the map alone. Map 530 holds Outland and also the Burning Crusade
starting zones (Eversong Woods, the Ghostlands, Silvermoon City, the Isle of Quel'Danas, Azuremyst and Bloodmyst Isles
and the Exodar), which lie in Azeroth but have a region of their own, `TBCStart`: `TBCStartDensity`, `TBCStartSkyColor`
and `TBCStartLocalLights`, and their converted client lights whatever `OutlandClassicFog` says. With the Azeroth values
(TEST Build-2-4) their lamps glowed far weaker than Elwynn's: at Falconwing Square at midnight the converted ground fog
held 0.0020/yd against Goldshire's 0.0078/yd, under a moonlit fog about seven times brighter, and the tester found the
lamps right again at local lights 4 with density 1; the region starts at 2.5, the owner's choice. Outland is the part of
map 530 at y ≥ −2550 yd. In the shipped `fogdata.bin` the 155 Outland lights of map 530 reach no further than y = −727
yd and its 23 starting-zone lights no closer than y = −4373 yd, so the line passes about 1800 yd from both; the land of
both lies within its lights. While the camera position cannot be read, map 530 counts as Outland. The `map` lines of
`Luxvelum.log` name the region, as in `map 530 (TBCStart) at (7595 -6782 87)`.

The phase of the Classic sun and moon scattering peaks at 1 toward the light. The authored intensities rise as g
falls, to about 4π for evenly scattering rows, which suggests that the modern lookup is energy-normalised (inferred;
it is not in the kit); `ClassicPhase=1` multiplies the scattering by `k(g) = (1+g)/(4π(1−g)²)`, capped at g = 0.95.

**Authored noise.** Layers with flag bit 2 modulate their density as the modern global fog kernel does (shader
6674335, variant 002): two octaves sample a tileable 3D noise volume at `frac((p − offset_i)·inverseTile_i)`, their
average passes through a fixed S-curve of contrast 20 about 0.5, and `f = lerp(1, curve, alpha)` scales the density
while the emission moves toward the fade colour by `1 − f`, so storm fog becomes patchier and about half as dense.
It is off when the key is missing (`ClassicNoise=0`), as a tester judged the storms without it right (2026-10-25); the
shipped INI turns it on (`ClassicNoise=1`) since it took another tester's settings (2026-11-16). Alpha is the share
of a layer's blend weight that carries noise, and each octave scrolls as a phase in tiles, so a
blend that changes the tile size scales the pattern about the camera. The modern `t_perlinNoise3D` is not in the kit;
the volume here is a 64³ tileable Perlin noise built at load. The water's reflection fog and the far-clip check take a
noisy layer at its mean. A march at the mean is more opaque than the noisy march on average: each long far step takes
the noise of one point, the curve is nearly binary, and transmittance averaged over dense and clear patches lets more
light through than a uniform half density. The lit composites once marched silhouettes that every low-resolution tap
missed at the mean, and in rain the trunks and hill crests before brighter fog showed bright specks and streaks (a
tester's screenshots in Mulgore, `ClassicNoise=1` against `0`; the specks follow from the code, the match is
inferred). The lit noisy composites sample the noise in that march as the lit noisy march does; to stay within the 32
temporary registers of ps_3_0 they leave out the density variation (`NoiseAmount`), which a noisy layer takes only at
weight `1 − alpha`, so a silhouette marched again in a layer without noise lacks the variation's patches, at most
`±NoiseAmount` of its density. On grazing water the rows between the low-resolution taps match no tap; where the lit
composite marched them again at the mean they showed as bright horizontal lines one low-resolution texel apart
(inferred from the code and a tester's screenshot of grazing water in rain; not yet confirmed in game), so the lit
noisy composites, which draw noisy fog wherever a point light is marched, interpolate the filtered fog of the taps on
the pixel's plane instead. They take the filtered fog rather than the current march, whose samples jitter each frame,
so that the noise cannot turn the jitter into shimmer on every other row (inferred).

`fogdata.bin` format 5 holds the lights, the light params they reference (with `LightParams.Glow`), the fog keys (with a
grading curve index), the layers, the zone lights and their outlines, the grading curves and the band probes; format 4
is the same without the probes, and the loader rejects any other format. The layer noise columns are inferred
(`FogData::UnpackNoise`): c4 the fade colour, c16–18 and c19–21 the octaves' scroll directions, c27[i] octave i's tile
in hundreds of yards, c28[i] its drift in yd/s; c24 is carried raw and unused. `LightData.ColorGradingFileDataID` names
a 32³ LUT; every LUT placed lights reach applies one curve alike to R, G and B, so the file keeps 32 codes per LUT and
the converter fails otherwise (`DarkerColorGradingFileDataID` is left out). The curve is interpolated between the
nearest keys that set one, wrapping past midnight (inferred). Glow and grading blend like the fog, on every map with a
placed light.

The converted maps depend on the client's `LightIntBand`. Project Reforged ships it in two variants, with its optional
"darker nights" patch and without; the darker variant scales the night keys of 10735 of its 15568 bands to about 0.66
(median of the changed channels, as low as 0.09) and never empties the sun and moon band 9 of a light params that keeps
it in the other, so the sunless decision is the same in both (2026-11-19, the owner's `LightIntBand_darker_nights` and
`LightIntBand_sem_darker_nights`). Fog converted from one variant is therefore about a third too dark or too bright at
night for a client with the other. `tools/convert_client_lights.py` writes 64 band probes into the file it makes: id,
key 0 and colour of every n-th band (by id) whose first key is at midnight with a colour other than black, from the
`LightIntBand` it converted from. Of the darker variant's 64 probes one matches the other variant. The package ships
the darker variant as `fogdata.bin` and the other as `fogdata-without-darker-nights.bin` (2026-11-22, both from the
owner's tables of 2026-11-18). With the variants converted into `fogdata.bin` and further `fogdata*.bin` files in
`Data\Luxvelum`, Luxvelum reads the probes of every such file at start-up, scores each against the client's loaded
`LightIntBand` at the first world frame after the table is loaded, and loads the file whose probes match best if more
than half match; otherwise, and with a single file, `fogdata.bin` stays. The log names every file's score. The client
keeps the table as a `WowClientDB` at `0x00AF49BC`, vtable `0x00A419E0`; its exit destructor at `0x009DCB90` installs
that vtable and releases the table through `0x007EB1B0`. The loader at `0x007EB540` (vtable slot 2) checks the file's 34
fields of 0x88 bytes (`DBFilesClient\LightIntBand.dbc` from `0x008BE100`), keeps the record count at `+0x08`, sets
`+0x0C` to 0 and `+0x10` to `0x0FFFFFFF` before the records come in, and marks the table loaded at `+0x04`. Its record
lookup at `0x007EB210` takes an id between `+0x10` (lowest) and `+0x0C` (highest) and reads the record pointer from the
index at `+0x20`, `[+0x20 + (id − lowest)·4]`; a record is the file's: id, key count, 16 times, 16 colours at `+0x48`.
The fog code calls it at `0x007EBF5D` with `ecx = 0x00AF49BC` and the id `params·18 − 17 + band`, the converter's
numbering.

## Forever water data

`tools/convert_forever_water.py` converts the WoW Forever water kit (a folder or its zip: decoded `LiquidType`,
`LiquidTypeXTexture`, `LiquidTypeXFFTTile` and `FFTTile` tables and the foam BLP textures, checked against the kit's
SHA-256 manifest) into `data/waterdata.bin`:

```powershell
python tools/convert_forever_water.py <kit folder or zip> data/waterdata.bin
```

It holds four presets (Forever liquids 1240 generic lake, 1288 generic river, 1250 generic ocean, 1290 WMO interior),
their seven FFT tiles and six foam masks (1024², full mip chain, alpha as 8-bit coverage with a fitted linear
colour). Forever's CPU packing of the table fields into its GPU structure is not in the kit, so the mapping is
inferred from value ranges and shader use: absorption from `-ln(Color[0])` scaled by `Float[0]`, crest and trough
scattering colours from `Color[1]`/`Color[2]` with `Float[2]`/`Float[1]`, scattering weights `Float[3..6]`, depth,
shore and wave foam `Float[7..19]`, sun and reflection roughness `Float[27]`/`Float[29]` and reflection strength
`Float[30]`; tile size, amplitude, wind alignment and multiplier from `FFTTile` fields 0–3 and the foam and oxygen
rates from fields 4–9. The water is tuned by eye against this mapping, not matched to Forever captures.

## How it attaches

- **Loader.** `Wow.exe` loads `Luxvelum.dll` at startup, before the client resolves Direct3D, through the Luxvelum
  loader of St0ny's AIO WoW EXE Patcher; `DllMain` does the rest and no export is called. A `Wow.exe` without that
  patch loads it through the optional `version.dll` proxy instead: all 17 exports forward lazily to the system copy,
  and its static import of the `vf_loader_anchor` export loads `Luxvelum.dll` before the client starts.
  Loaded with `LoadLibraryA`, the DLL pins itself before it patches anything, so no `FreeLibrary` can unmap the code
  the hooks call, and an exception during start-up is logged and leaves the load successful with the hooks installed
  so far.
- **D3D9.** The client resolves `Direct3DCreate9` through the delay-loaded `GetProcAddress` slot `[0xB2ED98]`.
  The DLL points that slot at a filter that returns a wrapped `IDirect3D9`. No d3d9 code is patched, so DXVK or
  other `d3d9.dll` builds keep working underneath.
- **Depth.** The wrapper creates the device without auto depth and binds an `INTZ` texture as the depth-stencil,
  which the client caches as its world depth. Multisampling is kept only as described below, and
  `D3DCREATE_PUREDEVICE` is removed. The shaders read depth through the captured world viewport's range and
  treat anything deeper as beyond the far clip.
  When the game asks for multisampling and a depth copy passes its self-test (see Antialiasing), the device keeps
  the game's multisampled back buffer and automatic depth (D24S8 for D24X8) instead, and `INTZ` becomes a copy.
  The `INTZ` and D24S8 support checks pass a fullscreen back buffer's display format as the adapter format:
  X8R8G8B8 for A8R8G8B8 and X1R5G5B5 for A1R5G5B5, since D3D9 displays no alpha and DXVK refuses those formats as
  adapter formats. Which format the client asks for in fullscreen is not confirmed; windowed devices use the
  desktop's mode. Without `INTZ` the wrapper binds a plain depth of the game's format and turns the fog off; when
  that cannot be created either, it creates or resets the device again with the game's own parameters, so the
  game keeps its automatic depth.
- **Hooks.** Five 5-byte call displacements: the world render call (`0x4FB03D`, stock-fog override and restore),
  after the opaque M2 pass (`0x4F911D`, captures camera inputs), the liquid surface pass (`0x4F9170`, forces
  depth writes), world-name text (`0x7E5818`, draws the fog unless it already composited this frame, then
  suppresses depth writes), and before the frame effects (`0x4F9281`, draws the fog over the completed world when
  the world text did not). The original bytes are checked first; on any mismatch nothing is patched. Two more
  retarget the far-clip clamp calls (`0x780810`, `0x781444`) when `FarClipMax` is set at start-up, independently of
  the fog hooks.
- **Transparent fog hooks.** The M2 batch fog call (`0x81FD15`) and the glare pass call (`0x4F9213`, then draws
  the fog before the world text unless it already composited this frame), installed
  after the fog hooks whatever `TransparentFog` says, and neither when a byte run checked under M2 batch fog, M2 fog
  setter or Native glare differs. The M2 thunk forwards to the client while nothing is armed and otherwise hands the
  cdecl argument block to the DLL (`pushad; lea eax, [esp+0x24]`); an exception in it turns transparent fog off.
  The unfogged batch's fog upload call (`0x81FD29`) is retargeted after them, on its own: when a byte run checked under
  M2 unfogged batch differs it stays unpatched and unfogged effects stay unfogged. Its thunk hands the batch (`esi`)
  and the upload's argument to the DLL, which calls the setter with the fitted line and turns the upload's 0 into 1.
- **Water hooks.** Installed after the fog hooks and only when those installed, since the water needs the wrapped
  device and the frame-end hook of the world render call: the water pass call (`0x790AA2`) and the `Render` slots of
  the two water material vtables (`0xA5954C`, `0xA59580`, read-only data patched under `VirtualProtect`). The call
  site, both slots and the layout bytes the classification reads are checked first; on any mismatch none are patched
  and the fog is unaffected. `waterdata.bin` is loaded only once they are installed. A water exception restores the
  device state, turns water off for the session and leaves the fog running.
- **Glow and grading.** No patch: the glow override runs before the fog composite and the grading in the world
  render's frame-end hook, each only while its guarded bytes match. An exception turns that one off for the session,
  or both when it comes from their shared inputs, and `GlowCompensation` then uses the clamped day/night glow.
- **State.** Every state the passes touch is captured with a recorded state block and restored, plus render
  targets, depth and stream 0 (whose offset state blocks drop). The client's shader-constant cache stays valid.
  The fog and grading passes restore them in a termination handler, so an exception inside a pass, which the
  hook's guard catches, still hands the client back its own targets and state, and releases the references that
  would otherwise block the next `Reset`.
- **Overlay.** When a fog device is created, the device window's procedure is chained so the settings window
  sees input first, and the wrapper's `Present` draws the window over the finished frame.

## Engine inputs

All addresses are static in the 12340 image (base `0x400000`, timestamp `0x4C2452FE`, both checked at start-up);
Gx is the graphics device `[0xC5DF88]`.

| Input | Address |
|---|---|
| View, projection | Gx `+0x1B00 + 64·[+0x1AF8]`, `+0xF88`; copies at `0xADF5E8`, `0xADF628` |
| World viewport depth range (MinZ, MaxZ) | Gx `+0xF80` |
| Camera position / look-at target | `0xCD8F5C`, `0xCD8F68` |
| Camera focus: the unit the camera follows | `0xCD7778` (stored by `0x780860`), else the camera position |
| Current map | `0xADFBC4` (read by the far clip setter), else `0xAB63BC` |
| Day fraction | `0xD38B04` |
| Stock fog groups: colour, start, end (read by the render callbacks) | `0xD38B8C`–`0xD38B94`, `0xD38BA0`–`0xD38BA8` |
| Zone fog distance (light float band 0) | `0xD38C1C` |
| Light colours: ambient, direct, sun | `0xD38BD4`, `0xD38BD8`, `0xD38BF8` |
| Visible sun / moon sprite, sky centre | `0xD38E28`, `0xD38E48`, `0xD38B18` (day window `[0xA41CA4, 0xA41CA0]`) |
| Camera in liquid | `0xCD8794` |
| Storm weight (0..1) blending the clear and storm light params | `0xD38B88` |
| Light params slot forced by the current screen effect (−1 = none) | `0xD38B58` |
| Glow: `ffx` CVar, current effect, glow effect, glow amount | `0xD45774`, `0xD45780`, `0xB74364`, `0xD38C2C` |
| Ghost (death) screen effect | `0xB74368` |
| Glow byte: pass list count and array, pass [2], its colour's alpha | `+0x0C`/`+0x10`, `+0x20`/`+0x24`; `+8`; `+0x33` |
| Far clip | From the projection; `[[0xB7436C] + 0xB14]` as a fallback |
| World M2 scene (point lights) | `[0xCD754C]` |
| Camera WMO instance, group ids, group count, interior fog blend | `0xCD87A4`, `0xCDB0DC`, `0xCDB0D8`, `0xD38B9C` |
| Sky colours (bands 2–7: top, middle, band 1, band 2, smog, fog) | `0xD38BE0`–`0xD38BF4` |
| Water colours: ocean close/far, river close/far | `0xD38C0C`, `0xD38C10`, `0xD38C14`, `0xD38C18` |
| Liquid renderer, transparent (water) draws queued this frame | `[0xCD8610]`, `[[0xCD8610] + 0x14]` |
| Liquid settings bank: count, entries | `0xD43B18`, `0xD43B1C` |
| `LiquidType` rows: maximum id, minimum id, row pointers | `0xAD4070`, `0xAD4074`, `0xAD4084` |
| Object manager, visible list: link offset, first object | `[[0xC79CE0] + 0x2ED0]`, `+0xA4`, `+0xAC` |
| Object: descriptors (type mask `+8`), GUID, list link, world entity | `+0x08`, `+0x30`, `+0x38`, `+0xB8` |
| Unit: movement block (pointer, embedded block), transport GUID, raw position | `+0xD8`, `+0x788`, `+0x790`, `+0x798` |
| Unit movement flags (swimming), collision radius and height | `+0x7CC`, `+0x850`, `+0x854` |
| World entity: world position, liquid flags, liquid surface | `+0x6C`, `+0x7C`, `+0x80` |
| Client `waterRipples` value | `0xADF7F0` |
| Current zone: `AreaTable` id of the zone, of the subzone; zone name | `0xBD080C`, `0xBD0810`; `[0xBD0788]` |

Engine notes behind the code:

- Current zone. The Lua functions `GetZoneText` `0x515570`, `GetRealZoneText` `0x5155A0`, `GetSubZoneText`
  `0x5155D0` and `GetMinimapZoneText` `0x515600` (registered at `0xAC8378`–`0xAC8390`) return the strings at
  `[0xBD0788]`, `[0xBD0780]`, `[0xBD0784]` and `[0xBD077C]`. The one writer of those strings, `0x5204C0`
  (`55 8B EC A1 0C 08 BD 00`), stores its first argument in `0xBD080C` and its second in `0xBD0810`, and raises the
  new-area event (`0xA8`) only when `0xBD080C` changes. Its one caller, `0x78F1DD` in `0x78F020`, looks up the
  area at the player in the `AreaTable` rows (`[0xAD3154]`, ids `[0xAD3144]`..`[0xAD3140]`) and, when the row
  has a parent area (`+8`), passes the parent's id as the zone and the area's id as the subzone; otherwise the
  area is the zone and the subzone is 0. So `0xBD080C` holds the zone of the client's map and zone texts (Redridge
  Mountains in Lakeshire, Stranglethorn Vale in Booty Bay). Read from the 12340 `Wow.exe` on 2026-11-18.

- Call sites. `0x4FB03D` (`E8 5E DE FF FF`) calls the world render `0x4F8EA0`, a thiscall on the world frame
  with no stack arguments; the thunk keeps ECX across the frame-begin hook. `0x4F911D` (`E8 8E AB 32 00`) calls
  the opaque M2 pass `0x823CB0`, a thiscall with one stack argument (`ret 4`). `0x4F9170` (`E8 AB 5E 28 00`)
  calls the liquid surface pass `0x77F020` (no arguments, outside liquid only). `0x4F9281` (`E8 8A 7D 3C 00`)
  calls FFX end `0x8C1010` (no arguments); the thunk renders the fog, then tail-jumps to it.
- Loader slot. On disk `[0xB2ED98]` holds the delay-load thunk `0x41C654`, which pushes the slot (`0x41C65F`)
  before resolving it; `0x41C673` jumps through it. The hooks install while the slot holds the thunk or, when the
  loader in `Wow.exe` runs after the client's first call through it, the `GetProcAddress` the thunk resolved
  (kernel32's or KernelBase's export; the filter calls its own import either way). Any other value is another hook:
  the engine, transparent fog and water hooks are not installed, while `LightStay` and the far-clip calls still
  apply, and `Luxvelum.log` names the state it found. A `Direct3DCreate9` resolved before the DLL loaded is not
  wrapped, and the hooks then find no fog device.
- Matrices. `0x6821A4` reads the camera-relative view matrix at Gx `+0x1B00 + 64·[+0x1AF8]` and then the projection at
  `+0xF88`. The engine builds its projection with `0x6BF370` (OpenGL depth range, w = view z); the D3D backend
  converts it.
- Depth. The world viewport's MaxZ is `[0xADEEE4]` = 0.94 (loaded at `0x4F9019`, passed to GxXformSetViewport
  at `0x4F905A`) and uploaded as `D3DVIEWPORT9::MaxZ` by the D3D9 backend (`0x6A9ACC`). The Gx viewport is
  stored by `0x681890` and uploaded lazily on the next draw or clear (`0x6A99E0`), so after the opaque pass the
  device can still hold the sky's `[0.999, 1]`; the capture reads MinZ/MaxZ from the Gx viewport, which the sky
  and WDL passes restore (`0x7F0CB3`, `0x796466`). The distant WDL terrain draws into `[0.998, 0.999]` with its
  own viewport and projection (`0x7960EB`); the sky viewport is set at `0x7F0A79` and the sky writes no depth,
  so depth at or above max(deepest world depth, 0.99903) is sky.
- Multisampling. The Video options' multisample list (`0x54F1B0`) is built once per process (it returns while
  `[0xBEA744]` is set) by `0x68A170`, which drops each count of `0xAD8CE8` = {0, 2, 4, …, 16} that the wrapped
  `CheckDeviceMultiSampleType` refuses (`0x68A2B9`, `0x68A2DE`). `gxMultisample` (`0xCABCF8`) asks for `gxRestart`,
  not established to reset rather than recreate the device, so `CreateDevice` (`0x68F4DE`) and `Reset` both choose
  between the copied and the bound `INTZ` depth. The present-parameter builder `0x68E250` takes the depth format from
  `0xA2E4A8` = {…, D16, D24X8, D24S8, D32} at `[0xCABCE8]`, where `gxDepthBits` "24" stores D24X8 (`0x7692D0`).
- Depth copy. `NvAPI_D3D9_StretchRectEx` copies a multisampled D24S8 depth into a registered `INTZ` texture, one
  sample per pixel. The NVAPI functions come from `nvapi_QueryInterface` with the IDs of NVIDIA's public headers:
  `0x0150E828` Initialize, `0xA064BDFC` RegisterResource, `0xBB2B17AA` UnregisterResource, `0x22DE03AA` StretchRectEx.
  `nvapi.dll` is loaded by its full path in the system folder (`GetSystemDirectoryW`, which WOW64 redirects to
  SysWOW64 for the 32-bit client) with `LOAD_WITH_ALTERED_SEARCH_PATH`, so its imports are also searched there
  first; the `LOAD_LIBRARY_SEARCH_SYSTEM32` flag would need KB2533623 on Vista and Windows 7. A failed load is logged
  with its Windows error.
- Liquid depth. While writes are forced on, the wrapper records the client's own `D3DRS_ZWRITEENABLE` requests
  and re-applies the last one afterwards, so the client's render-state cache stays accurate.
- World-name text. `0x7E5818` (`E8 23 76 ED FF`) calls `0x6BCE40`, a cdecl wrapper that takes the font batch
  `[0xD380A8]` and tail-jumps to `0x6C53A0`. The batch is created by `0x6BF160(1, 1)` at `0x7E6511`; its bit 0
  at `+8` selects world rendering (`0x6C5564`), which requests depth testing (Gx state 13, `0x6C5591`) and depth
  writes (Gx state 15, `0x6C55BE`, dispatched as `D3DRS_ZWRITEENABLE` at `0x6A8FC7`). Suppressing the writes
  keeps glyph quads out of the depth the fog is integrated against, where they would end fog rays at the text,
  while keeping the draw and its depth test. The text is drawn without depth writes after lightning, missile arcs and
  the glare, so a fog composite after it fogs each glyph by the depth behind it: names in front of the sky took the
  sky's full fog and vanished, while the same name in front of a nearby tree stayed visible. The thunk therefore draws
  the fog first when it has not composited this frame, as the stock client leaves names unfogged, and the world-done
  hook then skips it. A name in front of a tree was still reported with the sky's fog beside the trunk with only this
  hook, so the glare thunk at `0x4F9213` draws the fog when the glare pass returns, just before the world text call
  (`0x4F9218`) that its byte guard checks; the text hook stays as the fallback while the glare thunk is not
  installed. With the glare thunk installed, world text the client draws before the glare pass returns does not draw
  the fog: with `TransparentFog=0` and a unit targeted, the fog was drawn at such text, before the see-through models
  of M2 pass 1, and banners in front of the targeted unit lost their fog (Darkshire, 2026-11-22). The later name/icon
  path at `0x4FB042` is left in place.
- Native glare. `0x4F9213` (`E8 58 76 2F 00`) calls the sun/moon glare pass `0x7F0870`, which is not patched.
  Its draw `0x9AC400` adds the `sunGlare`/`moonGlare` quads (`D3DBLEND_SRCALPHA`, `D3DBLEND_ONE`) at depth
  `[0.9990234375, 1]`, and its update `0x9AC3C0` runs a GPU occlusion query against the current world depth
  (`0x9ABE00`). World geometry `0x7984A0` also runs after the opaque hook (`0x4F9154` calls `0x77F010`, which
  jumps to it). Drawing the fog once at `0x4F9281` sees the completed depth and keeps the glare and its query
  order unchanged.
  With `TransparentFog=1` the liquid thunk calls the glare pass instead of its own call, after checking `0x4F9210`
  (`83 C4 14`), `0x4F9218` (`E8 63 C3 2E 00`, world text) and the pass after its entry, which Extensions.dll detours:
  `0x7F0877 74 3C D9 05 48 8B D3 00 51 B9 A8 8E D3 00`, the moon tail jump `0x7F08AB B9 58 8F D3 00 E9 4B BB 1B 00`
  and `ret` at `0x7F08B5`. The entry `0x7F0870 83 3D CC 8C D3 00 00` is only logged (see Extensions.dll). The call
  at `0x4F9213` itself is patched (see Transparent fog hooks).
- Sun and moon sight line. The glare's visibility fallback `0x9ABC60` traces from the camera toward the light: it
  starts half a yard out (float 0.5 at `0x9E2EC4`), ends at the camera plus the light direction times the far clip
  `[camera+0x40]`, pushes the hit flags (`0x9ABC79 68 12 01 20 00`) and calls `0x77F310` at `0x9ABD18`
  (`E8 F3 35 DD FF`). `0x77F310` (`55 8B EC 5D E9 57 48 02 00`) jumps to `0x7A3B70`
  (`55 8B EC 83 EC 18 8B 45 18 83 05 C4 04 CE 00 01`), a cdecl `bool(const float* start, const float* end,
  float* hit, float* fraction, uint32_t flags, 0)`: the fraction is read and written and starts at 1, the hit point
  is written only on a hit, and it returns `al`. The DLL checks the thunk's and the entry's bytes before its first
  trace and calls the thunk with the glare's flags under an exception guard; a fault turns the trace off and is
  logged once. The second glare test, `0x4F9410` on the world frame `0xB7436C`, is not used.
- World render order. In `0x4F8EA0` the map render draws the sky, the WDL, terrain and the WMOs with their alpha
  batches, then M2 pass 0 (`0x4F911D`). Above water come M2 pass 2 (`0x4F9167 6A 02`), the liquid (`0x4F9170`),
  weather, the barrier effect and M2 pass 1 (`0x4F918C 6A 01`), under water pass 1, weather, the liquid (`0x4F91B0`)
  and pass 2; then lightning, missile arcs, the glare (`0x4F9213`), world text (`0x4F9218`) and FFX end (`0x4F9281`).
  `0x821A20` puts see-through batches (blend mode above 1 or alpha below 0.99999, `0x821F47`) in pass 1 on the
  camera's side of the model's liquid plane and in pass 2 beyond it. The liquid pass sets the Gx fog, draws the water
  (`0x790AA2`) and tail-jumps to the ripples `0x79D5E0`.
- M2 batch fog. `0x81FB10` picks the fog colour by blend mode through `0xA45390` = {1, 1, 1, 2, 2, 3, 4, 0}
  (`0x81FB7B 8B 04 8D 90 53 A4 00`) and the jump table `0x81FE7C` (`0x81FBA3 FF 24 85 7C FE 81 00` → `0x81FBAA`,
  `0x81FCAE`, `0x81FCC2`, `0x81FCD8`): mode 1 is the lighting fog colour with alpha 0xFF (`0x81FC5D C6 45 FB FF`),
  modes 2, 3 and 4 black, white and grey 0x80 with alpha 0 (`0x81FB20 33 DB`; `88 5D FB` at `0x81FCB7`, `0x81FCCD`
  and `0x81FCE3`), mode 0 unfogged. From `0x81FCEE` to `0x81FD14` it pushes the exponent (`[eax+0xB4]` of the
  lighting `[esi+0x70]`), the colour's address (`lea ecx, [ebp−4]`), the end (`+0xAC`) and the start (`+0xA8`),
  calls the setter at `0x81FD15` (`E8 F6 34 05 00` → `0x873210`), then `0x873390(1)`, and pops the arguments
  (`0x81FD1A 6A 01 E8 6F 36 05 00 83 C4 14`).
- M2 model placement. The batch state `esi` of `0x81FB10` is the M2 scene's render state: the element loop at
  `0x8239E0` walks draw elements of 0x44 bytes and keeps the current one at `+0x50`, its type at `+0x58` and its
  model at `+0x60` (`0x823A1A 8B 40 04 89 46 60`, element `+4`); the lighting is the model's `+0x2A8` at `+0x70`.
  The transparent elements sort by the float at element `+0x10` (comparator `0x81EF40`), which `0x821930` copies
  from the model's `+0x88`. `0x82E0C0` multiplies the model's 4×4 matrix at `+0xB4` (`0x82E349 8D 96 B4 00 00 00`)
  by the camera matrix through `0x4C1F00` into `+0xF4` (`0x82E354 8D BE F4 00 00 00`) and stores the squared length
  of that product's translation, `+0x124`..`+0x12C` (`0x82E37F D9 86 2C 01 00 00 D9 86 28 01 00 00 D9 86 24 01 00
  00`), as `+0x88` (`0x82E3A2 D9 9E 88 00 00 00`), or a parent's `+0x88` for an attached model. The DLL reads the
  translation row of `+0xB4` (`+0xE4`) as the model's world position and uses it only where its squared distance to
  the camera matches `+0x88` within 2% (at least 1 yd²), so an attached model keeps the screen's line.
- M2 unfogged batch. The batch is `esi` throughout `0x81FB10` (`0x81FB18 8B F1`); seven calls reach it (`0x82040D`,
  `0x8209E6`, `0x820B3E`, `0x820FA6`, `0x8210CD`, `0x821606`, `0x82307D`). It reads the material
  (`0x81FB58 8B 8E 98 00 00 00`), takes mode 0 when its flags have 0x2 (`0x81FB61 F6 01 02`) or the lighting's fog
  at `+0xB0` is not above 0, and otherwise the table entry of the blend mode at material `+2`
  (`0x81FB77 0F B7 49 02`). Mode 0 jumps (`0x81FB86 3B C3 89 86 80 00 00 00 0F 84 92 01 00 00`) to
  `0x81FD26 53 DD D8 E8 62 36 05 00 83 C4 04`, the upload `0x873390(0)` with `ebx` = 0. The upload
  (`55 8B EC 83 EC 10 83 7D 08 00 74 5D`) with 1 sets vertex c30 from the setter's stored constants at `0xD43068`
  (`0x8733C6 68 68 30 D4 00 6A 1E 6A 00 FF D2`, written at `0x87327F A3 68 30 D4 00`), with 0 c30 = (0, 1, 1, 0)
  (`0x87341D`), fog 1 at every depth; nothing else writes c30. The setter is called only from `0x7A84BA`,
  `0x7A9B35` (map object fog from the day/night fog at `0xD38B8C` or `0xD38BA0`) and `0x81FD15`.
- M2 fog setter. `0x873210` (`55 8B EC 83 EC 14 83 3D 20 30 D4 00 00`) writes the colour's bytes +2, +1 and +0 over
  255 (`0x873225 8B 75 14 0F B6 46 02`, `0x873242 0F B6 4E 01`, `0x873254 0F B6 16`) to pixel c2 and builds vertex
  `c30 = (−1/(end − start), end/(end − start), exponent, 0)` from the start and end (`0x873263 D9 45 0C D9 45 08 D8
  E9`) and the exponent (`0x87328C D9 45 10`). The client's M2 and WMO shaders fog by planar view depth, `oFog =
  min(pow(max(c30.x·z + c30.y, 0), c30.z), 1)`, blending `lerp(c2, colour, fog)`. c2.w is the alpha-test reference,
  so the DLL changes the colour only through the setter's argument; each WMO render resets its fog cache.
- Weather fog. `0x78AE20` draws weather either with its own grey fog and a vertex fade (`0x78A7C7`) or with the Gx
  fog set to 0x80808080 from 70 yd (`0x7893B5`) to 75 yd (`0x7893C6`); no weather draw reads the zone fog.
- Extensions.dll. Ascension's Extensions.dll (TimeDateStamp `0x6ABAD5C2`, read statically) detours the glare pass
  `0x7F0870` (7 bytes; it then draws a second moon), the LightParams getter `0x7EB180` (it multiplies the glow by its
  `ambientGlow` CVar, so the glow can exceed 1), the light blend `0x7F3230` and M2 and DayNight functions. None
  touches `0x4F9170`, `0x81FD15`, `0x873210`, `0x4F9213`, `0x4F9281`, `0x4F8770`, `0xD38C2C` or the FFX code, or
  overlaps a byte run checked before the M2 unfogged batch hook; at `LogLevel=2` the log says what `0x7F0870` begins
  with. The runs of that hook (`0x81FB18`-`0x81FD30`, `0x873390`, `0x8733C6`, `0x87327F`) and its call `0x81FD29`
  were not checked against it; when one differs the log says so and the hook stays off.
- Screen effects. FFX end runs the current effect `[0xD45780]` when the `ffx` CVar (`[0xD45774]`, int at `+0x30`)
  and the effect's own CVar (`+4`) are on. The glow effect `[0xB74364]` keeps `ffxGlow` there (`0x8BFEDB`);
  `0x4F8770` feeds it the DayNight glow (`0xD38C2C`) as the additive weight of `lerp(screen, blur, other) +
  g·blur²`, where `other` is the screen effect's own blend amount, which the fog leaves as is.
  The ghost effect is `[0xB74368]` (`FFXDeath`, vtable `0xA418D8`, stored at `0x7EA274` by its constructor).
- Glow feed. The world frame callback `0x4FAF90` rewrites `0xD38C2C` from the light blend every world frame before
  the world render. There `0x4F8F3D`, the only call of `0x4F8770`, feeds the glow effect while it is current: it
  turns the glow into a byte with the +512.0 trick (`0x4F878E`–`0x4F87AC`, floor(255·g) mod 256, so 1.1 gives 24) and
  passes {underwater, glow byte, blur byte} with id 3 to SetParam (`0x4F883C 8B 40 10 52 6A 03 FF D0`), slot
  `0xA941D8` of the glow vtable `0xA941C8` (stored at `0x8BFE98 C7 03 C8 41 A9 00`). SetParam `0x8BFDE0` (`55 8B EC 8B
  45 0C 8B 10 89 51 2C 8A 50 04 8A 40 08`) stores the D3DCOLOR {B = G = R = blur, A = glow} at `+0x30` of pass [2] in
  the clear-view list at `+0x08` (`0x8BFE14 8B 41 10 8B 48 08`, `0x8BFE21 89 51 30`) and the underwater list at
  `+0x1C` (`0x8BFDF2 8B 71 24 8B 76 08`, `0x8BFE08 89 7E 30`), and returns with `ret 8` (`0x8BFE26 C2 08 00`). A list
  is {capacity, count, array}; pass [2] is the composite (vtable `0xA94294`, stored at `0x8C2206 C7 06 94 42 A9 00`),
  which hands that colour to its draw (`0x8C28A1 83 C6 30`). `0x4F8F3D` precedes both liquid pass calls, so when the
  liquid pass returns the byte holds this frame's value. The ghost composite `0x7E87B0` reads `0xD38C2C` itself. The
  glow override requires these byte runs and `0x4F9286`, and each frame an effect graph with these vtables.
- FFXGlow. The shader loader `0x684970` accepts only BLS version `0x10003`, so Ascension's ps_3_0 `FFXGlow` falls
  back to ps_2_0, `lerp(screen, blur, v0.z) + blur²·v0.w` with the pass colour as `v0`. Every FFX pass end rebinds
  the default target (`0x8C15A7 8B 01 8B 50 5C 6A 00 6A 00 6A 00 FF D2`), so the composite draws to the back buffer.
- After the frame effects. After FFX end the world render only runs `0x747AE0` (`A1 68 13 CA 00`, clearing flag
  `0x1000` along `[0xCA1368]`) and returns (`0x4F9286 E8 55 E8 24 00 5F 5E 8B E5 5D C3`); names and icons follow
  (`0x4FB042 E8 F9 A0 2E 00`). Early returns skip FFX end and the world-done hook, and with them the grading, which
  requires `0x4F9286`, `0x747AE0`, `0x4FB042` and `0x8C15A7`. None of these guarded bytes is one the hooks patch.
- Light params slots. `0x7EB180` returns a light's `LightParams` for a slot (`Light` record `+0x1C + 4·slot`).
  For each light, `0x7EE510` takes slot 0 and, while the storm weight `[0xD38B88]` is above zero, blends in
  slot 2 by it (`0x7EC220`). The DayNight update sets that weight to `min(1, 4·[0xD38B4C])` (`0x7F3995`); the
  weather update writes `[0xD38B4C]` (`0x784A01`). When `[0xD38B58]` holds a slot, the light blend `0x7F3230`
  uses it instead (`0x7F346F`). `0x7ECEC0` stores it from the current `ScreenEffect` row (`+0x1C`, called at
  `0x4F712D`; values above 7 become −1) and `0x7ECEE0` clears it. In CoA's `ScreenEffect.dbc` the ghost effect
  (ID 1) and the other death-style effects use slot 4; a few event effects force slots 0, 1, 2, 3 or 5.
- Current map. The world-transfer loading screen `0x40AD50` (`LoadingScreen.cpp`) stores its map argument in
  `[0xAB63BC]` (`0x40AD56`, `A3 BC 63 AB 00`); `0x40AE0A` and `0x40AECF` set it to −1. In-game logs show map 0 in Elwynn
  Forest. The `farclip` CVar setter `0x780800` reads the map it passes to the clamp from `[0xADFBC4]` (`0x780803`,
  `A1 C4 FB AD 00`, the 5 bytes whose FNV-1a hash Lumenveil checks as `0xF496B995BC571E34`), so the DLL reads the map
  from `0xADFBC4` while those bytes match and from `0xAB63BC` otherwise (the log says which). Lumenveil reports −1 at
  `0xAB63BC` after a ship or zeppelin trip; with −1 the fog took no Classic data and Azeroth's region settings,
  Northrend's density and lamps included.
- Camera focus. The look-at target `0xCD8F68` is the camera position plus its forward row, one yard ahead
  (`0x4FA7C1`–`0x4FA80F`, camera vtable `0xA1EA54` slot 1 `0x600C20`); logged frames show 1.00 yd between the two. The
  world render looks up the object whose GUID the active camera `[[0xB7436C] + 0x7E20]` follows (`camera + 0x88`,
  `0x4FA664`–`0x4FA696`), and when `[camera + 0x48]` is 0 and the object exists it takes the object's position
  (`0x4FAAA0`, `8B 8F 20 7E 00 00 83 79 48 00 75 13 85 DB 74 0F`; vtable `+0x2C` at `0x4FAAB0`), else the camera
  position, as the third argument of `0x7831A0` (`0x4FAADF`). `0x7831A0` hands it to `0x780860` (`0x7831F0`,
  `0x783209`), which first stores it at `0xCD7778`–`0xCD7780` (`0x780871`, `A3 78 77 CD 00 8B 4F 04 89 0D 7C 77 CD 00`;
  `0x78087F`, `8B 57 08 89 15 80 77 CD 00`); nothing else writes there. The DLL reads the focus there while both stores
  match, and the camera position otherwise (the log says which).
- Far clip. The clamp `0x780770` is cdecl `float(float farclip, int mapId)`, result in ST0, caller pops; it
  bounds the value to `[0xA3E708]` (183.33) .. `[0xA3E710]` (1583.33). Its calls at `0x780810` (in the
  `farclip` CVar setter `0x780800`, also reached from Extensions.dll on zone changes) and `0x781444` (map load
  `0x781430`) are rare, so the hook re-reads the INI on every call.
- Present. The D3D9 backend presents through the device at Gx `+0x397C` (`0x6A3584`, `0x6A7724`), which is the
  wrapper, so every frame passes through its `Present`.
- Input. `GxWindowClassD3d` and `GxWindowClassD3d9Ex` share the window procedure `0x6A0360`, which hands keyboard
  and mouse messages to the Gx input callback. The message pump's pre-filter `0x86CB00` acts only on the client's
  own dialog windows, and DirectInput only enumerates game controllers (`0x870725`), so keyboard and mouse
  reach the chained window procedure. The client draws its own D3D cursor (`0x6A009C`, `0x6A058E`), so ImGui
  leaves the cursor alone.
- World lights. `0x4F90EC` loads the world M2 scene `[0xCD754C]` and passes it to the opaque M2 pass. The
  point-light registration `0x834C70` allocates a 64-by-64 pointer table at scene `+0x24`, hashes the light's
  world X/Y in 20-yard cells and inserts it into a null-terminated list. A light holds its scene at `+0`, type at
  `+8` (1 = point), world position at `+0xC`, diffuse RGB at `+0x3C`, constant/linear/quadratic attenuation at
  `+0x54`, enabled state at `+0x60`, the address of its preceding link at `+0x64` and the next light at
  `+0x68`; removal `0x834AB0` and the setters `0x835690`/`0x8356F0` maintain the links. Animated M2 positions
  are already transformed to world space before the setter (`0x828AF4`). The diffuse already includes the
  animated intensity and model scale (`0x8305B9`–`0x8305FE`) and is uploaded unchanged (`0x835527`,
  `0x6A462F`). The attenuation is uploaded as `D3DLIGHT9::Attenuation0/1/2` (`0x835539`, `0x6A46AC`) with the
  fixed range 10000 (`[0xA2F95C]`), so the 200-yd cap on a fog light's sphere and the eight-light selection are
  the fog's own policy. No native spotlight cone has been established.
  Registration skips a disabled light (`0x834C79`, `83 7E 60 00`) and the enable setter `0x8356F0` re-registers or
  unlinks it (`0x835716`, `0x835720`–`0x835740`), so the table holds only enabled lights.
  Lights carry a frame stamp at `+4` that the client writes only when the light's model is drawn (`0x828B9C`), and
  the map light query `0x81E400` disables every light with a stale stamp through `0x8356F0`, so a torch that leaves
  the frustum is unlinked from the table the fog walks; the `je` at `0x81E548` is the branch that skips that
  disable call (St0ny's AIO WoW EXE Patcher No. 70 "lightstay" turns its opcode `74` into `EB` at file offset
  `0x41D948`, which its patch table records as holding `74` in the stock exe; with the image base `0x400000` and
  `.text` at RVA `0x1000` for raw offset `0x400`, that file offset is VA `0x81E548`). `LightStay=1` writes the same
  byte in memory at start-up, after `EngineHooks=1` passed the image check, when the opcode is `74` or `EB` and the
  block the `je` skips (rel8 5..64) holds a `call` to `0x8356F0`; the bytes around the branch have not been
  recorded from a disassembly, only this structure, so the guard is looser than the byte guards of the other sites
  and the patcher itself lists No. 70 as untested. Lights whose model is not drawn then keep their last position
  and colour; removal of the model still unregisters them (`0x824D30`).
- Interiors. `0x795D40` stores the camera's WMO instance at `[0xCD87A4]`, its active group IDs at `[0xCDB0DC]`
  and their count at `[0xCDB0D8]`. The instance's root is at `+0xF4`; the root's loaded flag is `+0x1E0`, its
  group count `+0x1F4` (set at `0x7D7F18`, bounded at `0x7AE467`) and inline group pointers start at `+0x1F8`;
  group `+0x198` bit 0 means loaded (`0x7AEA80`). The client's camera-fog query at `0x7A11C7` accepts an interior
  group when its `+0x30` flags have neither bit in `0x48` set. Day/night fog update `0x7F17B5` writes the
  transition weight to `[0xD38B9C]` at `0x7F1931` and from `0x7F1955` blends the outdoor fog toward the WMO fog,
  so the stock fog inputs already hold the blended colour, start and end.
- Capture guards. Before reading the light and interior inputs, the module checks the image timestamp and base
  and the instruction bytes at `0x4F90EC`, `0x834C8D`, `0x834D3C`, `0x835539`, `0x7A11B0`, `0x7A11C7`,
  `0x7AEA83` and `0x7F1931`. Reads are SEH-guarded with pointer, span and count limits (512 nodes per bucket,
  links checked against their owner); no client references survive the capture. A disabled or non-point node is
  skipped and counted, a walk beyond 8192 nodes stops as truncated, and a damaged link, a table that changed during
  the walk or a read fault end the walk; in each case the lights read before it stay usable and the reason is
  logged once. The interior inputs are read after the walk whatever it returned and kept when their reads were
  consistent; a read fault leaves them exterior. An unsupported client or an out-of-range camera gives an empty
  result. `LocalLights=0` skips the point-light walk.
- Water pass. `0x77F020` jumps to `0x790A80`, which calls the liquid renderer `0x8A2240` at `0x790AA2`
  (`E8 99 17 11 00`) with ECX = `[0xCD8610]` (`0x790A91`), the camera `0xCD8F5C` and pass 1 (`0x790A9B`, `6A 01`);
  it is a thiscall that returns with `ret 8` (`0x8A2376`). The same `0x77F020` runs from `0x4F9170` (camera above
  liquid) and `0x4F91B0` (below). Liquids are queued into one of two 16-byte buckets by their `LiquidMaterial`
  flag (`0x8A20D6`, settings `+0x360`): pass 1 holds every water and ocean material, pass 0 the magma and slime,
  drawn earlier in the map render (`0x79ACF1`). `0x8A2240` keeps the renderer in EBX (`0x8A224C`), indexes the
  bucket by `pass·16` (`0x8A229C`), loads its count from `+4` (`0x8A229F`), skips its draw loop when that is 0
  (`0x8A22B2`) and empties the bucket every frame (`0x8A2314`–`0x8A2319`). The water ripples `0x79D5E0` run after
  the call and are not part of the pass. Each instance is drawn by its material's vtable slot 2 through `call edx`
  (`0x8A22C7`–`0x8A22EA`), a thiscall with seven stack arguments, the last the `Liquid::CMaterialSettings*`:
  `CMaterialWater::Render` `0x8A5590` (vtable `0xA59544`, slot `0xA5954C`) and `CMaterialWaterNoSpec::Render`
  `0x8A5900` (vtable `0xA59578`, slot `0xA59580`), both `ret 0x1C` (`0x8A58FB`, `0x8A5C6B`). Stock water is
  alpha-blended without depth writes on D3D9 hardware, and the client never uses the stencil: its D3D9 backends set
  no stencil state and its clear (`0x6A74B0`) passes only the target and depth flags.
- Water classification. The settings bank `0x8A28F0` bounds a liquid type id by `[0xD43B18]` (`0x8A28F8`) and
  keeps one settings object per id in `[0xD43B1C]` (`0x8A2900`, `0x8A2952`), so the draw's settings map back to its
  id. The settings builder `0x8A27C0` copies `LiquidType.Texture[k]` (`+0x3C`, `0x8A280C`) into the settings at
  `+0x80·k` (`0x8A2809`, `0x8A282E`). `LiquidType` rows are looked up as at `0x793DF7`–`0x793E12`; the material id
  is `+0x38` (`0x8A1FE8`) and the sound bank `+0x0C` (0 water, 1 ocean; from the 45-field `LiquidType.dbc` layout).
  A draw is water when its material is 1 and its first texture lies in a `\river\` or `\ocean\` folder, which
  excludes CoA's liquid 181 (orange slime drawn with the water material); liquid 17 (WMO water, interior) is
  indoor, sound bank 1 ocean, liquid 9 (fast water) river, and the rest lake.
- Water contacts. `0x4D77A9` (`mov ecx, [0xC79CE0]; mov [ecx+0x2ED0], eax`) stores the object manager; `[0xC79CE0]`
  is null before login. The visible list starts at `[mgr + 0xAC]` (`0x4D4B44`, `add eax, 0xA8; mov eax, [eax+4]`) and
  steps to `[obj + [mgr + 0xA4] + 4]` (`0x4D4B80`, `add eax, 0xA4; mov eax, [eax]; add eax, ebx; mov ebx, [eax+4]`),
  link offset 0x38 (`0x4D6193 BA 38 00 00 00`, `0x4D6233 89 96 A4 00 00 00`); a set low bit ends it, and each link's
  `[obj + 0x38]` holds the previous link's address, which the walk checks. The GUID is at `+0x30` (`0x4D4B6D 8B 4B 34
  8B 43 30`), the type mask at `[[obj + 8] + 8]`, unit bit 0x8 (`0x4D4DF1`, `test [ecx+8], edx`). A unit's `+0xD8`
  points at its movement block `+0x788` (`0x73F67A`, `lea eax, [esi+0x788]; mov [esi+0xD8], eax`): raw position
  `+0x10` (`0x6E6F13`, `mov edx, [ecx+0x10]`), transport-local while the transport GUID `+0x8` is set (`0x6E6F70`,
  `mov eax, [ecx+0x790]`), movement flags `+0x44`, swimming 0x200000 (`0x730DA2 8B 40 44 A9 00 00 20 00`), collision
  radius and height `+0xC8`/`+0xCC` (`0x6E95AF D9 9E C8 00 00 00`, `0x6E95CE D9 9E CC 00 00 00`). The world entity is
  at `+0xB8` (`0x7438B0 89 86 B8 00 00 00`), its world position at `+0x6C` (`0x7803BD`, `mov [ecx+0x6C], edi`; the
  feet in world space, inferred, used only for passengers). The entity's liquid refresh `0x7A1BC0` sets 0x20 with the
  surface at `+0x80` (read at `0x77F1EA`, `test byte [eax+0x7C], 0x20; jz; fld [eax+0x80]`) and 0x100 when the surface
  lies above the feet in a liquid with `LiquidType` flag bit 0 (read at `0x77F230`, `mov ecx, [eax+0x7C]; …; shr ecx,
  8; and ecx, 1`). The client's ripple emitter `0x71CBA0` emits only while surface − z is below max(1, 2·height)
  (`0x71CC28`, `fld [esi+0x854]; fadd st, st; fld1`), at strength 1 at half that depth to 0.5 at it, and the swim
  update `0x730D10` splashes when the depth crosses 0.4·height (`0x730DED`) while not swimming. The walk calls no
  client code, is SEH-guarded with pointer and range checks, rejects a frame with a broken back link and stops at 4096
  objects.
- Client ripple sprites. `waterRipples` is a console command registered at `0x7813A4` (`68 90 F6 77 00 68 00 E8 A3
  00`); its handler, the only writer, reads its argument into `0xADF7F0` (`0x77F696 68 F0 F7 AD 00 68 9C 28 9E 00`).
  The only reader, `0x79D463` (`83 3D F0 F7 AD 00 00 74 61`), makes no new sprite while it is 0; it is reached only
  through `0x77F434` (`E8 27 E0 01 00`) from the unit emitter `0x71CF16` (`E8 E5 24 06 00`). The update `0x79D5E0`
  expires each sprite (`0x79D648 D9 41 28 A1 58 FB AD 00 D8 1D A4 76 CD 00`, at most 0.7·e s) without reading it.
  Extensions.dll holds neither the address nor the emitter's functions (a byte search).
- Water colours. The DayNight light record at `0xD38BD4` (`0x7F3574`) stores the sky bands 2–7 at `0xD38BE0`–
  `0xD38BF4` (`0x7EC03C`–`0x7EC09C`) and the ocean and river close/far colours at `0xD38C0C`–`0xD38C18`
  (`0x7EC11D`–`0x7EC152`); the client bakes the water colours into its per-frame depth ramps. The water is lit by
  the direct light (band 0, `0xD38BD8`), which `0x7EE756` copies to the world's direct light; band 9
  (`0xD38BF8`) only colours the sun and moon sprites (`0x7F36F6`).
- Not used. `0xD38B98` is fog group 0's exponent, which Extensions.dll overrides (group 0's end is `0xD38B94`), and
  `0xD38C9C` is a near-constant model lighting direction, not the visible sun, so the fog's light direction
  follows the sprite positions.

## In-game settings

`Insert` (the shipped `OverlayKey`; `Ctrl+F7` when the key is missing) shows and hides a Dear ImGui window over the
game. On laptops whose F-keys send media keys by
default, hold Fn (the log names the key that arrived). The window edits every setting below except `Enable`,
`EngineHooks`, `Overlay`, `OverlayKey` and `ToggleKey`, and the next frame uses the change.
**Save** writes the settings changed in the window back to `WTF\Luxvelum.ini`, keeping the comments and every other
line; settings not changed in the window keep what the file holds. **Revert** reloads the file, and a hand edit of the
file also replaces unsaved changes. The window shows whether the fog drew in the last frame, or why it did not. It also
shows whether the game's multisampling is kept, or why it is off (`Multisampling` applies later, see Settings). It can
be moved and resized, and keeps its place and size for the session. Its title bar names the build, the branch and
commit of `BUILD.txt` (as in `Luxvelum reforged-dev ec0af3b`; `local` for a build outside the CI), and so does the
first line of `Luxvelum.log`.

The **This zone** section names the client's current zone and sets its sun and moon halo, factors on top of `SunScatter`
and `MoonScatter`, and its fog density, a factor on top of `Density` and the region's density that scales every layer of
the fog (each 0..4, 1 = the global value alone, a density of 0 clears the fog), kept as `[SunHalo]`, `[MoonHalo]` and
`[Density]` entries by `AreaTable` zone id with the zone's name under `[Zones]`. The shipped halos, the owner's tuning,
are in `Data\Luxvelum\zones.ini`, which every release replaces and Luxvelum never writes; the player's own go to
`WTF\Luxvelum-zones.ini` when a slider is let go, which no package contains, and win over the shipped ones for the zones
they list. A zone neither lists keeps 1; **Reset this zone** removes the player's own entry and falls back to the
shipped halo. The section and the log name the source (`shipped`, `your own`, `not listed`). The owner's own entries are
folded into `data/zones.ini` by `tools/fold_zone_halos.py`, his values winning, so the players' copies of the shipped
halos follow his corrections while their own edits stay. The halo follows the zone the map shows (Booty Bay belongs to
Stranglethorn Vale), so it does not depend on which Classic light covers the place: Redridge has none of its own and
takes Eastern Kingdoms' global light, which a per-light factor would have shared with the Badlands, Arathi and the sea.
Crossing into another zone fades the factors over about two seconds (time constant 0.6 s); a slider takes effect at
once. While the client reports no zone (id 0, for a second in Duskwood in TEST Build-2-9), the last zone's halo stays.
Each zone change writes a `zone 44 (Redridge Mountains): sun halo …, fog density …` line to the log. A file without a
`[Density]` section, such as one saved before the density existed, leaves every zone's density at 1. The density takes
effect where Luxvelum builds the frame's settings (`ConfigForFrame`), so the volumetric fog, the see-through models'
fitted fog and the fog over the water follow it alike; the local lights' gain, which evens out the regions' densities,
does not compensate it, so a denser zone also makes its lamps glow more. The owner judged the halo per zone, not per
region: Durotar right, the Barrens and Booty Bay far too large, Tirisfal nearly right (2026-11-18, `ClassicPhase=1`).

While the window is open, clicks and the wheel go to it only while the cursor is over it or a drag started on
it, and keys only while one of its text fields is active (Ctrl+click on a slider). The hotkey never reaches the
client; mouse moves and key-ups always do, so no game key sticks. The window is drawn in the wrapper's
`Present` with the device state saved and restored, and releases its resources before every `Reset`. An
exception in the overlay turns it off for the session and is logged; `Overlay=0` leaves the game window
untouched from the next start, unless `ToggleKey` is set.

### Switching Luxvelum off

`ToggleKey` and the window's **Luxvelum on** box switch Luxvelum off and on while the game runs. The shipped INI
leaves `ToggleKey` empty, so no key is taken until a player sets one (same names as `OverlayKey`, for example
`Ctrl+F9`); a key that arrives as `OverlayKey` does is refused and logged (Windows sends `Ctrl+Pause` and
`Ctrl+ScrollLock` alike), and the key itself never reaches the client. The window procedure only flips a request, and
the frame-begin hook (`vf_on_frame_begin`) takes it once per world frame, so every hook of a frame sees one state and
the log notes each change. Every start begins switched on; nothing is saved. While it is off, a frame that finds
neither a `ToggleKey` nor the window to switch it on again (`Overlay=0` and no `ToggleKey`, or the overlay stopped
after an exception) switches it on and logs why, so it never stays off until a restart.

Switched off, each hook takes the path it already takes when the matching setting is off:

- the stock fog is not pushed out of range, no fog, god rays or early composite is drawn, so the M2 batch fog
  arguments and the glare pass stay the client's (as without fog), and the liquid surface and the world-name text keep
  the client's depth writes;
- the water pass is not armed and its copies, wave maps and foam masks are released at the end of the frame, as with
  `Water=0`, and the client's `waterRipples` value is given back;
- the glow byte is not fed and the grading copies are released, as with `ForeverGlow=0` and `ColorGrading=0`;
- the transparent fog counters of `LogLevel=2` stop, so the M2 batch fog calls go straight to the client;
- the INI is still re-read every second, and the settings window reports the fog and water as not drawn because
  Luxvelum is switched off.

Switched on again, the first frame keeps the stock fog and draws the fog after the world, as after a loading screen, and
the early composite returns with the next frame. Nothing needs a reset: the temporal history is used only within 0.25 s
of the last drawn frame, the local-light stabiliser restarts after 1 s without a step, the sun's sight line is traced
again, and the water is re-created as when `Water` is set back to 1. Not switched, because they are set up once: the
graphics device (INTZ depth, the multisampling choice, no `D3DCREATE_PUREDEVICE`) and the patched call sites, which pass
through; the far clip lifted by `FarClipMax`, which the client clamps again only at a view distance change, a zone
change or a map load; and the `LightStay` byte, an instruction of the client's code written at start-up. With
`Overlay=0` at start-up, the window procedure is still chained when `ToggleKey` is set, so the key works without the
window and `Overlay=1` works again without a restart; without a `ToggleKey` at such a start, the window procedure stays
the client's, and a `ToggleKey` set later works only from the next start. `Save` takes `Overlay`, `OverlayKey` and
`ToggleKey` from the INI, so a hand edit of them made since the last reload is kept. Each hotkey swallows only its own
key-up, so a key released while the other hotkey is held still reaches the client. The log names the keys that arrive
with Ctrl, Shift or Alt, and `OverlayKey` and `ToggleKey` themselves, as for `OverlayKey` on laptops.
While the window procedure is chained, the game window's `WM_DESTROY` writes `game window closed` to the log, the
last line of a session the client ended normally; a session whose log stops without it is still running or crashed,
which is how an install into the owner's client tells whether the game is closed. With `Overlay=0` and no `ToggleKey`
the procedure stays the client's and no such line is written.

## Build

Requirements: Visual Studio 2022 (C++ x86), the Windows 10/11 SDK (`fxc.exe`), CMake 3.20+. The first configure
downloads Dear ImGui v1.92.9b (FetchContent, pinned by SHA-256) into the build directory.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Outputs in `build/Release`: `Luxvelum.dll`, the optional `version.dll` loader and `vfog_harness.exe`, laid out like a
client folder with `Luxvelum.ini` copied into `WTF` and `fogdata.bin`, `waterdata.bin` and `zones.ini` into
`Data\Luxvelum` on every
build, also of `--target Luxvelum` alone and when only the INI or a data file has changed. The DLL the harness loads
reads them there and writes `Logs\Luxvelum.log`.

### Download a CI build

The [Windows release build](https://github.com/jealous-sound/coa-vfog/actions/workflows/release.yml) runs on every
push to `main` (including merged pull requests), on pull requests targeting `main`, and manually from
**Actions → Windows release build → Run workflow**. The manual button becomes available after the workflow is
merged into `main`; select a branch to rebuild that revision.

Open a successful run and download its `Luxvelum-Win32-<commit>` artifact under **Artifacts**. GitHub supplies a ZIP
containing `Luxvelum.dll`, `WTF/Luxvelum.ini`, `Data/Luxvelum/fogdata.bin`, `Data/Luxvelum/waterdata.bin`,
`Data/Luxvelum/zones.ini`, the README,
licenses and a `BUILD.txt` identifying the commit and validation. Artifacts are kept for 30 days; rerun the workflow to
rebuild an expired download. Pull-request artifacts are builds of GitHub's proposed merge revision. The optional
`version.dll` loader comes with the licence as a second artifact, `Luxvelum-version-dll-Win32-<commit>`.

On a push to the release branch `reforged-main`, and on a manual run of that branch, a second job publishes the same
files as a GitHub release `v<version>` titled `Luxvelum v<version> (reforged-main <commit>)`, `<version>` being the
`MAJOR.MINOR.PATCH` in `VERSION`, which the build also compiles into the DLL for the settings window's title and the
log's first line. The release's notes start with `RELEASE_NOTES.md`, its "What's new", above the install text. Its
assets are `Luxvelum-Win32.zip`, `Luxvelum-version-dll-Win32.zip` (the optional `version.dll` loader and the licence)
and `Luxvelum.ini`, the same file as `WTF/Luxvelum.ini` in the zip, attached on its own for the Project Reforged
Launcher, which installs `Luxvelum.dll` and `Data/Luxvelum/` from the zip and that INI as its settings preset. They
need no login to download. The job then deletes every other release with its tag, test releases included, so the
repository keeps only the newest release at `releases/latest/download/Luxvelum-Win32.zip`. A commit that leaves
`VERSION` at a version released from another commit publishes nothing and fails the job, so each release raises
`VERSION`; a rerun for the commit that `v<version>` already points at uploads the assets to that release again. A
manual run of `reforged-main` with `refresh` set rebuilds `v<version>` at the new commit: it moves the tag to the
commit and replaces the title, notes and assets of the release in place, so the release is never deleted and keeps its
publication date. GitHub may refuse to move a tag with the workflow's `GITHUB_TOKEN` when files under
`.github/workflows` changed since the tagged commit; such a refresh fails and leaves the release as it was. A refresh
uploads the new zip as `Luxvelum-Win32.new.zip` before it moves the tag, then renames the old zip away, gives the new
one its name and deletes the old one, so the download is missing only between those two renames, never while an upload
runs; if the upload fails, the release stays as it was. A push whose commit message carries `[skip ci]` runs nothing, so
a fix can join the current release this way.
On a push to the development branch `reforged-dev`, and on a manual run of it, a test job deletes every test release
(`v<version>-test.<k>`, or `build-<n>-<k>-test` from before the versions) and, when `reforged-dev` has commits that
`reforged-main` lacks, publishes `v<version>-test.<k>` as a pre-release titled
`Luxvelum TEST v<version>-test.<k> (reforged-dev <commit>)`, which never becomes `releases/latest`, `<version>` being
`reforged-dev`'s `VERSION`. `<k>` is one more than the successful runs of the workflow on `reforged-dev` since the
newest release was published, which a refresh does not change, or one more than the test build of that version it
replaces when that is higher, so testers can tell the test builds apart. The two branches publish in parallel, so the
test job checks again after creating its release and deletes it when `reforged-main` has taken the commit meanwhile. A
build whose branch has already moved on to a newer commit gets no release and deletes nothing; other branches and pull
requests publish none.

CI uses Windows Server 2022, MSVC 2022 and `fxc` to build Win32 Release, then runs the comment check and the CPU
harness. The CPU harness exercises the existing data, fog model, local-light/interior, settings persistence (with the
takeover of an old `Luxveil.ini` or `CoAVolFog.ini`), glow and water-contact checks before any D3D9 device is created.
CI runs CTest with `-LE gpu`; the full `vfog_harness` remains enabled by default for a Windows machine with a suitable
graphics device. A successful CI build does not establish graphics correctness or in-game appearance and performance.

## Test

```powershell
ctest --test-dir build -C Release --output-on-failure
```

This runs the comment check, the CPU-only `interior_fog` transition and INI persistence checks, and
`vfog_harness`, which creates a real D3D9 device through the wrapper with the
client's flags (`0x52`, auto depth D24X8), renders a Z-up test scene with the client's projection convention,
and runs the fog passes through the same entry the hook uses. Its checks, in `tests/`, cover the device wrapper
and state restoration, depth and sky handling, Classic light blending and slot selection, the march against CPU
integrals at every quality, temporal filtering and upsampling, point lights and interiors, the text and liquid
depth overrides, fog-data validation, the GPU timer and depth probe, the settings window and INI saving, and
`Reset`. The water suites check the water data and its loader, the FFT against a double-precision reference, the
liquid classification, the water pass driven through the hook entry points (state restoration, stencil tagging,
optics against a CPU reference, fault recovery) and the water settings. They do not establish in-game appearance or
performance. It writes `before.png`, `after.png`,
`overlay.png` and the debug views to `build/harness-out`.

Further suites check the authored noise and `ClassicPhase`, the transparent fog, glow and grading through the hooks, a
4x device (`SKIP` without a copy method), and the ripples. The display dither checks draw an opaque fog ramp from code
30 to 38 through each composite, both fixed-function blends and the late god-ray composite: every column must average to
the continuous code (0.1 rms, 0.2 at most), while exact codes without fog and the debug views stay unchanged. The
grading checks compare stripe means over 32 rows with the curve (0.2 at most) and allow each pixel the dither. The
god-ray mask checks draw one mask texel over a depth buffer at an odd subviewport offset: with sky in one row of its
footprint it must read 0.25 and with sky in all but that row 0.75, and a one-pixel column of geometry must leave it at
0.75 in each of the footprint's four columns and at 1 beside it. A third check gives the four half-resolution scene
texels under the mask texel 1, 0.5, 0 and 0 at a threshold of 0.4 and its neighbours 1: the mask must read 0.3. The
previous mask, one depth tap and one colour per texel, fails all three checks (Wine's Direct3D on llvmpipe; the new mask
passes them there).

`vfog_harness --scene harbour <dir> --data data/fogdata.bin` renders the logged in-game frame at the
Stormwind harbour (sunset, far clip 791.6 yd) with ideal depth and with the client's depth range, and
prints fog opacity and colour at probe points next to a CPU integration.
`--classic-phase 1` renders it with `ClassicPhase=1`, and `--storm 1` in a full storm (noisy layers at their mean).

`vfog_harness --scene ripples <dir> --water-data data/waterdata.bin` renders standing, running, walking, swimming
and wading-in units at `WaterRipples=0.5`, each as the shaded frame, its normals and a calm reference.

`vfog_harness --scene performance` times the fog passes at 1920×1080 at each quality on one street: derived
layers with no point lights, eight flood lights or eight street lamps, Classic layers at the harbour with and
without the lamps (skipped if the `fogdata.bin` of `Luxvelum.dll` does not resolve the harbour), and the shipped
`LogLevel=1`. It prints `quality,case,point_lights,median_ms,p95_ms` of GPU time; this is a controlled renderer
cost, not a game frame-rate test.
It also times the harbour in a full storm and the colour grading pass; `--samples 4` creates a 4x device.

## Install

Patch `Wow.exe` with the Luxvelum loader of St0ny's AIO WoW EXE Patcher. Close the client, then unzip the package into
the client folder: `Luxvelum.dll` goes next to `Wow.exe`, `fogdata.bin`, `waterdata.bin` and `zones.ini` into
`Data\Luxvelum` and `Luxvelum.ini` into `WTF`, in the client's own folders for its data and settings; a player's own
`WTF\Luxvelum-zones.ini` is not in the package and stays. Without the patch, unzip
`Luxvelum-version-dll-Win32.zip` next to `Wow.exe` as well: its `version.dll` loads `Luxvelum.dll`, and a patched
`Wow.exe` does not need it. Remove `Luxvelum.dll` to uninstall, with the `version.dll` loader too, which keeps the
client from starting on its own; the patched `Wow.exe` then starts without it. The DLL writes `Luxvelum.log` into the
client's `Logs` folder and makes `Logs` and `WTF` when they are missing; where it cannot, the log and the settings stay
next to `Luxvelum.dll`. A `fogdata.bin` or `waterdata.bin` missing from `Data\Luxvelum` is still read from next to
`Luxvelum.dll`, where earlier packages put it. The start-up line `Luxvelum loaded from` names the settings and data
files in use. Deleting the `WTF` folder to reset the client deletes Luxvelum's settings too: the next start takes over
an earlier INI still next to `Wow.exe` again, as below, or else runs on the built-in defaults until `WTF\Luxvelum.ini`
is unzipped from the package again. A client started from the same folder while another holds the log writes the first
free one of `Luxvelum-2.log` to `Luxvelum-8.log` beside it instead, and its first line names that file and the client's
process id.
A slot held by a running client is left as it is; a free one is reused and overwritten, so the clients of one folder
share at most eight log files. A ninth client running beside eight others writes no log.
Delete the `version.dll` of a CoAVolFog or Luxveil package: it keeps the client from starting once its old DLL is gone.
The `version.dll` of an earlier Luxvelum package loads this `Luxvelum.dll` too; Build-1 and TEST Build-2-1 to 2-3
shipped none, and their `Luxvelum.dll` lacked the `vf_loader_anchor` export it needs.
To keep the settings of an earlier install, leave out `WTF\Luxvelum.ini`: on its next start the DLL then copies the
`Luxvelum.ini` next to `Wow.exe` of an earlier package, also one with the `[LuxVelum]` section of the builds spelt
`LuxVelum`, or without it `Luxveil.ini`, or `CoAVolFog.ini`, into `WTF\Luxvelum.ini` with its section renamed
and logs which file it took; an existing `WTF\Luxvelum.ini` is never replaced, and the earlier file stays as it was. A
file saved as UTF-8 or Unicode (UTF-16) and a section header followed by more text are taken over too; a file without
its section is skipped with a log line, and the next one is tried. The `Luxvelum.ini`, `Luxvelum.log`, `fogdata.bin` and
`waterdata.bin` of an earlier package next to `Wow.exe`, and the old `Luxveil.dll`, `Luxveil.ini`, `Luxveil.log`,
`CoAVolFog.dll`, `CoAVolFog.ini` and `CoAVolFog.log`, are then no longer used and can be deleted; delete the earlier INI
once the log names it as taken over, so that a later reset of `WTF` does not bring it back.

## Steam Deck and Linux (Proton)

Install the files as above. With the loader in `Wow.exe` no `WINEDLLOVERRIDES` launch option should be needed; this has
not been tested on Proton yet. The optional `version.dll` loader needs the launch option
`WINEDLLOVERRIDES=version=n,b %command%`, so that Proton loads it before its builtin version library. A Steam Deck test
on 8 October 2026 (Proton with DXVK, AMD VanGogh, 1280x800), then through a `version.dll` proxy and
`WINEDLLOVERRIDES=version=n,b`, loaded the DLL and installed all hooks. DXVK copied the multisampled depth through
`RESZ` into `INTZ`, passing the depth-copy self-test, so the default `Multisampling=1` worked on this device.

With `Quality=2` and the game's Multisampling set to 2x, the tested scene held 60 fps at 69% GPU utilisation,
914 MHz and 2.8 W. At 8x, median GPU costs were 6.47 ms per frame for fog and 2.81 ms for water; GPU
utilisation was 97% at 1,600 MHz and 8.3 W. These are effect GPU costs, excluding CPU work and the client's
own rendering, and observations of one scene rather than a frame-rate guarantee. Start with 2x on the Deck;
if a scene still dips, lower water quality, then fog quality. No defaults are changed by these notes.
Other Linux hardware and Proton/DXVK versions have not been verified by this test.

## Log

The log is `Logs\Luxvelum.log` in the client folder.
At `LogLevel=1` the log holds a frame summary on the first frame, whenever the viewport or far clip changes, and
every 60 s (`LogLevel=2` adds one every 600 frames). Its first line ends with the median GPU time of the fog
passes since the previous summary, for example
`frame 3600: viewport 0,0 1920x1080 ... maxdist 5000, fog gpu 2.31 ms (median of 3542 frames, 0 skipped)`.
The time covers the fog passes alone, god rays included, without the client's own rendering or any CPU work;
compare it with the frame time to see the fog's share of a GPU-bound frame. `skipped` counts frames whose timing
was lost, and `fog gpu no samples` means none finished in the interval. Without timestamp queries the log says
so once and the summaries omit the time; at `LogLevel=0` no queries are issued.
With Classic data the summary adds the resolved glow, three points of the grading curve and each noisy layer.
Its day line ends with the sun's or moon's sight line, for example `sight line hidden 1.00 behind 412 yd, halo shadow
1.00`: the hidden share, the farthest occluder and the share the fog uses after the horizon gate.

Every 60 s the water adds `water gpu 1.24 ms (median of 3500 frames, 0 skipped), classes lake+ocean, waves 256
(7 tiles)`: its GPU time without the client's own water draws. `water:` lines name each liquid type the first
time it is classified; idle states (no water in view, camera under water) are logged once and repeats only at
`LogLevel=2`. Each settled change from the settings window, an INI reload or Revert is logged as one line, for
example `settings: WaterFoam 1 -> 1.5, WaterWind 2 -> 4`.
The water line goes on with the ripples (`ripples 512 at 0.125 yd, 30 Hz, up to 3 contacts, 0 steps dropped`, or
`ripples idle`/`off`), and `water: the client's splash and wake sprites are hidden` or `shown again` reports the hold.

The depth probe logs raw depth, distance and fog opacity at 25 points on frame 60, then every 60 s up to five
times (every 30 s without limit at `LogLevel=2`); its rows are read back on a later frame. The first reason a
fog draw is skipped is logged as `fog skipped: <reason>`; a camera under water is logged once as `fog idle`.

Each device creation and `Reset` logs the adapter, the sample count requested and used, the depth format bound,
and the depth copy method with its self-test result or why multisampling is off. Guarded client bytes that differ
are logged with the feature they leave off.

## Settings

`WTF\Luxvelum.ini` is re-read within a second while the game runs (except `Enable`, `EngineHooks` and `LightStay`);
its comments describe every key. In the game, `Insert` (the shipped `OverlayKey`) opens the same settings in a window
(see In-game settings).

The Default column holds the built-in value that a key missing from the file takes. The shipped `Luxvelum.ini` differs
from it in `FarClipMax=1583` and, since it took a tester's settings (2026-11-16), in `OverlayKey=Insert`, `Quality=1`,
`Density=0.8`, `Haze=0.4`, `FarFog=0`, `NoiseAmount=0`, `NoiseScale=0.001`, `NoiseWindSpeed=0`, `ClassicNoise=1`,
`OutlandDensity=0.5`, `NorthrendDensity=0.7`, `AzerothSkyColor=0.5`, `OutlandSkyColor=1`, `NorthrendSkyColor=1`,
`TBCStartSkyColor=0.5`, `SunScatter=0.44`, `MoonScatter=0.5`, `ClassicExposure=0.6`, `LocalLightIntensity=1.4`,
`OutlandLocalLights=4`, `LocalLightPhase=0.4`, `GodRays=2.5`, `ColorGrading=0.5`, `WaterWaves=0.5`, `WaterWind=1`,
`WaterFoam=0.3`, `WaterReflections=0.5`, `WaterSpecular=1.5` and `WaterClarity=1.4`; since 2026-11-18, at the owner's
choice, also `ClassicPhase=1` and `Temporal=0.97`, which ends the sky dithering of `Quality=1` with `Temporal=0.85`.

| Key | Default | Meaning |
|---|---|---|
| `Enable` | 1 | Master switch (restart) |
| `EngineHooks` | 1 | Install the hooks; 0 leaves the client unmodified (restart) |
| `LightStay` | 1 | Keep point lights on while their model is not drawn, exe patch No. 70 in memory (restart, hooks) |
| `Overlay` | 1 | Settings window; 0 hides it at once, and from the next start leaves the game window alone unless `ToggleKey` is set |
| `OverlayKey` | Ctrl+F7 | Key that shows and hides the window, with optional `Ctrl+`, `Shift+`, `Alt+` |
| `ToggleKey` | none | Key that switches Luxvelum off and on while the game runs; empty = no key |
| `Quality` | 2 | 1 quarter resolution / 16 steps, 2 half / 24, 3 half / 32 |
| `Density`, `Haze`, `GroundFog`, `FarFog` | 1, 1, 0.6, 1 | Density multipliers; with Classic data `Haze`, `FarFog` and `GroundFog` relative to their defaults scale layers 0, 1 and 2 (haze, far wall, ground fog) |
| `NoiseAmount` | 0.15 | World-space density variation, 0..1; 0 disables it |
| `NoiseScale` | 0.025 | Inverse feature size of the variation |
| `NoiseWindSpeed` | 0.5 | Drift of the variation along world +X in yd/s |
| `ClassicNoise` | 0 | The modern client's authored noise on the Classic layers that carry it (mostly storms); 0 = off |
| `StockFog` | 1 | 1 replaces the stock fog with the distance fog, 0 keeps it |
| `TransparentFog` | 1 | 1 fogs see-through effects at their own distance, 0 at the scene behind (see Transparent fog) |
| `DataMode` | 1 | 1 Classic layers where available, 0 derived layers everywhere |
| `AzerothDensity`, `OutlandDensity`, `NorthrendDensity`, `TBCStartDensity` | 1, 0.15, 0.15, 1 | Density per region, on top of `Density` |
| `AzerothLocalLights`, `OutlandLocalLights`, `NorthrendLocalLights`, `TBCStartLocalLights` | 1, 8, 8, 2.5 | Lamp intensity and reach, 0..128 |
| `OutlandClassicFog`, `NorthrendClassicFog` | 1, 1 | 1 the converted client lights there, 0 derived layers |
| `AzerothSkyColor`, `OutlandSkyColor`, `NorthrendSkyColor`, `TBCStartSkyColor` | 0, 0, 0, 0 | Share of the client's fog hue in the layers |
| `ColorSpace` | 1 | 1 scatter and blend in linear light with a highlight roll-off, 0 gamma |
| `SunScatter`, `Ambient`, `Exposure` | 1, 1, 1 | Light in the fog |
| `MoonScatter` | 0.2 | Moon light in the fog at night, on its own (`SunScatter` is the sun's), Classic and derived |
| `ClassicExposure` | 1 | Brightness of the Classic layers (1 = as authored) |
| `ClassicPhase` | 0 | Classic sun and moon scattering: 0 phase peaks at 1 toward the light, 1 energy-normalised |
| `LocalLights`, `LocalLightIntensity` | 1, 1 | Scatter up to eight world point lights in view; intensity 0..8 |
| `LocalLightPhase` | 0.3 | Henyey–Greenstein g of every point light in the fog, −0.9..0.9 (a calibration) |
| `InteriorAware`, `InteriorDensity` | 1, 0.15 | Fade outdoor layers indoors, keeping this fraction of their density |
| `InteriorFarFog` | 1.0 | Native distance-fog density retained indoors (0..1); lower values clear dungeon haze |
| `GodRays` | 0 | Shafts of light through gaps in trees, buildings and terrain in front of the sun, 0 = off |
| `GlowCompensation` | 1 | Pre-compensate for the client's glow: the fog with `ColorSpace=1`, else only the god rays |
| `ForeverGlow` | 0 | 1 gives the client's glow Forever's per-light amount where Classic lights cover the camera |
| `ColorGrading` | 0 | Strength 0..1 of Forever's per-light colour curve over the 3D view; 0 = off |
| `FarClipMax` | 0 | Continent view distance up to 1583 yd, within `farclip`; 0 keeps Ascension's 791 cap |
| `MaxDistance` | 5000 | Fog range: sky integration length and the Classic distance-curve scale |
| `Temporal` | 0.85 | History weight, 0 = off |
| `Underwater` | 0 | Keep the effect under water |
| `LiquidDepth` | 1 | Water surfaces write depth so fog uses their distance (always while modern water is drawn) |
| `Multisampling` | 1 | Keep the game's Multisampling when its depth can be copied; 0 = multisampling off (see below) |
| `DebugView` | 0 | 1 radiance, 2 transmittance, 3 linear depth |
| `SunMarker` | 0 | Red dot where the light direction projects |
| `LogLevel` | 1 | 0 errors, 1 info (frame summary with the fog's GPU time, depth probe; see Log), 2 debug |
| `Water` | 1 | Modern water, 0 = the client's own water |
| `WaterQuality` | 2 | 1 128² waves, sky reflections only; 2 256², scene reflections; 3 256², finer reflections |
| `WaterWaves` | 1 | Wave height 0..2; 0 = flat water |
| `WaterWind` | 2 | Wind driving the waves, 0.5..10 |
| `WaterFoam`, `WaterReflections` | 1, 1 | Foam and reflection strength 0..2 |
| `WaterReflectionWaviness` | 0.35 | How far the wave slope bends the reflection rays 0..1; 0 = a mirror, 1 = the full wave normal. It leaves the waves alone, unlike `WaterWaves`; first labelled "Reflection distortion", renamed after the owner took it for a second wave height (2026-11-25), and not "ripple" because `WaterRipples` are the units' wakes |
| `WaterSpecular` | 1 | Sun or moon glint 0..4 |
| `WaterMoonGlint` | 6 | The moon's glint 0..10, on top of `WaterSpecular`: the night's direct light the water is lit by is dim (and scaled by 0.35), while the moon's disc is bright, so at 1 the moon's path on the lake was barely visible (owner, Mulgore, 2026-11-24) |
| `WaterClarity` | 1 | How far you see into the water 0.25..4 |
| `WaterZoneColors` | 0.5 | Tint by the zone's own water colours 0..1 |
| `WaterRipples` | 0.5 | Wakes of units in the water 0..2; 0 = no unit reads and no ripple simulation |
| `WaterClientSplashes` | 0 | 1 keeps the client's splash and wake sprites; 0 hides them for all units while wakes run |
| `WaterDebugView` | 0 | 1 wave normals, 2 foam, 3 transmittance, 4 reflection, 5 liquid class, 6 ripple height |

Turning `FarClipMax` on from 0 needs a restart; other changes, including 0, apply at the next `farclip` change,
map load or zone change, where raising it shows a loading screen.

Turning `Multisampling` on from 0 also needs a restart: the game builds its Video options' Multisampling list
once per start (see Engine inputs), and with 0 that list holds only 1x. Turning it off applies the next time the
game resets or recreates its display, for example after changing Multisampling or the resolution.

## Status and limits

This is an atmospheric approximation, not a reproduction of WoW Forever's complete lighting renderer.
[Blizzard's official overview](
https://news.blizzard.com/en-gb/article/24303862/world-of-warcraft-forever-whats-next-panel-recap)
describes mist over water and moonlight through trees; matching those scenes needs matched camera, time, weather
and exposure captures. Surface lighting and bloom remain the client's own; the modern client's glow amounts and
colour grading can be applied with `ForeverGlow` and `ColorGrading`. Neither has been compared with Forever in the
game: both are off when their keys are missing, and the shipped INI keeps `ForeverGlow` off and grades at half strength
(`ColorGrading=0.5`), so colours still differ from Classic.

- The fog has been tested in the client with native D3D9 on an RTX 2060 laptop. A first modern-water build ran
  there on Elwynn lakes and the Darkshore coast; the review fixes since (direct-light shading, narrower shore
  foam, fogged reflections, the dry-frame skip) are harness-checked only. Changes to lighting and occlusion require
  an owner test; the offline harness does not establish in-game appearance or performance. Other GPU vendors
  and integrated GPUs are untested except for the Proton/DXVK Steam Deck result above; other Wine
  configurations remain untested.
- The march, the composite, the water shading and the ripple step need more than the 512 instruction slots
  guaranteed by [baseline pixel shader 3.0](
  https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-ps-3-0), and lower
  quality does not reduce that (the water shading without its reflection march still takes 1,118 slots). An unsupported
  required fog shader disables volumetric fog for that device and logs its name, HRESULT and the shader caps; native
  fog remains the fallback. An unsupported water shading shader leaves the client's own water on that device and logs
  its name and HRESULT, and an unsupported ripple step logs its HRESULT and leaves the modern water without ripples.
- Scene geometry shadows the fog's sunlight only through the screen. A low-resolution pass (`ps_sun_shadow`, stage 11)
  walks six points along the first 800 yd of each view ray toward the light in 14 growing steps (about 1800 yd)
  against the depth buffer and counts a point as shadowed when a step crosses behind a surface; the march and the
  composites' full-resolution march scale the direct light of the steps in that range by the share that reached the
  light (`SunShadow`). Geometry off screen or behind the camera cannot shadow, the noisy composites'
  full-resolution march stays unshadowed and the lit composites' one does not fade the shadow out toward 800 yd
  (ps_3_0 temp limit). The probe is unrolled without early exits: on an AMD RX 6700 XT the same probe as a loop with
  `break`/`continue` never reported a crossing.
- Local lighting captures the current M2 scene's point-light table. It does not create spotlight cones or local
  shadows, and coverage of WMO-only lights is unverified. A light's sphere is capped at 200 yd and its attenuation
  denominator floored at 1; it is a candidate at any camera distance while its sphere meets the view and the client
  keeps it in its light table (see World lights), and a far light in view costs each march ray its chord test, with the
  scattering evaluated only where the ray crosses its sphere.
- The owner reported on 2026-10-15 that the flicker was gone in game with the stabiliser, the tolerant table walk and
  `LightStay` together; no `CoAVolFog.log` of that test has been read, which of the three ended it was not separated,
  the view-only selection without a camera-distance bound and the edge margin are harness-checked only, and the exe
  patch that `LightStay` mirrors is listed as untested by its patcher. With `LightStay=1` a source that moves while its
  model is not drawn (a carried lantern) keeps lighting its last drawn spot, in the fog and in the client's own model
  lighting, until it is drawn again; the client's own choice of four lights per model is unchanged. `LightStay` is the
  only setting that changes an instruction of the client's own code; the hooks retarget call sites and vtable slots to
  the DLL.
- Transparent materials without depth writes, such as windows, are fogged at the depth behind them, and so are
  see-through effects with `TransparentFog=0`. With `TransparentFog=1`, the default since one tester's game test (see
  Transparent fog), M2 effects above water, those the client draws unfogged included, take one line fitted to the fog
  of the first 100 yd, or of a longer range when the fog in those is thin, which cannot follow height fog and fully
  fogs effects past its end (500–900 yd in tests), so far glows vanish.
- Interior treatment follows the camera's transition weight, not rooms or portals along each ray, so views
  through doorways may differ.
- The `gxApi d3d9ex` path is not wrapped (fog and the settings window stay off there). The settings window also
  needs a fog device, so it is missing when INTZ depth is unsupported.
- Multisampling is harness-checked on the RTX 2060 (NVAPI) and tested in game on the Steam Deck (`RESZ` through DXVK,
  see above), on the owner's RTX 4060 through NVAPI with the system `d3d9.dll` and through `RESZ` behind DXVK 3.1.1 with
  `d3d9.customVendorId = 1002` in `dxvk.conf` or from `DxvkAmdVendor=1` (2026-11-23); alpha-tested foliage stays jagged.
- The silhouette split sees one copied depth sample per pixel, so sub-pixel geometry it misses takes the far fog.
- The distance fog (`FarFog`, maps without Classic data) has no modern counterpart: it stands in for the stock
  fog up to the 3.3.5 far clip, which is far shorter than the modern client's.
- `FarClipMax` raises memory use (about four times the loaded terrain in a 32-bit process); Ascension's reason
  for the continent cap is unknown. Without the key in the INI it stays off.
- Modern water: water seen from below keeps the client's look; rivers have no flow direction (the client has no
  flow maps), so Forever's river foam is omitted; the surface stays flat and only its shading moves; reflections are
  screen-space, so what is off screen falls back to the sky colours; interiors are detected by liquid type only.
- The noise column mapping, the noise volume and `ClassicPhase`'s normalisation are inferred; `ClassicPhase` stays 0:
  the tester's settings set 1, but the shipped INI keeps 0, as 1 burnt the harbour's sunset halo to white.
- `ForeverGlow` sets only the amount: the client's glow adds `g·blur²`, Forever's a linear term.
- Switching Luxvelum off and on (`ToggleKey`) is harness-checked only. Switched off, the game is drawn by the client
  but on Luxvelum's device setup, with the far clip of `FarClipMax` and `LightStay`, so it can differ from a start
  without Luxvelum where those matter (a game that asked for multisampling Luxvelum could not keep, for example).
- Ripples are harness-checked only; walkers (2.5 yd/s) push a mound rather than a V, and every unit is stamped round.
- The ripple step, texel, window, footprint, fades, splash and contact limits are choices, not recovered values.
- The sprite hold is global: units past the 32 nearest, the 24 yd window or the modern water get no wake at all.
- The client's footstep spray (`0x723A50`) is not hooked, and game objects such as boats make no ripples.
- The zone lights' edge fade distance is chosen here: their `TransitionType` is 0 in every row and the modern
  client's transition rule is not known.

## License

CoAVolFog is licensed under the GNU General Public License version 3 only (`GPL-3.0-only`); see [LICENSE](LICENSE).

As an additional permission under GPLv3 section 7, you may link or combine CoAVolFog, including modified versions,
with the World of Warcraft client, and distribute the resulting combination without providing the client's source
code. GPLv3 continues to apply to CoAVolFog.

Luxvelum is a modified version of CoAVolFog under the same licence and the same additional permission.
