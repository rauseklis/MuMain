#pragma once

// A notice that tells the player who killed their character. A player who was away (AFK) when they were killed
// comes back at the safe zone and would otherwise miss the one-line system message long before. The notice stays
// on screen until the player moves again, then it stays 30 more seconds and fades.
namespace UI::KillNotice
{
    // Called when the hero's character was killed; killerKey is the key of the character which dealt the last hit.
    // Only a killer which is another player produces a notice (not monsters, not duels, not the hero itself).
    void OnHeroDied(int killerKey);

    // Draws the notice (if there is one) and advances its timing. Called once per frame from the overlay.
    void Render();
}
