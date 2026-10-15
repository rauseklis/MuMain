# Map assets

Map object files can contain malformed model indices. The loader skips negative
indices and indices outside the model table before creating an object, so these
records cannot reach rendering. Skipped entries are reported in the client log.

This includes model index -515 in the original Arena object file. The file and
its valid placements are preserved.
