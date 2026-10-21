# Packet packages

The client takes its network protocol from three NuGet packages built from the **server** repository (`rauseklis/OpenMU`):
`MUnique.OpenMU.PlugIns`, `MUnique.OpenMU.Network` and `MUnique.OpenMU.Network.Packets`. They are kept in this folder, and
`nuget.config` makes it a package source, so no package server or token is needed. `OpenMuPacketsVersion` in
`ClientLibrary/MUnique.Client.Library.csproj` names the version in use: **0.9.20** (CMake reads it from there too).

## When the server's packets change

1. In the server repository raise `<Version>` and `<PackageVersion>` of the three projects (`src/PlugIns`, `src/Network`,
   `src/Network/Packets`) to the next version, e.g. 0.9.21, and merge that.
2. From the server repository, build and pack each project into this folder:
   ```
   dotnet build src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release -t:Rebuild
   dotnet pack  src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release --no-build -o <client repo>/nuget-feed
   ```
   (the same two commands for `src/Network/MUnique.OpenMU.Network.csproj` and `src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj`).
3. Set `OpenMuPacketsVersion` to the new version, commit the three new `.nupkg` files with it, and delete the old ones.
4. Build once so the C++ wrappers under `src/source/Dotnet/` regenerate (`dotnet build ClientLibrary/MUnique.Client.Library.csproj -c Release -p:Platform=x86`);
   commit the regenerated files.

The server and the client must be merged together: a client with other packets than the server cannot talk to it.
