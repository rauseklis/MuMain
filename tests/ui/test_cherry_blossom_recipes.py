"""Check the shipped recipes consumed by the native Cherry Blossom mix window."""
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
KEY = (0xFC, 0xCF, 0xAB)
RECIPE_SIZE = 656


def decode(data):
    return bytes(value ^ KEY[index % 3] for index, value in enumerate(data))


def cherry_recipes():
    data = (ROOT / "src/bin/Data/Local/mix.bmd").read_bytes()
    counts = struct.unpack("<14i", decode(data[:56]))
    offset = 56 + sum(counts[:9]) * RECIPE_SIZE
    return [decode(data[offset + index * RECIPE_SIZE:offset + (index + 1) * RECIPE_SIZE])
            for index in range(counts[9])]


class CherryBlossomRecipes(unittest.TestCase):
    def test_all_three_exchange_counts_are_shipped(self):
        recipes = cherry_recipes()
        self.assertEqual(len(recipes), 3)
        actual = {}
        for row in recipes:
            self.assertEqual(struct.unpack_from("<i", row, 652)[0], 1)
            self.assertEqual(struct.unpack_from("<i", row, 324)[0], 100)
            item_min, item_max, *fields = struct.unpack_from("<2h9i", row, 332)
            self.assertEqual(item_min, item_max)
            self.assertEqual(fields[6], fields[7])
            actual[item_min] = (struct.unpack_from("<i", row, 4)[0], fields[6])
        self.assertEqual(actual, {14 * 512 + 88: (42, 10),
                                  14 * 512 + 89: (43, 30),
                                  14 * 512 + 90: (41, 255)})


if __name__ == "__main__":
    unittest.main()
