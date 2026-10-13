# Auction House sub-categories and category rail Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Browse can filter by a sub-category (gloves, maces, potions, rings, scrolls and so on) through a redesigned category rail with a flyout, backed by a server taxonomy, a new packet field and a stored per-listing sub-category.

**Architecture:** One server table maps `(item group, item number)` to `(category, sub-category)`. The listing stores both, the browse packet carries a new `SubCategory` byte, and the browse query filters on it. The client mirrors only the ids and labels in a pure header, draws a nine-row rail with item icons and a click-open flyout, and sends the leaf with each browse request.

**Tech Stack:** C# / .NET 10, EF Core + PostgreSQL, NUnit + Moq (server); C++20, OpenGL NewUI, doctest, CMake + Ninja (client); the packet XML to NuGet to generated client bindings pipeline.

**Spec:** `docs/superpowers/specs/2026-10-13-auction-categories-design.md` (client worktree). Read it before starting; the taxonomy table there is the source of every number list below.

## Repositories and paths

- Server worktree: `C:\Users\rause\OneDrive\Desktop\mu-online-openmu\server\.worktrees\auction-house` (branch `codex/auction-house`). Called `SERVER` below; all server paths are relative to it.
- Client worktree: `C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\auction-house` (branch `codex/auction-house`). Called `CLIENT` below.
- Item.txt (read-only input for a fixture): `C:\Users\rause\OneDrive\Desktop\Emberion\public_html\application\data\ServerData\en\Item.txt`.
- Local NuGet feed: `C:\Users\rause\.local-nuget\auction-house`.

## Global Constraints

- Use only the `openmu_auction_test` database through the running server; never touch `openmu`. Never create another database without asking the owner first.
- Commit locally on the `codex/auction-house` branches only. Never push, merge, or commit in the monorepo root `C:\Users\rause\OneDrive\Desktop\mu-online-openmu` itself. Do not touch `server/src/Web/Map/wwwroot/js/MUnique.OpenMU.Web.Map.js`.
- Commit messages end with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
- Wire values 0 to 6 of `AuctionCategory` never change; Scrolls & Orbs is `7`. `255` means "all" for both category and sub-category.
- The browse packet becomes length 105: `SubCategory` (Byte) at index 104, appended after `Name`.
- The packet package version becomes `0.9.12`; the client `OpenMuPacketsVersion` becomes `0.9.12`.
- Stop `Main.exe` before building the client; stop the server process before building the server.
- The user logs in and takes their own screenshots; do not drive their screen. Relaunch the client and server when done.
- Client build: PowerShell, vcvarsall x86, then `ninja -f build-Debug.ninja Main` in `out/build/windows-x86`. Tests: `ninja ... auction_model_tests` then `tests\ui\Debug\auction_model_tests.exe`. Server tests: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionHouse"`.
- Python edits of repo files: read and write with `newline=''` and keep each file's existing line endings (repo files are CRLF); the Write tool is fine for new files.
- Bash heredocs containing certain characters have failed in this environment; prefer the Write tool for script and test files.

## Review Focus

Failure modes the spec implies that a person using this would hit, each pinned by a test named in the task that owns it:

1. A sub-category sent with "all categories" (a stale leaf after pressing Reset) must be ignored, not filter to nothing. Task 1 `SubCategoryFilterIsIgnoredWithoutACategory`.
2. A sub-category id the category does not have (client and server out of step) must return an empty page, not an error. Task 3 `UnknownSubCategoryReturnsNoRows` (PostgreSQL).
3. The re-classification step must be safe to run repeatedly and in batches: a batch smaller than the backlog makes progress each run, and a second run after completion changes nothing. Task 4 `ReclassifyRunsInBatchesUntilNothingIsLeft` (PostgreSQL) and `ReclassifyDelegatesToTheRepository`.
4. A flyout opened from a row near the bottom of the rail must stay inside the table area, and a row with the longest list must not run off it. Task 8 `flyout stays inside the limits`.
5. Picking a leaf and then a different category must not keep the old leaf (leaf ids repeat across categories, so a stale 3 would read as Pendants under Pet & Helper). Task 8 `the rail row shows the picked leaf of its own category only`, and Task 9's `SelectCategory`, which always replaces the leaf together with the category.

---

## Task 1: Server taxonomy and category enum

**Files:**
- Create: `SERVER/src/Persistence/AuctionHouse/AuctionTaxonomy.cs`
- Modify: `SERVER/src/Persistence/AuctionHouse/AuctionCategory.cs`
- Modify: `SERVER/src/Persistence/AuctionHouse/AuctionCategoryClassifier.cs`
- Modify: `SERVER/tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionNoteAndCategoryTests.cs`
- Create: `SERVER/tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionTaxonomyTests.cs`
- Create: `SERVER/tests/MUnique.OpenMU.Tests/AuctionHouse/Fixtures/ItemCatalog.tsv` (generated)
- Modify: `SERVER/tests/MUnique.OpenMU.Tests/MUnique.OpenMU.Tests.csproj`

**Interfaces:**
- Produces: `AuctionClassification(AuctionCategory Category, byte SubCategory)`; `AuctionTaxonomy.Classify(int group, int number) -> AuctionClassification`; `AuctionTaxonomy.AllCategories`, `AuctionTaxonomy.AllSubCategories` (both `byte` 255); `AuctionTaxonomy.ToSubCategoryFilter(byte category, byte subCategory) -> byte?`; `AuctionCategory.ScrollOrb = 7`. `AuctionCategoryClassifier.Classify(group, number)` keeps its signature and returns `AuctionTaxonomy.Classify(group, number).Category`.
- Consumed by: Tasks 3, 4, 5.

- [ ] **Step 1: Generate the Item.txt fixture**

Run from `SERVER` (Write the script to a scratch file first if the shell mangles it):

```python
import re
src = r"C:\Users\rause\OneDrive\Desktop\Emberion\public_html\application\data\ServerData\en\Item.txt"
out = r"tests\MUnique.OpenMU.Tests\AuctionHouse\Fixtures\ItemCatalog.tsv"
group = None
rows = []
for line in open(src, encoding="utf-8", errors="replace"):
    t = line.strip()
    if re.fullmatch(r"\d+", t):
        group = int(t)
        continue
    if t.lower() == "end":
        group = None
        continue
    if group is None or not t or t.startswith("//"):
        continue
    m = re.match(r'(\d+)\s+(\S+)\s+.*?"([^"]*)"', t)
    if m:
        rows.append((group, int(m.group(1)), m.group(3)))
import os
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w", encoding="utf-8", newline="\n") as f:
    for g, n, name in rows:
        f.write(f"{g}\t{n}\t{name}\n")
print(len(rows), "rows")
```

Expected output: `1839 rows`.

- [ ] **Step 2: Copy the fixture to the test output**

In `MUnique.OpenMU.Tests.csproj`, add a new item group before the last `</Project>`:

```xml
  <ItemGroup>
    <Content Include="AuctionHouse\Fixtures\ItemCatalog.tsv">
      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
    </Content>
  </ItemGroup>
```

- [ ] **Step 3: Write the failing tests**

Create `AuctionTaxonomyTests.cs`:

```csharp
// <copyright file="AuctionTaxonomyTests.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.OpenMU.Tests.AuctionHouse;

using System.Text.RegularExpressions;
using MUnique.OpenMU.GameLogic.AuctionHouse;

/// <summary>
/// Verifies the browse taxonomy: the category and sub-category of an item by group and number, the sub-category filter
/// rules, and a name-based cross-check of every entry of the owner's Item.txt.
/// </summary>
[TestFixture]
public class AuctionTaxonomyTests
{
    /// <summary>
    /// Verifies known items classify as the spec's taxonomy table says.
    /// </summary>
    /// <param name="group">The item group.</param>
    /// <param name="number">The item number.</param>
    /// <param name="category">The expected category.</param>
    /// <param name="subCategory">The expected sub-category.</param>
    [TestCase(0, 16, AuctionCategory.Weapon, 0)]
    [TestCase(1, 0, AuctionCategory.Weapon, 1)]
    [TestCase(2, 5, AuctionCategory.Weapon, 2)]
    [TestCase(3, 0, AuctionCategory.Weapon, 3)]
    [TestCase(4, 6, AuctionCategory.Weapon, 4)]
    [TestCase(4, 7, AuctionCategory.Weapon, 4)]
    [TestCase(5, 10, AuctionCategory.Weapon, 5)]
    [TestCase(7, 8, AuctionCategory.Armor, 0)]
    [TestCase(8, 8, AuctionCategory.Armor, 1)]
    [TestCase(9, 8, AuctionCategory.Armor, 2)]
    [TestCase(10, 8, AuctionCategory.Armor, 3)]
    [TestCase(11, 8, AuctionCategory.Armor, 4)]
    [TestCase(6, 8, AuctionCategory.Armor, 5)]
    [TestCase(12, 5, AuctionCategory.Wing, 0)]
    [TestCase(12, 150, AuctionCategory.Wing, 0)]
    [TestCase(12, 40, AuctionCategory.Wing, 1)]
    [TestCase(12, 242, AuctionCategory.Wing, 1)]
    [TestCase(13, 30, AuctionCategory.Wing, 1)]
    [TestCase(13, 4, AuctionCategory.PetHelper, 0)]
    [TestCase(13, 5, AuctionCategory.PetHelper, 0)]
    [TestCase(13, 3, AuctionCategory.PetHelper, 1)]
    [TestCase(13, 37, AuctionCategory.PetHelper, 1)]
    [TestCase(13, 400, AuctionCategory.PetHelper, 1)]
    [TestCase(13, 8, AuctionCategory.PetHelper, 2)]
    [TestCase(13, 12, AuctionCategory.PetHelper, 3)]
    [TestCase(13, 200, AuctionCategory.PetHelper, 4)]
    [TestCase(13, 46, AuctionCategory.PetHelper, 5)]
    [TestCase(14, 13, AuctionCategory.JewelMaterial, 0)]
    [TestCase(12, 15, AuctionCategory.JewelMaterial, 0)]
    [TestCase(14, 160, AuctionCategory.JewelMaterial, 0)]
    [TestCase(14, 250, AuctionCategory.JewelMaterial, 0)]
    [TestCase(12, 30, AuctionCategory.JewelMaterial, 1)]
    [TestCase(12, 141, AuctionCategory.JewelMaterial, 1)]
    [TestCase(14, 41, AuctionCategory.JewelMaterial, 2)]
    [TestCase(14, 44, AuctionCategory.JewelMaterial, 2)]
    [TestCase(12, 60, AuctionCategory.JewelMaterial, 3)]
    [TestCase(12, 129, AuctionCategory.JewelMaterial, 3)]
    [TestCase(13, 15, AuctionCategory.JewelMaterial, 4)]
    [TestCase(13, 57, AuctionCategory.JewelMaterial, 4)]
    [TestCase(14, 3, AuctionCategory.Consumable, 0)]
    [TestCase(14, 133, AuctionCategory.Consumable, 0)]
    [TestCase(14, 72, AuctionCategory.Consumable, 1)]
    [TestCase(14, 140, AuctionCategory.Consumable, 1)]
    [TestCase(14, 11, AuctionCategory.Consumable, 2)]
    [TestCase(14, 300, AuctionCategory.Consumable, 2)]
    [TestCase(12, 7, AuctionCategory.ScrollOrb, 0)]
    [TestCase(12, 44, AuctionCategory.ScrollOrb, 0)]
    [TestCase(12, 21, AuctionCategory.ScrollOrb, 1)]
    [TestCase(12, 48, AuctionCategory.ScrollOrb, 1)]
    [TestCase(15, 0, AuctionCategory.ScrollOrb, 1)]
    [TestCase(15, 36, AuctionCategory.ScrollOrb, 1)]
    [TestCase(12, 26, AuctionCategory.Miscellaneous, 0)]
    [TestCase(16, 0, AuctionCategory.Miscellaneous, 0)]
    [TestCase(20, 3, AuctionCategory.Miscellaneous, 0)]
    public void ClassifiesKnownItems(int group, int number, AuctionCategory category, int subCategory)
    {
        var result = AuctionTaxonomy.Classify(group, number);

        Assert.Multiple(() =>
        {
            Assert.That(result.Category, Is.EqualTo(category));
            Assert.That((int)result.SubCategory, Is.EqualTo(subCategory));
        });
    }

    /// <summary>
    /// Verifies a sub-category is only a filter when a specific category and a specific sub-category are both named.
    /// </summary>
    /// <param name="category">The requested category, 255 for all.</param>
    /// <param name="subCategory">The requested sub-category, 255 for all.</param>
    /// <param name="expected">The expected filter, or <c>null</c> for no filter.</param>
    [TestCase((byte)255, (byte)3, null)]
    [TestCase((byte)1, (byte)255, null)]
    [TestCase((byte)255, (byte)255, null)]
    [TestCase((byte)1, (byte)3, 3)]
    [TestCase((byte)0, (byte)0, 0)]
    public void SubCategoryFilterIsIgnoredWithoutACategory(byte category, byte subCategory, int? expected)
    {
        var filter = AuctionTaxonomy.ToSubCategoryFilter(category, subCategory);

        Assert.That(filter, Is.EqualTo(expected is null ? null : (byte?)expected));
    }

    /// <summary>
    /// Cross-checks the rules against the names in the owner's Item.txt: an item named like a ring is a ring, an orb is an
    /// orb, a cape is a cape, and so on. A failure lists each item the rules put in the wrong place.
    /// </summary>
    [Test]
    public void EveryCatalogEntryLandsWhereItsNameSays()
    {
        var catalog = LoadCatalog();
        Assert.That(catalog, Has.Count.GreaterThan(1500));
        var violations = new List<string>();

        void Expect(string label, Func<int, string, bool> applies, Func<int, int, string, (AuctionCategory, int)> expected)
        {
            foreach (var (group, number, name) in catalog)
            {
                if (!applies(group, name))
                {
                    continue;
                }

                var actual = AuctionTaxonomy.Classify(group, number);
                var wanted = expected(group, number, name);
                if (actual.Category != wanted.Item1 || actual.SubCategory != wanted.Item2)
                {
                    violations.Add($"{label}: ({group},{number}) '{name}' is {actual.Category}/{actual.SubCategory}, expected {wanted.Item1}/{wanted.Item2}");
                }
            }
        }

        Expect("orb or scroll", (g, n) => (g == 12 || g == 15) && Regex.IsMatch(n, "Orb|Scroll|Parchment") && !Regex.IsMatch(n, "Seed|Sphere"), (g, _, n) => (AuctionCategory.ScrollOrb, n.Contains("Orb") ? 0 : 1));
        Expect("crystal", (g, n) => g == 12 && n.StartsWith("Crystal of", StringComparison.Ordinal), (_, _, _) => (AuctionCategory.ScrollOrb, 0));
        Expect("cape or cloak", (g, n) => g == 12 && Regex.IsMatch(n, "Cape|Cloak"), (_, _, _) => (AuctionCategory.Wing, 1));
        Expect("wings", (g, n) => g == 12 && Regex.IsMatch(n, "Wing|Wind of") && !Regex.IsMatch(n, "Cape|Cloak"), (_, _, _) => (AuctionCategory.Wing, 0));
        Expect("ring", (g, n) => g == 13 && Regex.IsMatch(n, @"Ring\b"), (_, _, _) => (AuctionCategory.PetHelper, 2));
        Expect("pendant", (g, n) => g == 13 && Regex.IsMatch(n, "Pendant|Necklace"), (_, _, _) => (AuctionCategory.PetHelper, 3));
        Expect("egg", (g, n) => g == 13 && n.StartsWith("Egg ", StringComparison.Ordinal), (_, _, _) => (AuctionCategory.PetHelper, 4));
        Expect("bundle", (g, n) => g == 12 && n.StartsWith("Bundled", StringComparison.Ordinal), (_, _, _) => (AuctionCategory.JewelMaterial, 1));
        Expect("seed or sphere", (g, n) => g == 12 && Regex.IsMatch(n, "^(Seed|Sphere)"), (_, _, _) => (AuctionCategory.JewelMaterial, 3));
        Expect("jewel", (g, n) => (g == 12 || g == 14) && Regex.IsMatch(n, "^Jewel [Oo]f"), (_, _, _) => (AuctionCategory.JewelMaterial, 0));
        Expect("potion", (g, n) => g == 14 && n.Contains("Potion"), (_, _, _) => (AuctionCategory.Consumable, 0));
        Expect("elixir", (g, n) => g == 14 && n.StartsWith("Elixir of", StringComparison.Ordinal), (_, _, _) => (AuctionCategory.Consumable, 1));

        Assert.That(violations, Is.Empty, string.Join(Environment.NewLine, violations));
    }

    private static List<(int Group, int Number, string Name)> LoadCatalog()
    {
        var path = Path.Combine(TestContext.CurrentContext.TestDirectory, "AuctionHouse", "Fixtures", "ItemCatalog.tsv");
        return File.ReadAllLines(path)
            .Where(line => line.Length > 0)
            .Select(line => line.Split('\t'))
            .Select(parts => (int.Parse(parts[0]), int.Parse(parts[1]), parts[2]))
            .ToList();
    }
}
```

In `AuctionNoteAndCategoryTests.cs` change the group-15 default-range case:

```csharp
    [TestCase(15, 0, AuctionCategory.ScrollOrb)]
```

(replacing `[TestCase(15, 0, AuctionCategory.Miscellaneous)]`).

- [ ] **Step 4: Run the tests to verify they fail**

Run from `SERVER`: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionTaxonomyTests|FullyQualifiedName~AuctionNoteAndCategoryTests"`

Expected: build error `The name 'AuctionTaxonomy' does not exist` and `'AuctionCategory' does not contain a definition for 'ScrollOrb'`.

- [ ] **Step 5: Add the enum value**

In `AuctionCategory.cs` add after `Miscellaneous = 6,`:

```csharp

    /// <summary>
    /// Skill orbs, crystals, scrolls and parchments (group 15, and the orb and scroll entries of group 12).
    /// </summary>
    ScrollOrb = 7,
```

- [ ] **Step 6: Write the taxonomy**

Create `AuctionTaxonomy.cs`:

```csharp
// <copyright file="AuctionTaxonomy.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.OpenMU.GameLogic.AuctionHouse;

/// <summary>
/// The browse category and sub-category of an item.
/// </summary>
/// <param name="Category">The category.</param>
/// <param name="SubCategory">The sub-category within the category; the constants in <see cref="AuctionTaxonomy"/> name the values.</param>
public readonly record struct AuctionClassification(AuctionCategory Category, byte SubCategory);

/// <summary>
/// Maps an item (group and number) to its browse category and sub-category. Exact numbers and ranges are checked first,
/// in the order they are listed, then the group's default. A number no rule names falls to its group's default, so a stock
/// item set still classifies sensibly. Sub-category values are per category.
/// </summary>
public static class AuctionTaxonomy
{
    /// <summary>
    /// The wire value which means every category.
    /// </summary>
    public const byte AllCategories = byte.MaxValue;

    /// <summary>
    /// The wire value which means every sub-category of the category. A listing stores it until it has been classified.
    /// </summary>
    public const byte AllSubCategories = byte.MaxValue;

    private static readonly Rule[] Rules = BuildRules();

    /// <summary>
    /// Sub-categories of <see cref="AuctionCategory.Weapon"/>; they equal the item group.
    /// </summary>
    public static class WeaponKinds
    {
        /// <summary>Swords, group 0.</summary>
        public const byte Swords = 0;

        /// <summary>Axes, group 1.</summary>
        public const byte Axes = 1;

        /// <summary>Maces and scepters, group 2.</summary>
        public const byte MacesAndScepters = 2;

        /// <summary>Spears, group 3.</summary>
        public const byte Spears = 3;

        /// <summary>Bows, crossbows and their ammunition, group 4.</summary>
        public const byte BowsAndCrossbows = 4;

        /// <summary>Staves and books, group 5.</summary>
        public const byte StavesAndBooks = 5;
    }

    /// <summary>
    /// Sub-categories of <see cref="AuctionCategory.Armor"/>.
    /// </summary>
    public static class ArmorKinds
    {
        /// <summary>Helms, group 7.</summary>
        public const byte Helms = 0;

        /// <summary>Armors, group 8.</summary>
        public const byte Armors = 1;

        /// <summary>Pants, group 9.</summary>
        public const byte Pants = 2;

        /// <summary>Gloves, group 10.</summary>
        public const byte Gloves = 3;

        /// <summary>Boots, group 11.</summary>
        public const byte Boots = 4;

        /// <summary>Shields, group 6.</summary>
        public const byte Shields = 5;
    }

    /// <summary>
    /// Sub-categories of <see cref="AuctionCategory.Wing"/>.
    /// </summary>
    public static class WingKinds
    {
        /// <summary>Wings.</summary>
        public const byte Wings = 0;

        /// <summary>Capes and cloaks.</summary>
        public const byte CapesAndCloaks = 1;
    }

    /// <summary>
    /// Sub-categories of <see cref="AuctionCategory.PetHelper"/>.
    /// </summary>
    public static class PetKinds
    {
        /// <summary>Pets and helpers.</summary>
        public const byte Pets = 0;

        /// <summary>Mounts.</summary>
        public const byte Mounts = 1;

        /// <summary>Rings.</summary>
        public const byte Rings = 2;

        /// <summary>Pendants and necklaces.</summary>
        public const byte Pendants = 3;

        /// <summary>Eggs.</summary>
        public const byte Eggs = 4;

        /// <summary>Seals, tickets and everything else of group 13.</summary>
        public const byte SealsAndTickets = 5;
    }

    /// <summary>
    /// Sub-categories of <see cref="AuctionCategory.JewelMaterial"/>.
    /// </summary>
    public static class MaterialKinds
    {
        /// <summary>Jewels.</summary>
        public const byte Jewels = 0;

        /// <summary>Bundled jewels and stones.</summary>
        public const byte Bundles = 1;

        /// <summary>Refining stones and gemstones.</summary>
        public const byte RefiningStones = 2;

        /// <summary>Seeds, spheres and seed spheres.</summary>
        public const byte SeedsAndSpheres = 3;

        /// <summary>Stat fruits and reset fruits.</summary>
        public const byte Fruits = 4;
    }

    /// <summary>
    /// Sub-categories of <see cref="AuctionCategory.Consumable"/>.
    /// </summary>
    public static class ConsumableKinds
    {
        /// <summary>Potions.</summary>
        public const byte Potions = 0;

        /// <summary>Elixirs and buff scrolls.</summary>
        public const byte ElixirsAndBuffs = 1;

        /// <summary>Boxes, tickets and everything else of group 14.</summary>
        public const byte BoxesAndMisc = 2;
    }

    /// <summary>
    /// Sub-categories of <see cref="AuctionCategory.ScrollOrb"/>.
    /// </summary>
    public static class ScrollKinds
    {
        /// <summary>Skill orbs and crystals.</summary>
        public const byte OrbsAndCrystals = 0;

        /// <summary>Skill scrolls and parchments.</summary>
        public const byte ScrollsAndParchments = 1;
    }

    /// <summary>
    /// Classifies an item.
    /// </summary>
    /// <param name="group">The item group.</param>
    /// <param name="number">The item number.</param>
    /// <returns>The category and sub-category.</returns>
    public static AuctionClassification Classify(int group, int number)
    {
        foreach (var rule in Rules)
        {
            if (rule.Group == group && number >= rule.First && number <= rule.Last)
            {
                return new AuctionClassification(rule.Category, rule.SubCategory);
            }
        }

        return new AuctionClassification(AuctionCategory.Miscellaneous, 0);
    }

    /// <summary>
    /// Gets the sub-category filter of a browse request. A sub-category only filters when a specific category and a specific
    /// sub-category are both named; otherwise it is ignored.
    /// </summary>
    /// <param name="category">The requested category, <see cref="AllCategories"/> for every category.</param>
    /// <param name="subCategory">The requested sub-category, <see cref="AllSubCategories"/> for every sub-category.</param>
    /// <returns>The sub-category to filter on, or <c>null</c> for no sub-category filter.</returns>
    public static byte? ToSubCategoryFilter(byte category, byte subCategory)
        => category == AllCategories || subCategory == AllSubCategories ? null : subCategory;

    private static Rule[] BuildRules()
    {
        var rules = new List<Rule>();

        void Range(int group, int first, int last, AuctionCategory category, byte subCategory)
            => rules.Add(new Rule(group, first, last, category, subCategory));

        void Numbers(int group, AuctionCategory category, byte subCategory, params int[] numbers)
        {
            foreach (var number in numbers)
            {
                Range(group, number, number, category, subCategory);
            }
        }

        // Jewels and materials.
        Numbers(12, AuctionCategory.JewelMaterial, MaterialKinds.Jewels, 15);
        Numbers(14, AuctionCategory.JewelMaterial, MaterialKinds.Jewels, 13, 14, 16, 22, 31, 42, 160, 161);
        Range(14, 244, 259, AuctionCategory.JewelMaterial, MaterialKinds.Jewels);
        Range(12, 30, 31, AuctionCategory.JewelMaterial, MaterialKinds.Bundles);
        Range(12, 136, 143, AuctionCategory.JewelMaterial, MaterialKinds.Bundles);
        Numbers(14, AuctionCategory.JewelMaterial, MaterialKinds.RefiningStones, 41, 43, 44);
        Range(12, 60, 65, AuctionCategory.JewelMaterial, MaterialKinds.SeedsAndSpheres);
        Range(12, 70, 74, AuctionCategory.JewelMaterial, MaterialKinds.SeedsAndSpheres);
        Range(12, 100, 129, AuctionCategory.JewelMaterial, MaterialKinds.SeedsAndSpheres);
        Numbers(13, AuctionCategory.JewelMaterial, MaterialKinds.Fruits, 15);
        Range(13, 54, 58, AuctionCategory.JewelMaterial, MaterialKinds.Fruits);

        // Scrolls and orbs.
        Range(12, 7, 14, AuctionCategory.ScrollOrb, ScrollKinds.OrbsAndCrystals);
        Range(12, 16, 19, AuctionCategory.ScrollOrb, ScrollKinds.OrbsAndCrystals);
        Range(12, 44, 47, AuctionCategory.ScrollOrb, ScrollKinds.OrbsAndCrystals);
        Numbers(12, AuctionCategory.ScrollOrb, ScrollKinds.ScrollsAndParchments, 21, 22, 23, 24, 35, 48);

        // Capes and cloaks, including the Cape of Lord which lives in group 13.
        Numbers(12, AuctionCategory.Wing, WingKinds.CapesAndCloaks, 40, 49, 50, 130, 135, 154, 156, 161, 163, 208, 210, 211, 234, 240, 242);
        Numbers(13, AuctionCategory.Wing, WingKinds.CapesAndCloaks, 30);

        // Group 12 number 26 is a gem which belongs nowhere else.
        Numbers(12, AuctionCategory.Miscellaneous, 0, 26);

        // Pets, mounts, rings, pendants and eggs of group 13.
        Numbers(13, AuctionCategory.PetHelper, PetKinds.Pets, 0, 1, 4, 5, 64, 65, 67, 80, 106, 123, 145);
        Numbers(13, AuctionCategory.PetHelper, PetKinds.Mounts, 2, 3, 37);
        Range(13, 400, 424, AuctionCategory.PetHelper, PetKinds.Mounts);
        Numbers(13, AuctionCategory.PetHelper, PetKinds.Rings, 8, 9, 10, 20, 21, 22, 23, 24, 39, 40, 41, 42, 68, 76, 107, 109, 110, 111, 112, 122);
        Numbers(13, AuctionCategory.PetHelper, PetKinds.Pendants, 12, 13, 25, 26, 27, 28, 38, 113, 114, 115);
        Range(13, 200, 276, AuctionCategory.PetHelper, PetKinds.Eggs);

        // Potions, elixirs and buff scrolls of group 14.
        Range(14, 0, 8, AuctionCategory.Consumable, ConsumableKinds.Potions);
        Range(14, 35, 40, AuctionCategory.Consumable, ConsumableKinds.Potions);
        Numbers(14, AuctionCategory.Consumable, ConsumableKinds.Potions, 70, 71, 94, 133);
        Range(14, 72, 82, AuctionCategory.Consumable, ConsumableKinds.ElixirsAndBuffs);
        Numbers(14, AuctionCategory.Consumable, ConsumableKinds.ElixirsAndBuffs, 97, 98, 140);

        // Group defaults, always last.
        Range(0, 0, int.MaxValue, AuctionCategory.Weapon, WeaponKinds.Swords);
        Range(1, 0, int.MaxValue, AuctionCategory.Weapon, WeaponKinds.Axes);
        Range(2, 0, int.MaxValue, AuctionCategory.Weapon, WeaponKinds.MacesAndScepters);
        Range(3, 0, int.MaxValue, AuctionCategory.Weapon, WeaponKinds.Spears);
        Range(4, 0, int.MaxValue, AuctionCategory.Weapon, WeaponKinds.BowsAndCrossbows);
        Range(5, 0, int.MaxValue, AuctionCategory.Weapon, WeaponKinds.StavesAndBooks);
        Range(6, 0, int.MaxValue, AuctionCategory.Armor, ArmorKinds.Shields);
        Range(7, 0, int.MaxValue, AuctionCategory.Armor, ArmorKinds.Helms);
        Range(8, 0, int.MaxValue, AuctionCategory.Armor, ArmorKinds.Armors);
        Range(9, 0, int.MaxValue, AuctionCategory.Armor, ArmorKinds.Pants);
        Range(10, 0, int.MaxValue, AuctionCategory.Armor, ArmorKinds.Gloves);
        Range(11, 0, int.MaxValue, AuctionCategory.Armor, ArmorKinds.Boots);
        Range(12, 0, int.MaxValue, AuctionCategory.Wing, WingKinds.Wings);
        Range(13, 0, int.MaxValue, AuctionCategory.PetHelper, PetKinds.SealsAndTickets);
        Range(14, 0, int.MaxValue, AuctionCategory.Consumable, ConsumableKinds.BoxesAndMisc);
        Range(15, 0, int.MaxValue, AuctionCategory.ScrollOrb, ScrollKinds.ScrollsAndParchments);

        return rules.ToArray();
    }

    private readonly record struct Rule(int Group, int First, int Last, AuctionCategory Category, byte SubCategory);
}
```

- [ ] **Step 7: Delegate the old classifier**

Replace the body of `AuctionCategoryClassifier.cs` (keep the file header and class summary) with:

```csharp
public static class AuctionCategoryClassifier
{
    /// <summary>
    /// Classifies an item.
    /// </summary>
    /// <param name="group">The item group.</param>
    /// <param name="number">The item number.</param>
    /// <returns>The category.</returns>
    public static AuctionCategory Classify(int group, int number) => AuctionTaxonomy.Classify(group, number).Category;

    /// <summary>
    /// Classifies an item, treating an explicit list of exact item pairs as jewels or materials before the taxonomy is consulted.
    /// </summary>
    /// <param name="group">The item group.</param>
    /// <param name="number">The item number.</param>
    /// <param name="materialPairs">The exact item pairs classified as jewel or material.</param>
    /// <returns>The category.</returns>
    public static AuctionCategory Classify(int group, int number, IEnumerable<(int Group, int Number)> materialPairs)
        => materialPairs.Any(pair => pair.Group == group && pair.Number == number)
            ? AuctionCategory.JewelMaterial
            : AuctionTaxonomy.Classify(group, number).Category;
}
```

Also update the class summary to: "Assigns the browse category of an item. The category and sub-category tables live in <see cref="AuctionTaxonomy"/>."

- [ ] **Step 8: Run the tests to verify they pass**

Run: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionTaxonomyTests|FullyQualifiedName~AuctionNoteAndCategoryTests"`

Expected: all pass. If `EveryCatalogEntryLandsWhereItsNameSays` lists violations, fix the rule lists in `BuildRules` (the simulation of these exact rules over the fixture had zero violations, so a failure means a transcription slip).

- [ ] **Step 9: Commit**

```bash
git add src/Persistence/AuctionHouse/AuctionTaxonomy.cs src/Persistence/AuctionHouse/AuctionCategory.cs src/Persistence/AuctionHouse/AuctionCategoryClassifier.cs tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionTaxonomyTests.cs tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionNoteAndCategoryTests.cs tests/MUnique.OpenMU.Tests/AuctionHouse/Fixtures/ItemCatalog.tsv tests/MUnique.OpenMU.Tests/MUnique.OpenMU.Tests.csproj
git commit -m "feat(auction): item taxonomy with sub-categories and a Scrolls & Orbs category" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

---

## Task 2: Browse packet field, category enum on the wire, and the 0.9.12 packages

**Files:**
- Modify: `SERVER/tests/MUnique.OpenMU.Network.Packets.Tests/AuctionHousePacketDefinitionTests.cs`
- Modify: `SERVER/src/Network/Packets/ClientToServer/ClientToServerPackets.xml` (the `AuctionBrowseRequest` packet, around line 4289)
- Modify: `SERVER/src/Network/Packets/CommonEnums.xml` (the `AuctionCategory` enum, around line 567)
- Regenerated by the build: `SERVER/tests/MUnique.OpenMU.Network.Packets.Tests/ClientToServerPacketTests.cs`

**Interfaces:**
- Produces: `Wire.AuctionBrowseRequest.SubCategory` (byte, index 104), `AuctionBrowseRequest.Length == 105`, `Packets.AuctionCategory.ScrollOrb == 7`; the packages `MUnique.OpenMU.Network`, `MUnique.OpenMU.PlugIns`, `MUnique.OpenMU.Network.Packets` at `0.9.12` in the local feed.
- Consumed by: Task 5 (handler) and Task 7 (client).

- [ ] **Step 1: Write the failing test change**

In `AuctionHousePacketDefinitionTests.cs`, in the `AuctionBrowseRequest` case change the length and add the field:

```csharp
        yield return new TestCaseData(
            "AuctionBrowseRequest",
            "01",
            "105",
            "4:IntegerLittleEndian:RequestId;8:ShortLittleEndian:Page;10:Byte:Category;11:Enum:CurrencyMode;"
            + "12:Enum:Sort;13:IntegerLittleEndian:ClassMask;17:Byte:LevelMinimum;18:Byte:LevelMaximum;"
            + "19:ShortLittleEndian:OptionFlags;" + Amount(21, "Minimum") + ";" + Amount(45, "Maximum") + ";"
            + "69:ShortLittleEndian:MaxRemainingHours;71:Byte:NameLength;72:Binary:Name[32];104:Byte:SubCategory");
```

and in `EnumCases` change the `AuctionCategory` line to:

```csharp
        yield return new TestCaseData("AuctionCategory", "Weapon=0;Armor=1;Wing=2;PetHelper=3;JewelMaterial=4;Consumable=5;Miscellaneous=6;ScrollOrb=7");
```

- [ ] **Step 2: Run to verify it fails**

Run from `SERVER`: `dotnet test tests/MUnique.OpenMU.Network.Packets.Tests --filter "FullyQualifiedName~AuctionHousePacketDefinitionTests"`

Expected: FAIL in `ClientRequestMatchesDesign` (length 104 vs 105) and `SharedEnumMatchesDesign` for `AuctionCategory`.

- [ ] **Step 3: Change the packet definition**

In `ClientToServerPackets.xml`, in the `AuctionBrowseRequest` packet change `<Length>104</Length>` to `<Length>105</Length>` and add this field after the `Name` field (before `</Fields>`):

```xml
        <Field>
          <Index>104</Index>
          <Type>Byte</Type>
          <Name>SubCategory</Name>
        </Field>
```

In `CommonEnums.xml`, in the `AuctionCategory` enum add after the `Miscellaneous` value:

```xml
        <EnumValue>
          <Name>ScrollOrb</Name>
          <Description>ScrollOrb.</Description>
          <Value>7</Value>
        </EnumValue>
```

- [ ] **Step 4: Run to verify it passes**

Run: `dotnet test tests/MUnique.OpenMU.Network.Packets.Tests --filter "FullyQualifiedName~AuctionHousePacketDefinitionTests|FullyQualifiedName~AuctionBrowseRequest"`

Expected: PASS. The build regenerates `ClientToServerPacketTests.cs` (its `AuctionBrowseRequest_PacketSizeValidation` now expects 105 and checks a `SubCategory` boundary). Check `git diff --stat` shows that file changed.

- [ ] **Step 5: Pack the three packages as 0.9.12**

Run from `SERVER`, in this order (Network and PlugIns first, because the Packets package depends on Network at the same version):

```bash
dotnet pack src/Network/MUnique.OpenMU.Network.csproj -c Release -p:Version=0.9.12 -p:PackageVersion=0.9.12 -o "C:\Users\rause\.local-nuget\auction-house"
dotnet pack src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release -p:Version=0.9.12 -p:PackageVersion=0.9.12 -o "C:\Users\rause\.local-nuget\auction-house"
dotnet pack src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj -c Release -p:Version=0.9.12 -p:PackageVersion=0.9.12 -o "C:\Users\rause\.local-nuget\auction-house"
```

Verify: `unzip -p "C:\Users\rause\.local-nuget\auction-house\MUnique.OpenMU.Network.Packets.0.9.12.nupkg" MUnique.OpenMU.Network.Packets.nuspec` shows `version 0.9.12` and a dependency on `MUnique.OpenMU.Network` version `0.9.12`; the packaged `ClientToServerPackets.xml` contains `SubCategory` (`unzip -p ... contentFiles/any/net10.0/ClientToServer/ClientToServerPackets.xml | grep -c SubCategory` prints 1).

- [ ] **Step 6: Commit**

```bash
git add src/Network/Packets/ClientToServer/ClientToServerPackets.xml src/Network/Packets/CommonEnums.xml tests/MUnique.OpenMU.Network.Packets.Tests/AuctionHousePacketDefinitionTests.cs tests/MUnique.OpenMU.Network.Packets.Tests/ClientToServerPacketTests.cs
git commit -m "feat(auction): browse request carries a sub-category; AuctionCategory gains ScrollOrb" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

---

## Task 3: Stored sub-category, the migration and the browse filter

**Files:**
- Modify: `SERVER/src/DataModel/Entities/AuctionListing.cs`
- Create: `SERVER/src/Persistence/EntityFramework/Migrations/20261013120000_AddAuctionSubCategory.cs`
- Modify: `SERVER/src/Persistence/EntityFramework/Migrations/EntityDataContextModelSnapshot.cs`
- Modify: `SERVER/src/Persistence/AuctionHouse/AuctionSearchTypes.cs`
- Modify: `SERVER/src/Persistence/AuctionHouse/AuctionCreateTypes.cs` (`AuctionListingDraft`)
- Modify: `SERVER/src/Persistence/EntityFramework/AuctionHouse/AuctionHouseRepository.cs`
- Modify: `SERVER/tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionSearchTests.cs`

**Interfaces:**
- Consumes: `AuctionTaxonomy.Classify`, `AuctionTaxonomy.AllSubCategories` (Task 1).
- Produces: `AuctionListing.SubCategory` (byte, default 255); `AuctionListingSearch.SubCategory` (`byte?`, appended last, default null); `AuctionListingDraft.SubCategory` (byte, appended last, default 255); `ListItemAsync` stores both values; `SearchListingsAsync` filters on it.

- [ ] **Step 1: Write the failing PostgreSQL tests**

These run only with `AUCTION_TEST_CONNECTION_STRING` set, like the other 125 database tests (see Task 10). In `AuctionSearchTests.cs`, change the private `List` helper to take a sub-category and pass it to the draft:

```csharp
    private async Task<Guid> List(byte currency, byte category, long startingPrice, int hours = 24, byte subCategory = 255)
    {
        var now = DateTimeOffset.UtcNow;
        var created = await new AuctionHouseRepository().CreateListingAsync(new EntityDataContext(), new AuctionListingDraft(
            OperationId: Guid.NewGuid(),
            AccountId: this._sellerAccountId,
            ItemId: Guid.NewGuid(),
            SellerCharacterId: Guid.NewGuid(),
            SellerName: "Seller",
            SellerIp: "127.0.0.1",
            Currency: currency,
            Category: category,
            StartingPrice: startingPrice,
            CreatedAt: now,
            EndsAt: now.AddHours(hours),
            SubCategory: subCategory));
        return created.ListingId;
    }
```

Add these tests after `ReturnsOnlyActiveListingsOfTheCurrencyAndCategory`:

```csharp
    /// <summary>
    /// Verifies a sub-category narrows the category to exactly the matching listings.
    /// </summary>
    [Test]
    public async Task SubCategoryNarrowsTheCategory()
    {
        await this.List(currency: 0, category: 1, startingPrice: 10, subCategory: 3);
        await this.List(currency: 0, category: 1, startingPrice: 20, subCategory: 4);
        await this.List(currency: 0, category: 0, startingPrice: 30, subCategory: 3);

        var gloves = await this.Search(new AuctionListingSearch(Currency: 0, Category: 1, SubCategory: 3, PageSize: 1000));

        Assert.That(gloves.Rows.Count(r => r.SellerAccountId == this._sellerAccountId), Is.EqualTo(1));
    }

    /// <summary>
    /// Verifies a sub-category the category does not have is an empty page, not an error.
    /// </summary>
    [Test]
    public async Task UnknownSubCategoryReturnsNoRows()
    {
        await this.List(currency: 0, category: 1, startingPrice: 10, subCategory: 3);

        var page = await this.Search(new AuctionListingSearch(Currency: 0, Category: 1, SubCategory: 99, PageSize: 1000));

        Assert.That(page.Rows.Count(r => r.SellerAccountId == this._sellerAccountId), Is.Zero);
    }
```

- [ ] **Step 2: Run to verify it fails**

Run from `SERVER`: `dotnet build tests/MUnique.OpenMU.Tests`

Expected: compile errors `'AuctionListingSearch' does not contain a definition ... 'SubCategory'` and for `AuctionListingDraft`.

- [ ] **Step 3: Add the property, the search field and the draft field**

In `src/DataModel/Entities/AuctionListing.cs`, after the `Category` property add:

```csharp

    /// <summary>
    /// Gets or sets the sub-category snapshot within <see cref="Category"/>. 255 means the listing has not been classified into
    /// a sub-category yet; the maintenance task fills it in.
    /// </summary>
    public byte SubCategory { get; set; } = 255;
```

In `AuctionSearchTypes.cs`, add a doc line and the parameter at the end of `AuctionListingSearch`:

```csharp
/// <param name="SubCategory">The sub-category filter within <paramref name="Category"/>, or <c>null</c> for every sub-category.</param>
public readonly record struct AuctionListingSearch(
    byte? Currency,
    byte? Category = null,
    AuctionSearchSort Sort = AuctionSearchSort.EndingSoonest,
    int Page = 1,
    int PageSize = 8,
    string? Name = null,
    byte LevelMinimum = 0,
    byte LevelMaximum = 0,
    long MinimumScalar = 0,
    long MaximumScalar = 0,
    FruitBasket? MinimumFruits = null,
    FruitBasket? MaximumFruits = null,
    int MaxRemainingHours = 0,
    uint ClassMask = 0,
    AuctionItemOptionFlags OptionFlags = AuctionItemOptionFlags.None,
    byte? SubCategory = null);
```

(Keep the existing `<param>` lines above it; only add the new one.)

In `AuctionCreateTypes.cs`, add to `AuctionListingDraft` a doc line `/// <param name="SubCategory">The sub-category, 255 until it has been classified.</param>` and the trailing parameter:

```csharp
    long? BuyoutPrice = null,
    byte SubCategory = 255);
```

- [ ] **Step 4: Write the migration and the snapshot entry**

Create `SERVER/src/Persistence/EntityFramework/Migrations/20261013120000_AddAuctionSubCategory.cs`:

```csharp
using Microsoft.EntityFrameworkCore.Infrastructure;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace MUnique.OpenMU.Persistence.EntityFramework.Migrations
{
    /// <inheritdoc />
    [DbContext(typeof(EntityDataContext))]
    [Migration("20261013120000_AddAuctionSubCategory")]
    public partial class AddAuctionSubCategory : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<byte>(
                name: "SubCategory",
                schema: "data",
                table: "AuctionListing",
                type: "smallint",
                nullable: false,
                defaultValue: (byte)255);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "SubCategory",
                schema: "data",
                table: "AuctionListing");
        }
    }
}
```

(The Designer file with a full model copy is only needed by the EF tooling; the attributes above register the migration, and the snapshot edit below keeps future `Add-Migration` runs correct.)

Update the snapshot with this script (run from `SERVER`; it inserts `SubCategory` between `Status` and `Version` inside the `AuctionListing` entity only):

```python
p = r"src\Persistence\EntityFramework\Migrations\EntityDataContextModelSnapshot.cs"
t = open(p, encoding="utf-8-sig", newline="").read()
nl = "\r\n" if "\r\n" in t else "\n"
start = t.index('modelBuilder.Entity("MUnique.OpenMU.Persistence.EntityFramework.Model.AuctionListing", b =>')
old = f'b.Property<byte>("Status"){nl}                        .HasColumnType("smallint");{nl}{nl}                    b.Property<int>("Version")'
at = t.index(old, start)
new = (f'b.Property<byte>("Status"){nl}                        .HasColumnType("smallint");{nl}{nl}'
       f'                    b.Property<byte>("SubCategory"){nl}                        .HasColumnType("smallint");{nl}{nl}'
       f'                    b.Property<int>("Version")')
t = t[:at] + new + t[at + len(old):]
open(p, "w", encoding="utf-8-sig", newline="").write(t)
print("ok")
```

Expected: `ok`; `git diff` of the snapshot shows exactly the three added lines inside the `AuctionListing` block.

- [ ] **Step 5: Store both values on create and filter on the sub-category**

In `AuctionHouseRepository.cs`:

1. In `CreateListingAsync(EntityDataContext context, AuctionListingDraft draft, ...)`, in the `new AuctionListing { ... }` initializer add after `Category = draft.Category,`:

```csharp
            SubCategory = draft.SubCategory,
```

2. In `ListItemAsync`, replace `var category = AuctionCategoryClassifier.Classify(facts.ItemGroup, facts.ItemNumber);` with:

```csharp
        var classification = AuctionTaxonomy.Classify(facts.ItemGroup, facts.ItemNumber);
```

and in the initializer replace `Category = (byte)category,` with:

```csharp
            Category = (byte)classification.Category,
            SubCategory = classification.SubCategory,
```

If `category` is used elsewhere in that method, use `classification.Category` there.

3. In the search method, after the existing category filter block add:

```csharp
        if (search.SubCategory is { } subCategory)
        {
            query = query.Where(l => l.SubCategory == subCategory);
        }
```

Add `using MUnique.OpenMU.GameLogic.AuctionHouse;` at the top of the file if `AuctionTaxonomy` does not resolve.

- [ ] **Step 6: Build**

Run: `dotnet build tests/MUnique.OpenMU.Tests`

Expected: build succeeds with no new errors (existing warnings are fine).

- [ ] **Step 7: Run the tests**

Run: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionSearchTests|FullyQualifiedName~AuctionTaxonomyTests"`

Expected: taxonomy tests pass; the PostgreSQL fixture reports the tests as ignored (no connection string). If a disposable database was approved (Task 10), they pass.

- [ ] **Step 8: Commit**

```bash
git add src/DataModel/Entities/AuctionListing.cs src/Persistence/EntityFramework/Migrations/20261013120000_AddAuctionSubCategory.cs src/Persistence/EntityFramework/Migrations/EntityDataContextModelSnapshot.cs src/Persistence/AuctionHouse/AuctionSearchTypes.cs src/Persistence/AuctionHouse/AuctionCreateTypes.cs src/Persistence/EntityFramework/AuctionHouse/AuctionHouseRepository.cs tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionSearchTests.cs
git commit -m "feat(auction): store a sub-category per listing and filter browse on it" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

---

## Task 4: Re-classify existing listings in the maintenance task

**Files:**
- Modify: `SERVER/src/Persistence/AuctionHouse/IAuctionHouseRepository.cs`
- Modify: `SERVER/src/Persistence/EntityFramework/AuctionHouse/AuctionHouseRepository.cs`
- Modify: `SERVER/src/GameLogic/AuctionHouse/AuctionHouseService.cs`
- Modify: `SERVER/src/GameLogic/AuctionHouse/AuctionExpiryPlugIn.cs`
- Create: `SERVER/tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionReclassifyTests.cs`

**Interfaces:**
- Consumes: `AuctionTaxonomy.Classify`, `AuctionTaxonomy.AllSubCategories` (Task 1); `AuctionListing.SubCategory` (Task 3).
- Produces: `IAuctionHouseRepository.ReclassifyListingsAsync(int batchSize, CancellationToken) -> Task<int>` (rows changed); `AuctionHouseService.ReclassifyListingsAsync(int batchSize, CancellationToken) -> Task<int>`.

- [ ] **Step 1: Write the failing tests**

Create `AuctionReclassifyTests.cs`:

```csharp
// <copyright file="AuctionReclassifyTests.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.OpenMU.Tests.AuctionHouse;

using System.Threading;
using Microsoft.EntityFrameworkCore;
using Moq;
using MUnique.OpenMU.GameLogic.AuctionHouse;
using MUnique.OpenMU.Persistence.AuctionHouse;
using MUnique.OpenMU.Persistence.EntityFramework;
using MUnique.OpenMU.Persistence.EntityFramework.AuctionHouse;
using MUnique.OpenMU.Persistence.EntityFramework.Model;

/// <summary>
/// Verifies the one-off re-classification of listings created before sub-categories existed.
/// </summary>
[TestFixture]
public class AuctionReclassifyTests
{
    /// <summary>
    /// Verifies the service hands the batch size to the repository and returns its count.
    /// </summary>
    [Test]
    public async Task ReclassifyDelegatesToTheRepository()
    {
        var repository = new Mock<IAuctionHouseRepository>();
        repository.Setup(r => r.ReclassifyListingsAsync(25, It.IsAny<CancellationToken>())).ReturnsAsync(7);
        var service = new AuctionHouseService(repository.Object, new AuctionHouseOptions { Enabled = true });

        var changed = await service.ReclassifyListingsAsync(25);

        Assert.That(changed, Is.EqualTo(7));
        repository.Verify(r => r.ReclassifyListingsAsync(25, It.IsAny<CancellationToken>()), Times.Once);
    }

    /// <summary>
    /// Verifies the repository re-classifies unclassified listings from their stored item identity, a batch at a time, and
    /// that a finished backlog stays untouched on the next run. Requires <c>AUCTION_TEST_CONNECTION_STRING</c>.
    /// </summary>
    [Test]
    [Category("PostgreSql")]
    public async Task ReclassifyRunsInBatchesUntilNothingIsLeft()
    {
        if (AuctionPostgresTestEnvironment.ConnectionString is null)
        {
            Assert.Ignore($"Set {AuctionPostgresTestEnvironment.ConnectionStringVariable} to run the PostgreSQL integration tests.");
        }

        AuctionPostgresTestEnvironment.EnsureConfigured();
        var seller = Guid.NewGuid();
        try
        {
            await using (var context = new EntityDataContext())
            {
                foreach (var (group, number) in new[] { (10, 3), (14, 3), (13, 8) })
                {
                    context.Set<AuctionListing>().Add(NewListing(seller, group, number));
                }

                await context.SaveChangesAsync();
            }

            var repository = new AuctionHouseRepository();
            var first = await repository.ReclassifyListingsAsync(new EntityDataContext(), batchSize: 2);
            var second = await repository.ReclassifyListingsAsync(new EntityDataContext(), batchSize: 2);
            var third = await repository.ReclassifyListingsAsync(new EntityDataContext(), batchSize: 2);

            await using var check = new EntityDataContext();
            var rows = await check.Set<AuctionListing>().Where(l => l.SellerAccountId == seller).OrderBy(l => l.ItemGroup).ToListAsync();
            Assert.Multiple(() =>
            {
                Assert.That(first, Is.EqualTo(2));
                Assert.That(second, Is.EqualTo(1));
                Assert.That(third, Is.Zero);
                Assert.That(rows.Select(r => ((int)r.Category, (int)r.SubCategory)), Is.EqualTo(new[] { (1, 3), (3, 2), (5, 0) }));
            });
        }
        finally
        {
            await using var cleanup = new EntityDataContext();
            await cleanup.Set<AuctionListing>().Where(l => l.SellerAccountId == seller).ExecuteDeleteAsync();
        }
    }

    private static AuctionListing NewListing(Guid seller, int group, int number)
    {
        var now = DateTimeOffset.UtcNow;
        return new AuctionListing
        {
            Id = Guid.NewGuid(),
            ItemId = Guid.NewGuid(),
            SellerAccountId = seller,
            SellerCharacterId = Guid.NewGuid(),
            SellerName = "Seller",
            Status = 0,
            Currency = 0,
            Category = 6,
            SubCategory = 255,
            StartingPrice = 1,
            CreatedAt = now,
            OriginalEndsAt = now.AddHours(24),
            EndsAt = now.AddHours(24),
            NormalizedName = string.Empty,
            ItemGroup = group,
            ItemNumber = number,
        };
    }
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `dotnet build tests/MUnique.OpenMU.Tests`

Expected: compile errors, `IAuctionHouseRepository` / `AuctionHouseService` have no `ReclassifyListingsAsync`.

- [ ] **Step 3: Add the interface method and the service wrapper**

In `IAuctionHouseRepository.cs`, after `RepairMissingReturnCollectionsAsync`:

```csharp
    /// <summary>
    /// Re-classifies one batch of listings which were created before sub-categories existed (sub-category 255), from their
    /// stored item group and number. Idempotent: a listing is changed once, and a finished backlog changes nothing.
    /// </summary>
    /// <param name="batchSize">The maximum number of listings to change.</param>
    /// <param name="cancellationToken">The cancellation token.</param>
    /// <returns>The number of listings changed.</returns>
    Task<int> ReclassifyListingsAsync(int batchSize, CancellationToken cancellationToken = default);
```

In `AuctionHouseService.cs`, after `RepairAsync`:

```csharp
    /// <summary>
    /// Re-classifies one batch of listings created before sub-categories existed. Runs even while the feature is disabled.
    /// </summary>
    /// <param name="batchSize">The maximum number of listings to change.</param>
    /// <param name="cancellationToken">The cancellation token.</param>
    /// <returns>The number of listings changed.</returns>
    public Task<int> ReclassifyListingsAsync(int batchSize, CancellationToken cancellationToken = default)
        => this._repository.ReclassifyListingsAsync(batchSize, cancellationToken);
```

- [ ] **Step 4: Implement it in the repository**

In `AuctionHouseRepository.cs`, next to `RepairMissingReturnCollectionsAsync` (the public wrapper that uses `WithContextAsync`, then the context overload):

```csharp
    /// <inheritdoc />
    public Task<int> ReclassifyListingsAsync(int batchSize, CancellationToken cancellationToken = default)
        => this.WithContextAsync(context => this.ReclassifyListingsAsync(context, batchSize, cancellationToken));

    /// <summary>
    /// Re-classifies one batch of unclassified listings. See <see cref="IAuctionHouseRepository.ReclassifyListingsAsync"/>.
    /// The listing's version is deliberately left alone, so a bid or buyout in flight is not invalidated.
    /// </summary>
    /// <param name="context">The context to write with.</param>
    /// <param name="batchSize">The maximum number of listings to change.</param>
    /// <param name="cancellationToken">The cancellation token.</param>
    /// <returns>The number of listings changed.</returns>
    public async Task<int> ReclassifyListingsAsync(EntityDataContext context, int batchSize, CancellationToken cancellationToken = default)
    {
        var size = Math.Clamp(batchSize, 1, 1000);
        var rows = await context.Set<AuctionListing>()
            .Where(l => l.SubCategory == AuctionTaxonomy.AllSubCategories)
            .OrderBy(l => l.ListingNumber)
            .Take(size)
            .ToListAsync(cancellationToken);
        foreach (var row in rows)
        {
            var classification = AuctionTaxonomy.Classify(row.ItemGroup, row.ItemNumber);
            row.Category = (byte)classification.Category;
            row.SubCategory = classification.SubCategory;
        }

        if (rows.Count > 0)
        {
            await context.SaveChangesAsync(cancellationToken);
        }

        return rows.Count;
    }
```

- [ ] **Step 5: Call it from the maintenance task**

In `AuctionExpiryPlugIn.cs`, in `ExecuteTaskAsync`, directly after `await service.FinalizeDueListingsAsync(configuration.ExpiryBatchSize).ConfigureAwait(false);` add:

```csharp
            await service.ReclassifyListingsAsync(configuration.ExpiryBatchSize).ConfigureAwait(false);
```

- [ ] **Step 6: Run the tests**

Run: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionReclassifyTests"`

Expected: `ReclassifyDelegatesToTheRepository` passes; the PostgreSQL test is ignored without a connection string (passes if a disposable database was approved in Task 10).

- [ ] **Step 7: Commit**

```bash
git add src/Persistence/AuctionHouse/IAuctionHouseRepository.cs src/Persistence/EntityFramework/AuctionHouse/AuctionHouseRepository.cs src/GameLogic/AuctionHouse/AuctionHouseService.cs src/GameLogic/AuctionHouse/AuctionExpiryPlugIn.cs tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionReclassifyTests.cs
git commit -m "feat(auction): re-classify listings created before sub-categories in the maintenance task" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

---

## Task 5: Browse handler uses the sub-category

**Files:**
- Modify: `SERVER/src/GameServer/MessageHandler/AuctionHouse/AuctionBrowseHandlerPlugIn.cs`
- Create: `SERVER/tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionBrowseSubCategoryTests.cs`

**Interfaces:**
- Consumes: `Wire.AuctionBrowseRequest.SubCategory` (Task 2), `AuctionTaxonomy.ToSubCategoryFilter` and `AuctionListingSearch.SubCategory` (Tasks 1 and 3).

- [ ] **Step 1: Write the failing test**

Create `AuctionBrowseSubCategoryTests.cs`:

```csharp
// <copyright file="AuctionBrowseSubCategoryTests.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.OpenMU.Tests.AuctionHouse;

using System.Threading;
using Moq;
using MUnique.OpenMU.GameLogic.AuctionHouse;
using MUnique.OpenMU.Persistence.AuctionHouse;
using Wire = MUnique.OpenMU.Network.Packets.ClientToServer;

/// <summary>
/// Verifies the sub-category travels from the browse packet to the repository search.
/// </summary>
[TestFixture]
public class AuctionBrowseSubCategoryTests
{
    /// <summary>
    /// Verifies the packet carries the sub-category at the end of its 105 bytes.
    /// </summary>
    [Test]
    public void TheBrowsePacketCarriesTheSubCategoryLast()
    {
        var packet = new byte[Wire.AuctionBrowseRequest.Length];
        var request = new Wire.AuctionBrowseRequest(packet);

        request.SubCategory = 5;

        Assert.Multiple(() =>
        {
            Assert.That(Wire.AuctionBrowseRequest.Length, Is.EqualTo(105));
            Assert.That(packet[104], Is.EqualTo((byte)5));
        });
    }

    /// <summary>
    /// Verifies the service keeps the sub-category when it forwards a browse to the repository.
    /// </summary>
    [Test]
    public async Task BrowseForwardsTheSubCategory()
    {
        AuctionListingSearch? seen = null;
        var repository = new Mock<IAuctionHouseRepository>();
        repository.Setup(r => r.SearchListingsAsync(It.IsAny<AuctionListingSearch>(), It.IsAny<CancellationToken>()))
            .Callback<AuctionListingSearch, CancellationToken>((search, _) => seen = search)
            .ReturnsAsync(default(AuctionListingPage));
        var service = new AuctionHouseService(repository.Object, new AuctionHouseOptions { Enabled = true });

        await service.BrowseAsync(new AuctionListingSearch(Currency: null, Category: 1, SubCategory: 3));

        Assert.That(seen?.SubCategory, Is.EqualTo((byte)3));
    }
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionBrowseSubCategoryTests"`

Expected: `TheBrowsePacketCarriesTheSubCategoryLast` fails only if Task 2's regenerated packet is missing (it should pass); `BrowseForwardsTheSubCategory` passes already because `search with { PageSize }` preserves it. Both passing here is fine: they pin behavior the handler change relies on. Proceed.

- [ ] **Step 3: Map the field in the handler**

In `AuctionBrowseHandlerPlugIn.cs`, in the `new AuctionListingSearch(...)` call add after the `Category:` argument:

```csharp
            SubCategory: AuctionTaxonomy.ToSubCategoryFilter(request.Category, request.SubCategory),
```

(`using MUnique.OpenMU.GameLogic.AuctionHouse;` is already present.)

- [ ] **Step 4: Build and run**

Run: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionBrowseSubCategoryTests|FullyQualifiedName~AuctionHouseHandlerRegistrationTests"`

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/GameServer/MessageHandler/AuctionHouse/AuctionBrowseHandlerPlugIn.cs tests/MUnique.OpenMU.Tests/AuctionHouse/AuctionBrowseSubCategoryTests.cs
git commit -m "feat(auction): the browse handler filters on the requested sub-category" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

---

## Task 6: Server build, full auction suite, restart on the test database

**Files:** none (verification).

- [ ] **Step 1: Run the whole auction suite**

Run from `SERVER`: `dotnet test tests/MUnique.OpenMU.Tests --filter "FullyQualifiedName~AuctionHouse" && dotnet test tests/MUnique.OpenMU.Network.Packets.Tests`

Expected: all pass; the PostgreSQL tests are reported as ignored when no connection string is set. Record the pass and ignored counts.

- [ ] **Step 2: Stop the running server**

PowerShell: `Get-Process MUnique.OpenMU.Startup -ErrorAction SilentlyContinue | Stop-Process -Force`

- [ ] **Step 3: Build the server**

Run from `SERVER`: `dotnet build src/Startup/MUnique.OpenMU.Startup.csproj -c Release`

Expected: build succeeds.

- [ ] **Step 4: Start it and watch the log**

PowerShell:

```powershell
$exe = "C:\Users\rause\OneDrive\Desktop\mu-online-openmu\server\.worktrees\auction-house\src\Startup\bin\Release\MUnique.OpenMU.Startup.exe"
Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
```

Expected: the process stays up. The new migration applies on startup (the same way the earlier auction migrations did); if the server log shows a migration or `SubCategory` error, stop and report it instead of retrying.

---

## Task 7: Client bindings on 0.9.12

**Files:**
- Modify: `CLIENT/ClientLibrary/MUnique.Client.Library.csproj` (line 18)
- Regenerated by the build: `CLIENT/src/source/Dotnet/PacketBindings_ClientToServer.h`, `PacketFunctions_ClientToServer.h`, `PacketFunctions_ClientToServer.cpp`, `PacketFunctions_CommonEnums.h`, `ClientLibrary/ConnectionManager.ClientToServerFunctions.cs`
- Modify: `CLIENT/src/source/UI/NewUI/AuctionHouse/NewUIAuctionWindow.cpp` (the `SendBrowseRequest` call only)

**Interfaces:**
- Consumes: the `0.9.12` packages (Task 2).
- Produces: `SendAuctionBrowseRequest(..., BYTE nameLength, const BYTE* name, uint32_t nameByteLength, BYTE subCategory)` (verify the parameter order in the regenerated header before relying on it).

- [ ] **Step 1: Raise the package version**

In `ClientLibrary/MUnique.Client.Library.csproj` change `<OpenMuPacketsVersion>0.9.11</OpenMuPacketsVersion>` to `<OpenMuPacketsVersion>0.9.12</OpenMuPacketsVersion>`.

- [ ] **Step 2: Stop the client and regenerate**

PowerShell: `Get-Process Main -ErrorAction SilentlyContinue | Stop-Process -Force`

Then build (the ClientLibrary pre-build step restores 0.9.12 from the local feed and regenerates the bindings from its XML):

```powershell
$bat = (Get-ChildItem "C:\Program Files*\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvarsall.bat" | Select-Object -First 1).FullName
cmd /c "`"$bat`" x86 >nul && cd /d C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\auction-house\out\build\windows-x86 && ninja -f build-Debug.ninja Main 2>&1" | Select-String "error|FAILED" | Select-Object -First 10
```

Expected: a C++ error in `NewUIAuctionWindow.cpp` at the `SendAuctionBrowseRequest` call (too few arguments). That confirms the bindings regenerated. If instead there are restore errors, check `nuget.config` lists the local feed and that Task 2 Step 5 produced `0.9.12` files.

- [ ] **Step 3: Confirm the new signature**

Run: `grep -n "SendAuctionBrowseRequest" src/source/Dotnet/PacketFunctions_ClientToServer.h` and read the parameter list; `subCategory` should be the last parameter after `nameByteLength`. Also `grep -n "ScrollOrb" src/source/Dotnet/PacketFunctions_CommonEnums.h` should show the new enum value.

- [ ] **Step 4: Pass "all" for now**

In `NewUIAuctionWindow.cpp`, in `SendBrowseRequest`, change the last two arguments of the `SendAuctionBrowseRequest(...)` call from

```cpp
        searchUtf8.empty() ? nullptr : reinterpret_cast<const BYTE*>(searchUtf8.data()),
        static_cast<uint32_t>(searchUtf8.size()));
```

to

```cpp
        searchUtf8.empty() ? nullptr : reinterpret_cast<const BYTE*>(searchUtf8.data()),
        static_cast<uint32_t>(searchUtf8.size()),
        0xFF); // sub-category: every one (Task 9 sends the picked leaf)
```

- [ ] **Step 5: Build and run the existing tests**

Run the same build command, then `ninja -f build-Debug.ninja auction_model_tests auction_wire_responses_tests` and run both test executables.

Expected: build succeeds; tests pass.

- [ ] **Step 6: Commit**

```bash
git add ClientLibrary/MUnique.Client.Library.csproj src/source/Dotnet src/source/UI/NewUI/AuctionHouse/NewUIAuctionWindow.cpp
git commit -m "feat(client-protocol): regenerate bindings for the browse sub-category (packets 0.9.12)" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

(If git reports generated files in `ClientLibrary/` as untracked or ignored, stage only the ones git tracks; the `.cs` generated file is normally ignored.)

---

## Task 8: Client taxonomy mirror and pure layout (tested)

**Files:**
- Create: `CLIENT/src/source/UI/NewUI/AuctionHouse/AuctionTaxonomy.h`
- Create: `CLIENT/src/source/UI/NewUI/AuctionHouse/AuctionTaxonomy.cpp`
- Create: `CLIENT/tests/ui/test_auction_taxonomy.cpp`
- Modify: `CLIENT/tests/ui/CMakeLists.txt` (the `auction_model_tests` target)

**Interfaces:**
- Produces (namespace `AuctionHouse`): `WireAll`; `SubCategoryInfo{uint8_t wire; const wchar_t* label;}`; `IconItem{int group; int number;}`; `CategoryInfo{uint8_t wire; IconItem icon; std::span<const SubCategoryInfo> subs;}`; `RailCategories()`; `CategoryForRailIndex(int)`; `WireCategoryForRailIndex(int)`; `FindSubCategory(const CategoryInfo&, uint8_t)`; `RailRowText(const wchar_t* categoryLabel, const CategoryInfo* category, uint8_t selectedSub)`; `Rect{x,y,width,height; Contains(px,py)}`; `FlyoutLayout{Rect panel; int firstRowY; int rowHeight; int rowCount; int padding;}`; `LayoutFlyout(panelX, anchorTop, leafCount, rowHeight, width, padding, topLimit, bottomLimit)`; `FlyoutRowAt(layout, x, y)`; `EllipsizeToWidth(text, maxWidth, measure)`.
- Consumed by: Task 9.

- [ ] **Step 1: Write the failing tests**

Create `tests/ui/test_auction_taxonomy.cpp`:

```cpp
#include <doctest.h>

#include <set>
#include <string>

#include "UI/NewUI/AuctionHouse/AuctionTaxonomy.h"

using namespace AuctionHouse;

TEST_CASE("the rail lists eight categories with Miscellaneous last [ui][auction_taxonomy]")
{
    const auto categories = RailCategories();
    REQUIRE(categories.size() == 8);
    const uint8_t expectedWires[] = { 0, 1, 2, 3, 4, 5, 7, 6 };
    for (size_t i = 0; i < categories.size(); ++i)
    {
        CHECK(categories[i].wire == expectedWires[i]);
    }
    CHECK(categories.back().subs.empty());
}

TEST_CASE("rail index zero is every category and others map to wire values [ui][auction_taxonomy]")
{
    CHECK(WireCategoryForRailIndex(0) == WireAll);
    CHECK(WireCategoryForRailIndex(1) == 0);
    CHECK(WireCategoryForRailIndex(2) == 1);
    CHECK(WireCategoryForRailIndex(7) == 7);
    CHECK(WireCategoryForRailIndex(8) == 6);
    CHECK(WireCategoryForRailIndex(9) == WireAll);
    CHECK(WireCategoryForRailIndex(-1) == WireAll);
    CHECK(CategoryForRailIndex(0) == nullptr);
    CHECK(CategoryForRailIndex(9) == nullptr);
    CHECK(CategoryForRailIndex(2)->wire == 1);
}

TEST_CASE("every category's leaves have unique ids and labels [ui][auction_taxonomy]")
{
    const size_t expectedLeaves[] = { 6, 6, 2, 6, 5, 3, 2, 0 };
    const auto categories = RailCategories();
    for (size_t i = 0; i < categories.size(); ++i)
    {
        CHECK(categories[i].subs.size() == expectedLeaves[i]);
        std::set<uint8_t> ids;
        std::set<std::wstring> labels;
        for (const SubCategoryInfo& leaf : categories[i].subs)
        {
            CHECK(leaf.wire != WireAll);
            CHECK(std::wstring(leaf.label).size() > 0);
            ids.insert(leaf.wire);
            labels.insert(leaf.label);
        }
        CHECK(ids.size() == categories[i].subs.size());
        CHECK(labels.size() == categories[i].subs.size());
    }
}

TEST_CASE("leaf ids follow the server table [ui][auction_taxonomy]")
{
    const CategoryInfo& armor = *CategoryForRailIndex(2);
    CHECK(FindSubCategory(armor, 3) != nullptr);
    CHECK(std::wstring(FindSubCategory(armor, 3)->label) == L"Gloves");
    CHECK(std::wstring(FindSubCategory(armor, 5)->label) == L"Shields");
    CHECK(FindSubCategory(armor, 6) == nullptr);
    CHECK(FindSubCategory(armor, WireAll) == nullptr);
}

TEST_CASE("every rail icon is a distinct item [ui][auction_taxonomy]")
{
    std::set<int> icons;
    for (const CategoryInfo& category : RailCategories())
    {
        icons.insert(category.icon.group * 512 + category.icon.number);
    }
    CHECK(icons.size() == RailCategories().size());
}

TEST_CASE("the rail row shows the picked leaf of its own category only [ui][auction_taxonomy]")
{
    const CategoryInfo& armor = *CategoryForRailIndex(2);
    CHECK(RailRowText(L"Armor", &armor, 3) == L"Gloves");
    CHECK(RailRowText(L"Armor", &armor, WireAll) == L"Armor");
    CHECK(RailRowText(L"Armor", &armor, 99) == L"Armor");
    CHECK(RailRowText(L"All Categories", nullptr, 3) == L"All Categories");
}

TEST_CASE("a flyout meets the rail and sits level with its row [ui][auction_taxonomy]")
{
    const FlyoutLayout layout = LayoutFlyout(128, 122, 6, 17, 132, 3, 105, 388);
    CHECK(layout.rowCount == 7);
    CHECK(layout.panel.x == 128);
    CHECK(layout.panel.y == 122);
    CHECK(layout.panel.width == 132);
    CHECK(layout.panel.height == 125);
    CHECK(layout.firstRowY == 125);
}

TEST_CASE("flyout stays inside the limits [ui][auction_taxonomy]")
{
    const FlyoutLayout low = LayoutFlyout(128, 360, 6, 17, 132, 3, 105, 388);
    CHECK(low.panel.y + low.panel.height == 388);

    const FlyoutLayout high = LayoutFlyout(128, 100, 1, 17, 132, 3, 105, 388);
    CHECK(high.panel.y == 105);
}

TEST_CASE("flyout rows are found by position [ui][auction_taxonomy]")
{
    const FlyoutLayout layout = LayoutFlyout(128, 122, 6, 17, 132, 3, 105, 388);
    CHECK(FlyoutRowAt(layout, 140, 125) == 0);
    CHECK(FlyoutRowAt(layout, 140, 142) == 1);
    CHECK(FlyoutRowAt(layout, 140, 243) == 6);
    CHECK(FlyoutRowAt(layout, 140, 244) == -1);
    CHECK(FlyoutRowAt(layout, 129, 130) == -1);
    CHECK(FlyoutRowAt(layout, 140, 124) == -1);
}

TEST_CASE("long labels are cut with dots to fit [ui][auction_taxonomy]")
{
    const auto measure = [](const std::wstring& text) { return static_cast<int>(text.size()) * 6; };
    CHECK(EllipsizeToWidth(L"Bows & Crossbows", 100, measure) == L"Bows & Crossbows");
    CHECK(EllipsizeToWidth(L"Bows & Crossbows", 60, measure) == L"Bows &...");
    CHECK(EllipsizeToWidth(L"Bows & Crossbows", 10, measure) == L"...");
}
```

In `tests/ui/CMakeLists.txt`, in the `auction_model_tests` target add the new sources:

```cmake
mu_add_test(
    NAME auction_model_tests
    SOURCES
        test_auction_model.cpp
        test_auction_sell_draft.cpp
        test_auction_taxonomy.cpp
        ${CMAKE_SOURCE_DIR}/src/source/UI/NewUI/AuctionHouse/AuctionModel.cpp
        ${CMAKE_SOURCE_DIR}/src/source/UI/NewUI/AuctionHouse/AuctionTaxonomy.cpp
)
```

- [ ] **Step 2: Run to verify it fails**

Build: `ninja -f build-Debug.ninja auction_model_tests` in `out/build/windows-x86` (reconfigure first with `cmake .` in that folder if ninja reports a missing `AuctionTaxonomy.cpp`; create both new files empty-bodied first if needed so configure succeeds).

Expected: compile errors, `AuctionTaxonomy.h` not found.

- [ ] **Step 3: Write the header**

Create `AuctionTaxonomy.h`:

```cpp
// <copyright file="AuctionTaxonomy.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>

namespace AuctionHouse
{
    // The wire value which means every category (and, in the sub-category byte, every sub-category).
    inline constexpr uint8_t WireAll = 0xFF;

    struct SubCategoryInfo
    {
        uint8_t wire;
        const wchar_t* label;
    };

    // The item drawn as a category's icon, by (group, number) as on the server.
    struct IconItem
    {
        int group;
        int number;
    };

    // A browse category in rail order. The ids mirror the server's AuctionTaxonomy; the client never classifies.
    struct CategoryInfo
    {
        uint8_t wire;
        IconItem icon;
        std::span<const SubCategoryInfo> subs;
    };

    // The categories below "All Categories", in rail order (Miscellaneous last).
    [[nodiscard]] std::span<const CategoryInfo> RailCategories() noexcept;

    // Rail index 0 is "All Categories"; 1 to RailCategories().size() are the categories.
    [[nodiscard]] const CategoryInfo* CategoryForRailIndex(int railIndex) noexcept;
    [[nodiscard]] uint8_t WireCategoryForRailIndex(int railIndex) noexcept;
    [[nodiscard]] const SubCategoryInfo* FindSubCategory(const CategoryInfo& category, uint8_t wire) noexcept;

    // The text of a rail row: the picked leaf's name when one of this category is picked, else the category's own label.
    [[nodiscard]] std::wstring RailRowText(const wchar_t* categoryLabel, const CategoryInfo* category, uint8_t selectedSub);

    struct Rect
    {
        int x;
        int y;
        int width;
        int height;

        [[nodiscard]] bool Contains(int px, int py) const noexcept
        {
            return px >= x && px < x + width && py >= y && py < y + height;
        }
    };

    struct FlyoutLayout
    {
        Rect panel;
        int firstRowY;
        int rowHeight;
        int rowCount;
        int padding;
    };

    // Places the flyout: its left edge at panelX, level with anchorTop, with one row for "All" plus one per leaf. It is
    // moved up when it would pass bottomLimit and down when it would pass topLimit.
    [[nodiscard]] FlyoutLayout LayoutFlyout(int panelX, int anchorTop, int leafCount, int rowHeight, int width, int padding,
        int topLimit, int bottomLimit) noexcept;

    // The row (0 is "All") under the point, or -1 when it is over the padding or outside the panel.
    [[nodiscard]] int FlyoutRowAt(const FlyoutLayout& layout, int x, int y) noexcept;

    using MeasureText = std::function<int(const std::wstring&)>;

    // Cuts the text and appends "..." until it fits maxWidth. Never returns an empty string.
    [[nodiscard]] std::wstring EllipsizeToWidth(const std::wstring& text, int maxWidth, const MeasureText& measure);
}
```

- [ ] **Step 4: Write the implementation**

Create `AuctionTaxonomy.cpp`:

```cpp
// <copyright file="AuctionTaxonomy.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "UI/NewUI/AuctionHouse/AuctionTaxonomy.h"

namespace
{
    using AuctionHouse::SubCategoryInfo;

    constexpr SubCategoryInfo WeaponSubs[] = {
        { 0, L"Swords" }, { 1, L"Axes" }, { 2, L"Maces & Scepters" }, { 3, L"Spears" },
        { 4, L"Bows & Crossbows" }, { 5, L"Staves & Books" },
    };
    constexpr SubCategoryInfo ArmorSubs[] = {
        { 0, L"Helms" }, { 1, L"Armors" }, { 2, L"Pants" }, { 3, L"Gloves" }, { 4, L"Boots" }, { 5, L"Shields" },
    };
    constexpr SubCategoryInfo WingSubs[] = { { 0, L"Wings" }, { 1, L"Capes & Cloaks" } };
    constexpr SubCategoryInfo PetSubs[] = {
        { 0, L"Pets" }, { 1, L"Mounts" }, { 2, L"Rings" }, { 3, L"Pendants" }, { 4, L"Eggs" }, { 5, L"Seals & Tickets" },
    };
    constexpr SubCategoryInfo JewelSubs[] = {
        { 0, L"Jewels" }, { 1, L"Bundles" }, { 2, L"Refining stones" }, { 3, L"Seeds & Spheres" }, { 4, L"Fruits" },
    };
    constexpr SubCategoryInfo ConsumableSubs[] = {
        { 0, L"Potions" }, { 1, L"Elixirs & Buffs" }, { 2, L"Boxes & Misc" },
    };
    constexpr SubCategoryInfo ScrollSubs[] = { { 0, L"Orbs & Crystals" }, { 1, L"Scrolls & Parchments" } };

    // Rail order: Miscellaneous (wire 6) sits last, after Scrolls & Orbs (wire 7). Icons are items present in item.bmd:
    // Sword of Destruction, Brass Armor, Wings of Dragon, Dark Horse, Jewel of Bless, Large Healing Potion, Scroll of
    // Fire Ball and Box of Luck.
    const AuctionHouse::CategoryInfo Categories[] = {
        { 0, { 0, 16 }, WeaponSubs },
        { 1, { 8, 8 }, ArmorSubs },
        { 2, { 12, 5 }, WingSubs },
        { 3, { 13, 4 }, PetSubs },
        { 4, { 14, 13 }, JewelSubs },
        { 5, { 14, 3 }, ConsumableSubs },
        { 7, { 15, 3 }, ScrollSubs },
        { 6, { 14, 11 }, {} },
    };
}

namespace AuctionHouse
{
    std::span<const CategoryInfo> RailCategories() noexcept
    {
        return Categories;
    }

    const CategoryInfo* CategoryForRailIndex(int railIndex) noexcept
    {
        if (railIndex < 1 || railIndex > static_cast<int>(std::size(Categories)))
        {
            return nullptr;
        }
        return &Categories[railIndex - 1];
    }

    uint8_t WireCategoryForRailIndex(int railIndex) noexcept
    {
        const CategoryInfo* category = CategoryForRailIndex(railIndex);
        return category == nullptr ? WireAll : category->wire;
    }

    const SubCategoryInfo* FindSubCategory(const CategoryInfo& category, uint8_t wire) noexcept
    {
        for (const SubCategoryInfo& leaf : category.subs)
        {
            if (leaf.wire == wire)
            {
                return &leaf;
            }
        }
        return nullptr;
    }

    std::wstring RailRowText(const wchar_t* categoryLabel, const CategoryInfo* category, uint8_t selectedSub)
    {
        if (category != nullptr && selectedSub != WireAll)
        {
            if (const SubCategoryInfo* leaf = FindSubCategory(*category, selectedSub))
            {
                return leaf->label;
            }
        }
        return categoryLabel;
    }

    FlyoutLayout LayoutFlyout(int panelX, int anchorTop, int leafCount, int rowHeight, int width, int padding,
        int topLimit, int bottomLimit) noexcept
    {
        const int rows = leafCount + 1;
        const int height = rows * rowHeight + 2 * padding;
        int top = anchorTop;
        if (top + height > bottomLimit)
        {
            top = bottomLimit - height;
        }
        if (top < topLimit)
        {
            top = topLimit;
        }
        return FlyoutLayout{ Rect{ panelX, top, width, height }, top + padding, rowHeight, rows, padding };
    }

    int FlyoutRowAt(const FlyoutLayout& layout, int x, int y) noexcept
    {
        if (x < layout.panel.x + layout.padding || x >= layout.panel.x + layout.panel.width - layout.padding)
        {
            return -1;
        }
        const int offset = y - layout.firstRowY;
        if (offset < 0 || offset >= layout.rowCount * layout.rowHeight)
        {
            return -1;
        }
        return offset / layout.rowHeight;
    }

    std::wstring EllipsizeToWidth(const std::wstring& text, int maxWidth, const MeasureText& measure)
    {
        if (measure(text) <= maxWidth)
        {
            return text;
        }

        const std::wstring dots = L"...";
        for (size_t length = text.size(); length-- > 0;)
        {
            std::wstring head = text.substr(0, length);
            while (!head.empty() && head.back() == L' ')
            {
                head.pop_back();
            }
            const std::wstring candidate = head + dots;
            if (measure(candidate) <= maxWidth)
            {
                return candidate;
            }
        }
        return dots;
    }
}
```

- [ ] **Step 5: Run to verify it passes**

Build and run: `ninja -f build-Debug.ninja auction_model_tests` then `tests\ui\Debug\auction_model_tests.exe`

Expected: all tests pass (the previous 74 plus the new ones). If `EllipsizeToWidth(..., 60, ...)` returns something other than `Bows &...`, check the trailing-space trim.

- [ ] **Step 6: Commit**

```bash
git add src/source/UI/NewUI/AuctionHouse/AuctionTaxonomy.h src/source/UI/NewUI/AuctionHouse/AuctionTaxonomy.cpp tests/ui/test_auction_taxonomy.cpp tests/ui/CMakeLists.txt
git commit -m "feat(auction-ui): taxonomy mirror, flyout layout and label fitting" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

---

## Task 9: Rail, flyout and request wiring in the window

**Files:**
- Modify: `CLIENT/src/source/UI/NewUI/AuctionHouse/NewUIAuctionWindow.h`
- Modify: `CLIENT/src/source/UI/NewUI/AuctionHouse/NewUIAuctionWindow.cpp`

**Interfaces:**
- Consumes: everything in Task 8; `SendAuctionBrowseRequest(..., subCategory)` (Task 7); `RenderItem3DFittedToSlot` and `Data::Items::MakeItemType` (existing).
- Produces: window members `m_SelectedSubCategory`, `m_bFlyoutOpen`, `m_FlyoutCategoryIndex`; methods `OnCategoryRailClicked(int)`, `SelectCategory(int, uint8_t)`, `HandleCategoryFlyoutMouse()`, `FlyoutLayoutFor(const AuctionHouse::CategoryInfo&, int)`, `RenderCategoryFlyout()`.

There is no unit-test seam for this window (it renders and reads the mouse). All its decisions that can be tested are in Task 8; this task is verified by building, the existing tests, and the owner's in-game check (Task 10).

- [ ] **Step 1: Header changes**

In `NewUIAuctionWindow.h` add the include next to the other AuctionHouse includes:

```cpp
#include "UI/NewUI/AuctionHouse/AuctionTaxonomy.h"
```

Replace the comment and declaration of `m_SelectedCategoryIndex` and add the new state right after it (keeping `m_CurrentPage` next, so constructor initialisation order stays valid):

```cpp
        // 0 = "All Categories"; 1 to 8 are the rail rows, which AuctionHouse::CategoryForRailIndex maps to wire values.
        int m_SelectedCategoryIndex;
        // The picked sub-category of that category, or AuctionHouse::WireAll. It belongs to the category: picking another
        // category resets it.
        uint8_t m_SelectedSubCategory;
        // The sub-category flyout: open or closed, and the rail row it belongs to.
        bool m_bFlyoutOpen;
        int m_FlyoutCategoryIndex;
```

Change the label array and comment:

```cpp
        // Index 0 is "All Categories"; 1 to 8 follow AuctionHouse::RailCategories() order.
        const wchar_t* m_CategoryLabels[9];
```

Add these private method declarations next to `RenderCategoryRail();`:

```cpp
        void RenderCategoryFlyout();
        void OnCategoryRailClicked(int railIndex);
        void SelectCategory(int railIndex, uint8_t subCategory);
        bool HandleCategoryFlyoutMouse();
        AuctionHouse::FlyoutLayout FlyoutLayoutFor(const AuctionHouse::CategoryInfo& category, int railIndex) const;
```

- [ ] **Step 2: Constructor, labels and constants**

In the constructor initialiser list, change `m_SelectedCategoryIndex(0), m_CurrentPage(1),` to:

```cpp
m_SelectedCategoryIndex(0), m_SelectedSubCategory(AuctionHouse::WireAll), m_bFlyoutOpen(false), m_FlyoutCategoryIndex(0), m_CurrentPage(1),
```

Replace the rail constants block (`CATEGORY_RAIL_ROW_HEIGHT = 19;` and the `WALLET_Y` line) with:

```cpp
    constexpr int CATEGORY_RAIL_ROW_COUNT = 9;
    constexpr int CATEGORY_RAIL_ROW_HEIGHT = 17;
    constexpr int CATEGORY_RAIL_ROW_INSET = 1;
    constexpr int CATEGORY_RAIL_ICON_X = 4;
    constexpr int CATEGORY_RAIL_ICON_SIZE = 12;
    constexpr float CATEGORY_RAIL_ICON_FILL = 0.92f;
    constexpr int CATEGORY_RAIL_TEXT_X = 22;
    constexpr int CATEGORY_RAIL_TEXT_PAD = 6;
    // The flyout meets the rail (a few units into the table) and is as tall as its rows need, kept inside the table area.
    constexpr int FLYOUT_X = CATEGORY_RAIL_X + CATEGORY_RAIL_WIDTH + 3;
    constexpr int FLYOUT_WIDTH = 132;
    constexpr int FLYOUT_PADDING = 3;
    constexpr int FLYOUT_ROW_INSET = 1;
```

and change the wallet line to:

```cpp
    constexpr int WALLET_Y = CATEGORY_RAIL_Y + CATEGORY_RAIL_ROW_HEIGHT * CATEGORY_RAIL_ROW_COUNT + 4;
```

(`CATEGORY_RAIL_ROW_HEIGHT` and `WALLET_Y` are defined in that order in the file; put the new constants right after `CATEGORY_RAIL_ROW_INSET`.)

In the constructor body area where the labels are set (the `m_CategoryLabels[...]` block), replace it with:

```cpp
    // Index 0 is "All Categories" (the wire's 0xFF); 1 to 8 follow AuctionHouse::RailCategories(): Weapon, Armor, Wing,
    // Pet & Helper, Jewel & Material, Consumable, Scrolls & Orbs, Miscellaneous.
    m_CategoryLabels[0] = I18N::Game::AllCategories;
    m_CategoryLabels[1] = I18N::Game::Weapon;
    m_CategoryLabels[2] = I18N::Game::Armor;
    m_CategoryLabels[3] = I18N::Game::Wing;
    m_CategoryLabels[4] = I18N::Game::PetHelper;
    m_CategoryLabels[5] = I18N::Game::JewelMaterial;
    m_CategoryLabels[6] = I18N::Game::Consumable;
    m_CategoryLabels[7] = L"Scrolls & Orbs";
    m_CategoryLabels[8] = I18N::Game::Miscellaneous;
    m_CategoryCombo.Setup(m_Pos.x + TOOLBAR_X + CATEGORY_COMBO_X_OFFSET, m_Pos.y + TOOLBAR_Y, CATEGORY_COMBO_WIDTH, CURRENCY_COMBO_ITEM_HEIGHT,
        m_CategoryLabels, 9, m_SelectedCategoryIndex);
```

Add the includes at the top of the .cpp if missing: `#include "Data/GameData/ItemData/ItemType.h"`.

- [ ] **Step 3: The new member functions**

Add these near `RenderCategoryRail` (replace that function's body as shown below):

```cpp
AuctionHouse::FlyoutLayout SEASON3B::CNewUIAuctionWindow::FlyoutLayoutFor(const AuctionHouse::CategoryInfo& category, int railIndex) const
{
    return AuctionHouse::LayoutFlyout(FLYOUT_X, CATEGORY_RAIL_Y + railIndex * CATEGORY_RAIL_ROW_HEIGHT,
        static_cast<int>(category.subs.size()), CATEGORY_RAIL_ROW_HEIGHT, FLYOUT_WIDTH, FLYOUT_PADDING,
        BROWSE_HEADER_Y, BROWSE_BODY_Y + BROWSE_TABLE_HEIGHT + TABLE_BOTTOM_PAD);
}

void SEASON3B::CNewUIAuctionWindow::SelectCategory(int railIndex, uint8_t subCategory)
{
    if (railIndex == m_SelectedCategoryIndex && subCategory == m_SelectedSubCategory)
    {
        return;
    }

    m_SelectedCategoryIndex = railIndex;
    m_SelectedSubCategory = subCategory;
    m_CategoryCombo.SetSelectedIndex(railIndex);
    m_CurrentPage = 1;
    SendBrowseRequest();
}

void SEASON3B::CNewUIAuctionWindow::OnCategoryRailClicked(int railIndex)
{
    const AuctionHouse::CategoryInfo* category = AuctionHouse::CategoryForRailIndex(railIndex);
    if (category == nullptr || category->subs.empty())
    {
        // "All Categories" and Miscellaneous have nothing to open: select them straight away.
        m_bFlyoutOpen = false;
        SelectCategory(railIndex, AuctionHouse::WireAll);
        return;
    }

    // A category with sub-categories toggles its flyout; nothing is requested until a row is picked.
    m_bFlyoutOpen = !(m_bFlyoutOpen && m_FlyoutCategoryIndex == railIndex);
    m_FlyoutCategoryIndex = railIndex;
}

bool SEASON3B::CNewUIAuctionWindow::HandleCategoryFlyoutMouse()
{
    const AuctionHouse::CategoryInfo* category = AuctionHouse::CategoryForRailIndex(m_FlyoutCategoryIndex);
    if (category == nullptr)
    {
        m_bFlyoutOpen = false;
        return false;
    }

    const AuctionHouse::FlyoutLayout layout = FlyoutLayoutFor(*category, m_FlyoutCategoryIndex);
    const int localX = MouseX - m_Pos.x;
    const int localY = MouseY - m_Pos.y;
    const bool overPanel = layout.panel.Contains(localX, localY);
    if (!IsRelease(VK_LBUTTON))
    {
        // While open, the panel owns the mouse over itself.
        return overPanel;
    }

    const int row = AuctionHouse::FlyoutRowAt(layout, localX, localY);
    if (row >= 0)
    {
        m_bFlyoutOpen = false;
        SelectCategory(m_FlyoutCategoryIndex, row == 0 ? AuctionHouse::WireAll : category->subs[row - 1].wire);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (overPanel)
    {
        return true;
    }

    // A release over the rail goes on to the rail's own handling (it toggles or switches the flyout); anywhere else
    // closes the flyout and is consumed.
    if (CheckMouseIn(m_Pos.x + CATEGORY_RAIL_X, m_Pos.y + CATEGORY_RAIL_Y, CATEGORY_RAIL_WIDTH,
            CATEGORY_RAIL_ROW_HEIGHT * CATEGORY_RAIL_ROW_COUNT))
    {
        return false;
    }

    m_bFlyoutOpen = false;
    return true;
}

void SEASON3B::CNewUIAuctionWindow::RenderCategoryFlyout()
{
    if (!m_bFlyoutOpen)
    {
        return;
    }

    const AuctionHouse::CategoryInfo* category = AuctionHouse::CategoryForRailIndex(m_FlyoutCategoryIndex);
    if (category == nullptr)
    {
        return;
    }

    const AuctionHouse::FlyoutLayout layout = FlyoutLayoutFor(*category, m_FlyoutCategoryIndex);
    UI::NineSlice::Render(AuctionPanelSkin().texture, static_cast<float>(m_Pos.x + layout.panel.x),
        static_cast<float>(m_Pos.y + layout.panel.y), static_cast<float>(layout.panel.width),
        static_cast<float>(layout.panel.height), AuctionPanelSkin().tints.normal);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const SIZE lineSize = g_pRenderText->MeasureText(L"Ag", 2);
    const int rowHeight = layout.rowHeight - 2 * FLYOUT_ROW_INSET;
    const int textOffsetY = FLYOUT_ROW_INSET + (rowHeight - lineSize.cy) / 2;
    const int textWidth = layout.panel.width - 2 * layout.padding - 12;
    const bool ownCategorySelected = m_SelectedCategoryIndex == m_FlyoutCategoryIndex;
    const auto measure = [](const std::wstring& text)
    {
        return static_cast<int>(g_pRenderText->MeasureText(text.c_str(), static_cast<int>(text.size())).cx);
    };

    for (int row = 0; row < layout.rowCount; ++row)
    {
        const int y = m_Pos.y + layout.firstRowY + row * layout.rowHeight;
        const uint8_t wire = row == 0 ? AuctionHouse::WireAll : category->subs[row - 1].wire;
        const bool selected = ownCategorySelected && wire == m_SelectedSubCategory;
        const bool hovered = !selected && AuctionHouse::FlyoutRowAt(layout, MouseX - m_Pos.x, MouseY - m_Pos.y) == row;
        UI::NineSlice::Render(AuctionButtonSkin().texture, static_cast<float>(m_Pos.x + layout.panel.x + layout.padding),
            static_cast<float>(y + FLYOUT_ROW_INSET), static_cast<float>(layout.panel.width - 2 * layout.padding),
            static_cast<float>(rowHeight),
            selected ? CATEGORY_TINT_SELECTED : (hovered ? CATEGORY_TINT_HOVER : CATEGORY_TINT_IDLE));

        const std::wstring label = row == 0 ? std::wstring(L"All ") + m_CategoryLabels[m_FlyoutCategoryIndex]
                                            : std::wstring(category->subs[row - 1].label);
        g_pRenderText->SetTextColor(selected ? 255 : 205, selected ? 215 : 205, selected ? 100 : 205, 255);
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x + layout.panel.x + layout.padding + 6),
            static_cast<float>(y + textOffsetY), AuctionHouse::EllipsizeToWidth(label, textWidth, measure).c_str(),
            static_cast<float>(textWidth), 0, RT3_SORT_LEFT);
    }
}
```

Replace the body of `RenderCategoryRail` with (the `>` glyph is gone, the text moves right of the icon, and a picked leaf replaces the category's name):

```cpp
void SEASON3B::CNewUIAuctionWindow::RenderCategoryRail()
{
    // A persistent category rail makes filtering a single, visible action instead of hiding the server's real auction
    // categories inside a dropdown. Each row has a small item icon (drawn in the 3D pass) and shows the picked
    // sub-category's name instead of the category's when one is picked.
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const SIZE lineSize = g_pRenderText->MeasureText(L"Ag", 2);
    constexpr int RowHeight = CATEGORY_RAIL_ROW_HEIGHT - 2 * CATEGORY_RAIL_ROW_INSET;
    const int textOffsetY = CATEGORY_RAIL_ROW_INSET + (RowHeight - lineSize.cy) / 2;
    const auto measure = [](const std::wstring& text)
    {
        return static_cast<int>(g_pRenderText->MeasureText(text.c_str(), static_cast<int>(text.size())).cx);
    };
    for (int category = 0; category < CATEGORY_RAIL_ROW_COUNT; ++category)
    {
        const int y = m_Pos.y + CATEGORY_RAIL_Y + category * CATEGORY_RAIL_ROW_HEIGHT;
        const bool selected = category == m_SelectedCategoryIndex;
        const bool hovered = !selected && CheckMouseIn(m_Pos.x + CATEGORY_RAIL_X, y, CATEGORY_RAIL_WIDTH, CATEGORY_RAIL_ROW_HEIGHT);
        UI::NineSlice::Render(AuctionButtonSkin().texture, static_cast<float>(m_Pos.x + CATEGORY_RAIL_X),
            static_cast<float>(y + CATEGORY_RAIL_ROW_INSET), static_cast<float>(CATEGORY_RAIL_WIDTH),
            static_cast<float>(RowHeight),
            selected ? CATEGORY_TINT_SELECTED : (hovered ? CATEGORY_TINT_HOVER : CATEGORY_TINT_IDLE));

        const int textX = category == 0 ? 8 : CATEGORY_RAIL_TEXT_X;
        const int textWidth = CATEGORY_RAIL_WIDTH - textX - CATEGORY_RAIL_TEXT_PAD;
        const std::wstring text = AuctionHouse::EllipsizeToWidth(
            AuctionHouse::RailRowText(m_CategoryLabels[category], AuctionHouse::CategoryForRailIndex(category),
                selected ? m_SelectedSubCategory : AuctionHouse::WireAll),
            textWidth, measure);
        g_pRenderText->SetTextColor(selected ? 255 : 205, selected ? 215 : 205, selected ? 100 : 205, 255);
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x + CATEGORY_RAIL_X + textX), static_cast<float>(y + textOffsetY),
            text.c_str(), static_cast<float>(textWidth), 0, RT3_SORT_LEFT);
    }
}
```

- [ ] **Step 4: Rail click, flyout input, Escape and state resets**

In `BtnProcess`, replace the old rail block (the `if (IsRelease(VK_LBUTTON) && CheckMouseIn(...CATEGORY_RAIL...)) { ... selectedCategory ... return true; }`) with:

```cpp
            if (m_bFlyoutOpen && HandleCategoryFlyoutMouse())
            {
                return true;
            }

            if (IsRelease(VK_LBUTTON)
                && CheckMouseIn(m_Pos.x + CATEGORY_RAIL_X, m_Pos.y + CATEGORY_RAIL_Y,
                    CATEGORY_RAIL_WIDTH, CATEGORY_RAIL_ROW_HEIGHT * CATEGORY_RAIL_ROW_COUNT))
            {
                OnCategoryRailClicked((MouseY - (m_Pos.y + CATEGORY_RAIL_Y)) / CATEGORY_RAIL_ROW_HEIGHT);
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
```

In the Reset button handler (the block containing `m_SelectedCategoryIndex = 0;`), add after that line:

```cpp
                m_SelectedSubCategory = AuctionHouse::WireAll;
                m_bFlyoutOpen = false;
```

In `UpdateKeyEvent`, at the top of the `if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_AUCTION_HOUSE) == true)` block, before the Sell-tab input handling:

```cpp
        if (m_bFlyoutOpen && SEASON3B::IsPress(VK_ESCAPE) == true)
        {
            m_bFlyoutOpen = false;
            return false;
        }
```

Close the flyout when the window closes, opens, or the tab changes: add `m_bFlyoutOpen = false;` as the first statement of `ClosingProcess()`, next to `m_bShowingDetail = false;` in `OpeningProcess()`, and just before `m_iCurrentTab = selected;` in `Update()`. Also, in `OpeningProcess()` where the Browse state is reset, add `m_SelectedSubCategory = AuctionHouse::WireAll;` next to any `m_SelectedCategoryIndex` reset there (if `OpeningProcess` does not reset the category, leave both unreset so the player's filter persists, as today).

- [ ] **Step 5: Send the category and leaf**

In `SendBrowseRequest`, replace the `constexpr BYTE AllCategories` and `const BYTE category` lines with:

```cpp
    const BYTE category = AuctionHouse::WireCategoryForRailIndex(m_SelectedCategoryIndex);
    const BYTE subCategory = m_SelectedCategoryIndex == 0 ? AuctionHouse::WireAll : m_SelectedSubCategory;
```

and change the last argument added in Task 7 from `0xFF);` to `subCategory);`. Update the comment above (`Category 255 means "every category" ...`) to say the rail index maps through `WireCategoryForRailIndex` and the leaf is sent as `subCategory`.

- [ ] **Step 6: Draw the flyout, the rail icons, and keep icons out from under it**

In `Render()`, immediately after the existing `m_CurrencyCombo.Render();` line (inside the Browse toolbar block) add:

```cpp
            RenderCategoryFlyout();
```

In `IsCoveredByOpenDropdown`, before the final `return false;`, add:

```cpp
    if (m_bFlyoutOpen)
    {
        if (const AuctionHouse::CategoryInfo* category = AuctionHouse::CategoryForRailIndex(m_FlyoutCategoryIndex))
        {
            const AuctionHouse::Rect panel = FlyoutLayoutFor(*category, m_FlyoutCategoryIndex).panel;
            const int panelX = m_Pos.x + panel.x;
            const int panelY = m_Pos.y + panel.y;
            if (x < panelX + panel.width && x + width > panelX && y < panelY + panel.height && y + height > panelY)
            {
                return true;
            }
        }
    }
```

In `Update()` where open dropdowns suppress row hover, extend the condition:

```cpp
        if (m_CurrencyCombo.IsOpen() || m_StatusCombo.IsOpen() || m_CollectionKindCombo.IsOpen() || m_bFlyoutOpen)
```

In `Render3D()`, immediately before the line `const bool hasResponse = m_iCurrentTab == TAB_MAILBOX ? m_bHasMailboxResponse : m_bHasListingResponse;` (confirm with grep that it is unique), add:

```cpp
    if (m_iCurrentTab == TAB_BROWSE)
    {
        for (int railIndex = 1; railIndex <= CATEGORY_RAIL_ROW_COUNT - 1; ++railIndex)
        {
            const AuctionHouse::CategoryInfo* category = AuctionHouse::CategoryForRailIndex(railIndex);
            const int itemType = Data::Items::MakeItemType(category->icon.group, category->icon.number);
            if (!Data::Items::IsValidItemType(itemType))
            {
                continue;
            }

            const int iconX = m_Pos.x + CATEGORY_RAIL_X + CATEGORY_RAIL_ICON_X;
            const int iconY = m_Pos.y + CATEGORY_RAIL_Y + railIndex * CATEGORY_RAIL_ROW_HEIGHT
                + (CATEGORY_RAIL_ROW_HEIGHT - CATEGORY_RAIL_ICON_SIZE) / 2;
            RenderItem3DFittedToSlot(static_cast<float>(iconX), static_cast<float>(iconY),
                static_cast<float>(CATEGORY_RAIL_ICON_SIZE), static_cast<float>(CATEGORY_RAIL_ICON_SIZE), itemType, 0, 0, 0,
                CATEGORY_RAIL_ICON_FILL, false);
        }
    }
```

(`Render3D` returns early for the Sell tab and the detail view before this point, so the icons show only on the Browse list.)

- [ ] **Step 7: Build and test**

Stop the client first (`Get-Process Main | Stop-Process -Force`), then build `Main`, `auction_model_tests`, `ui_scaling_tests`, and run `ctest -C Debug -R "inventory_window_captures_hover|ui_scaling|auction"` in `out/build/windows-x86`.

Expected: build succeeds; all tests pass. Typical compile slips: `CATEGORY_RAIL_ROW_COUNT` used before its definition (move the constants block up), `std::size(m_CategoryLabels)` leftovers (grep for it; there should be none), missing `<string>` include.

- [ ] **Step 8: Commit**

```bash
git add src/source/UI/NewUI/AuctionHouse/NewUIAuctionWindow.h src/source/UI/NewUI/AuctionHouse/NewUIAuctionWindow.cpp
git commit -m "feat(auction-ui): category rail with icons and a sub-category flyout" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

---

## Task 10: Hand-over, verification and notes

**Files:**
- Modify: `CLIENT/docs/auction-house-claude-handover-2026-10-13.md`
- Modify: `CLIENT/docs/superpowers/specs/2026-10-13-auction-categories-design.md` (one clarifying line)
- Modify: `C:\Users\rause\.claude\projects\C--Users-rause-OneDrive-Desktop-Emberion-public-html\memory\project_auction_house.md`

- [ ] **Step 1: Ask before using a database for the PostgreSQL tests**

The `AUCTION_TEST_CONNECTION_STRING` integration tests (Tasks 3 and 4) need a disposable PostgreSQL database. Ask the owner whether to create a scratch database such as `openmu_auction_scratch` on the existing PostgreSQL server and run the PostgreSQL-category tests against it. Do not create it, and do not point the tests at `openmu` or `openmu_auction_test`, without a yes. If the answer is no, report that those tests stayed skipped and that the server-side filter and re-classification are verified in game instead.

- [ ] **Step 2: Relaunch both programs**

Server was restarted in Task 6. Relaunch the client:

```powershell
Get-Process Main -ErrorAction SilentlyContinue | Stop-Process -Force
$exe = "C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\auction-house\out\build\windows-x86\src\Debug\Main.exe"
Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
```

- [ ] **Step 3: Note the spec's rail order and update the handover**

In the spec's Client section add one line after the rail paragraph: `Rail order is the taxonomy array order: Weapon, Armor, Wing, Pet & Helper, Jewel & Material, Consumable, Scrolls & Orbs, Miscellaneous (wire 7 is listed before wire 6).`

In the handover doc, replace the "Not done / next" item about categories if present, and add under "What this session changed": a "Sub-categories" bullet describing the taxonomy table (server `AuctionTaxonomy`), the `SubCategory` column and migration, the re-classification maintenance step, the 105-byte browse request and packets 0.9.12 in the local feed, and the rail and flyout (`AuctionTaxonomy.h`, `RenderCategoryFlyout`).

In the memory file `project_auction_house.md`, add to the current state: sub-categories shipped locally on both branches; the client's packet bindings come from the hand-packed local NuGet feed (bump the three packages together); the PostgreSQL integration tests need a disposable database and are skipped otherwise.

- [ ] **Step 4: Commit the docs and report**

```bash
git add docs/auction-house-claude-handover-2026-10-13.md docs/superpowers/specs/2026-10-13-auction-categories-design.md
git commit -m "docs: sub-categories handover and spec note" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>"
```

Report to the owner what to check in game: the rail's nine rows and icons; opening each flyout (Weapon, Armor, Wing, Pet & Helper, Jewel & Material, Consumable, Scrolls & Orbs) and picking a leaf; the table filtering across pages; Reset; clicking outside and Escape closing the flyout; no table icon showing through the open flyout; and that existing listings (created before this work) appear under the right leaf a few seconds after the server starts.
