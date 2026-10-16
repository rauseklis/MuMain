# Map assets

Map object files can contain malformed model indices. The loader skips negative
indices and indices outside the model table before creating an object, so these
records cannot reach rendering. Skipped entries are reported in the client log.

The guard also protects older Arena files containing model index -515.

## Full Arena

The shipped World7 and Object7 folders now contain the full replacement Arena,
including its terrain height, collision, layout and object placements. Ordinary
builds copy these files directly; a separate visual overlay is no longer needed.
Use the matching server Terrain7.att resource, and update an existing database's
Arena terrain with the parent repository's Arena installation script.

The minimap is deliberately unavailable in Arena. Tab and other interface-open
paths cannot display it, even if a minimap image is present. Other maps retain
their usual minimap behaviour.
