# Model preload

The first time an NPC or monster model is needed on a map, loading it (its high-resolution textures in particular) stalls the
game for 30 to 250 ms. To avoid that stutter while walking, the client remembers which models each map needed in
`ModelPreload.txt` (in the game folder, one line per model: map, `N` for an NPC or `M` for a monster, model id) and loads
them while the map loads.

- The first visit to a map still stutters once per new NPC; later visits are smooth.
- Entering a map takes a little longer (about a second in Devias) because its models load then.
- Deleting `ModelPreload.txt` is harmless: it is rebuilt as you play.
