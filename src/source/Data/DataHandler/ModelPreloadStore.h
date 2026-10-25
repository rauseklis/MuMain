#pragma once

#include "Data/DataHandler/ModelPreloadList.h"

namespace Data::Models
{
// Writes down that the map needed the model, the first time it is loaded there. A model already on the list is ignored.
void RememberModelForMap(int map, PreloadEntry entry);

// Loads the models the map needed before, so no NPC or monster loads while the player walks around.
void PreloadModelsForMap(int map);
} // namespace Data::Models
