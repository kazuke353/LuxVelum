# Handover

Luxvelum changed maintainers on 17 November 2026: `Raz0r1337` transferred the repository to Stormhand
(`Stormhand-dev`), who tested it until then and now builds, tests in the game and releases himself. `AGENTS.md` keeps
the code conventions and `README.md` the design; this file holds the working state at the handover. Open work is
tracked in the GitHub issues.

## Branches and releases

- `reforged-dev` is the development branch; every push to it replaces the pre-release `build-<n+1>-<k>-test`.
  `reforged-main` is the release branch and the default branch; every push to it publishes `build-<n>` and deletes every
  other release. A commit message with `[skip ci]` runs nothing. `.github/workflows/release.yml` and the README section
  "Download a CI build" describe the numbering and the manual `refresh` run.
- `reforged-main` still carries the old `release.yml`, which triggers only on `st0ny-main`. Until dev's workflow is on
  main, a push to `reforged-main` starts no run and a manual run skips the release job. The release path under the
  `reforged-main` name has never run; watch the first one.
- Main is not an ancestor of dev (main was reset to `ea0d3d0` for Build-1 on 24 October 2026). Dev holds everything
  main has; check with `git diff reforged-dev origin/reforged-main` before releasing. To release dev, either set main to
  dev (`git push --force-with-lease origin reforged-dev:reforged-main`) or run `git merge -s ours origin/reforged-main`
  on dev once, push dev and fast-forward main (`git push origin reforged-dev:reforged-main`); later releases are then
  fast-forwards. The second way was used once before (merge `1c1f49a`, build-26).
- Do not merge upstream `jealous-sound/coa-vfog`: its commit `3908909` carries a session link removed from this history,
  and its re-indented `release.yml` breaks a partial merge. Port upstream changes one by one.

## State on 17 November 2026

- `reforged-main` `6656eee` = Luxvelum Build-1 (28 October 2026). `reforged-dev` `541ebe5` = TEST Build-2-3, 84
  commits ahead: Outland and Northrend fog from the client's light tables, god rays as shafts only, the `Logs`, `WTF`
  and `Data\Luxvelum` folders, the Ghostlands without sun scattering, sun and moon hidden behind the world, twin torch
  lights merged, the tester's INI as the shipped one, and the `ToggleKey` switch.
- The CI runs only the CPU checks. The GPU checks of `vfog_harness` have not run on a real GPU since late October; run
  the full `ctest` suite on a Windows machine with a graphics card before trusting them, and expect some failures to lie
  in the tests themselves.

## Test round of 16 and 17 November (TEST Build-2-2 and 2-3)

Confirmed in the game, with log evidence:

- The moon's halo goes behind terrain, trees and buildings (`sight line hidden 1.00`).
- The sun's glint on water goes behind a ridge, trees and buildings; with only part of the disc covered, the glint is
  fully off, because one ray to the disc centre decides.
- No sun scattering in the Ghostlands at noon; the light weights blend over about 100 yd into Eversong Woods.
- Twin torches merge at Light's Hope Chapel (`2 lights merged`, one cluster of 6); 17 to 18 sources stay in view for
  eight slots.
- Local Lights cost about 1.7 ms of fog GPU time at Light's Hope and about 10 FPS along Elwynn's lamp posts. The
  Mulgore FPS drop of 31 October did not reproduce.

Not yet tested: the `ToggleKey` switch and `LightStay=0`.

## Open points

- The GitHub issues hold the findings of that round: map 530 zones read as Outland, the Ghostlands also losing the
  moon's scattering, the shipped INI dithering the sky, log spam at zone light borders, scatter per region, lit squares
  on the terrain at Light's Hope, and a possible cap on the light reach.
- The lit squares on the terrain are not the fog. Luxvelum only draws the lights' glow in the fog; the light on floors
  and walls comes from the client and, where it is installed, from `Raz0r1337/WoW-lights-patch-shader`, which lights
  surfaces with eight lights of its own. Name which one a light report means.
- The mist that seems to follow the character in Tirisfal Glades: the Tirisfal layers carry flag `0x2` (heights relative
  to `ReferenceZ`, the camera minus 1 yd), so the ground fog keeps its height to the camera.
- A dark, unfogged stone in Ashenvale (Maestra's Post): suspected blend mode 7, which the unfogged batch hook turns
  black; workaround `TransparentFog=0`; first get the player's log and INI; proposed fix after data: fog mode 7 like
  mode 2.
- Leftovers: the upstream workflow link and `main` under "Download a CI build"; the sentence "Build-1 and TEST Build-2-1
  to 2-3 shipped none" under "Install", which means the builds before 28 October; "coa-vfog" in `CMakeLists.txt`; the
  upstream owner in `.github/CODEOWNERS`.

## Decisions to keep

- At most eight local lights; more coverage comes from selection (hysteresis, fades, twin merging), never from a
  higher limit.
- Light emitters belong in the Project Reforged models and Luxvelum finds them at run time; there is no lamp catalogue.
  `git revert 1ca476e` would bring the catalogue back; `CatalogLampId` and the twin rule "same kind only" then need to
  be restored by hand.
- Lumenveil (GPLv2-only) is a source of ideas, never of code.
- `Luxvelum.dll` keeps the `vf_loader_anchor` export, or the client does not start with the `version.dll`.
- The old world keeps its Classic fog data; only Outland, Northrend and maps Classic never had use the client's lights.

## Earlier decisions, open for the new maintainer

- Antialiasing stays as in the original.
- Optimisations must leave the image unchanged; an image-identical package was stopped because the gain was too small.
- `ClassicPhase=1` ships since 18 November at the owner's choice (0 before); the README reports a white sunset halo
  at the harbour with 1, which `CheckSunsetHaloKeepsItsHue` checks on a GPU with the shipped INI.
