#pragma once

// Tells the server which build this client is and a fingerprint of its program files (the executable and the DLLs next
// to it), so the server can record it. The report changes nothing in the game; it is a record, not a proof: a client
// which is changed can send anything. Used from the periodic game-server check.
namespace ClientIntegrity
{
// Called on every periodic check while the connection to the game server is up. Sends the report a few times per
// connection once the player is logged in.
void OnTick(bool loggedIn);

// Called when the connection to the game server is down, so the next connection reports again.
void OnDisconnected();
} // namespace ClientIntegrity
