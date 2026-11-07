# Changes

## Unreleased

- A plugin's meters are held until the host reads them: a level keeps its largest and a gain reduction its deepest
  since the last read, so a 10 ms burst between two reads is in the next one (include/omx_meter_ring.h, docs/meters.md).
- A level the face measures carries a clip word, the longest run of samples over full scale since the last read, served
  by org.openmixer.meter-clip/1. The generator writes the words `held` and `clip` into every meter row; `clip: false`
  in a declaration leaves the word out.
- tools/meter-ring-check.sh proves the held accumulator with plain readers outnumbering the cores beside one taker, and
  tools/meter-held-check.sh proves it through a built face; each has its sabotage arms.
