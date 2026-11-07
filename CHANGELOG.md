# Changes

## Unreleased

- A plugin's meters are held until the host reads them: a level keeps its largest and a gain reduction its deepest
  since the last read, so a 10 ms burst between two reads is in the next one (include/omx_meter_ring.h, docs/meters.md).
- A level the face measures carries a clip word: the meter is flagged with the host's held and clip bits (8 and 9) and
  get_values() writes one more value after the channels, 1 when a run of samples over full scale ran since the last read. The generator writes the words `held` and `clip` into every meter row; `clip: false`
  in a declaration leaves the word out.
- tools/meter-ring-check.sh proves the held accumulator with plain readers outnumbering the cores beside one taker, and
  tools/meter-held-check.sh proves it through a built face; each has its sabotage arms.
