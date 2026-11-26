## What's new in v1.1.0

- **Clearer water reflections.** Reflections no longer break into noisy specks on wavy water. The waves now bend the
  reflections less than they bend the surface, so piers, lamps and cliffs reflect as steady streaks. The new
  *Reflection waviness* setting (`WaterReflectionWaviness`, 0.35) sets how much: 0 is a mirror, 1 is the old look.
  It leaves the waves themselves alone; *Waves* still sets those.
- **No more cyan patches in the water.** Under piers and behind posts, where the screen holds nothing to reflect,
  the water used to show a sky colour. It now reflects the wood and stone around it, and reflections fade softly at
  the screen's edges instead of switching on and off as the camera moves.
- **No ghosting at the screen's sides.** Objects at the left and right edges of the screen no longer leave a ghost in
  the water while the camera turns.
- **Fog density per zone.** Next to the sun and moon halos, *Fog density here* in the settings window scales the fog
  of the zone you stand in. 42 zones ship tuned in `Data\Luxvelum\zones.ini`, among them a lighter haze in the
  Barrens and Durotar and a thicker one in Elwynn Forest. Your own values go to `WTF\Luxvelum-zones.ini`.
- **Moon highlight on the water.** *Moon highlight* (`WaterMoonGlint`, 6) sets the moon's reflection on water on its
  own, so lakes catch moonlight at night without changing the sun's highlight by day.
- **Settings.** The shipped `Luxvelum.ini` carries the new options. Copy your own changes into it if you keep a
  backup of the previous one.

All of the above was tested in game on the owner's client (RTX 4060, DXVK 3.1.1, Project Reforged patches).
