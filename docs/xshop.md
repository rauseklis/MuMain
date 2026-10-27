# xShop (in-game cash shop)

Press **X** in a safe zone to open the shop. The window, its texts and its item lists are the original Webzen Season 6 client; the server
(`rauseklis/OpenMU`, `src/GameServer/MessageHandler/CashShop`) answers it.

## How it fits together

- **Script folder:** the client reads the shop lists from `Data/InGameShopScript/<zone>.<year>.<id>/` (`IBSCategory.txt`, `IBSPackage.txt`,
  `IBSProduct.txt`). The server announces the version (`D2 0C`, now `512.2012.085`) when a character enters the game; without it the client refuses to open the
  shop. The original Webzen script `512.2012.084` stays in the repository, it is not used.
- **`512.2012.085`** is generated, not edited by hand: `python scripts/generate-xshop-script.py <original script folder> <cashshop-items.csv> <output folder>`.
  It keeps only the packages whose item the server can deliver (a package with more than one product, for example the "+GP" seal bundles, is left out).
  A copy of the three files is embedded in the server (`Catalog/`); client and server copies must be identical, and the server has a test that every
  shipped package is purchasable.
- **Which item a package gives** is decided only by the server's `cashshop-items.csv` (shop item name, item group, number, level). The
  client's item codes and prices are never trusted: the server takes price, currency and item from the script.
- **Currencies:** WCoin (C), WCoin (P) and Goblin Point, shown on the three tabs. A balance belongs to the account on one game server.
  A game master gives currency to an online player with `/cash <character> <wcc> <wcp> <gp>`.
- **Buying** puts the item into the shop storage (the "Storage" tab of the window); **using** it there creates the real item in the inventory.

## Adding or changing an item

1. Add a line to the server's `cashshop-items.csv` (name as in the Webzen script, item group, number, level). The item must exist in the item data.
2. Re-run the generator and copy `IBSCategory.txt`, `IBSPackage.txt` and `IBSProduct.txt` into the new script folder in the client and into the
   server's `Catalog/` folder. If the script changes, use a new folder name and update `CashShopCatalog.ScriptYearIdentifier` in the server, so
   clients with the old files load the new ones.
3. Merge the server first, then the client.
