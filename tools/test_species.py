"""
Checks on the species art in src/buddies/, run against the real source
files. These catch the mistakes that are easy to make when adding or
removing a species and that the compiler will not: a file that is never
registered, a missing state, a pose with the wrong number of rows, art too
wide for the screen, a character the stick's font does not have.

  uv run --no-project python -m unittest discover -s tools
"""
import unittest

from species import FIT_COLS, STATES, load_all, registered_symbols


class SpeciesArt(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.species = load_all()

    def test_found_some(self):
        self.assertGreater(len(self.species), 0)

    def test_name_matches_file(self):
        for sp in self.species:
            self.assertEqual(sp.name, sp.path.stem)

    def test_every_state_has_art(self):
        for sp in self.species:
            for st in STATES:
                self.assertTrue(sp.first(st), f"{sp.name}: no pose for {st}")
            self.assertEqual(sorted(sp.poses), sorted(STATES), sp.name)

    def test_poses_are_five_rows(self):
        for sp in self.species:
            for st, poses in sp.poses.items():
                for pose, lines in poses:
                    self.assertEqual(len(lines), 5, f"{sp.name}.{st}.{pose}")

    def test_fits_the_screen(self):
        for sp in self.species:
            for st, poses in sp.poses.items():
                for pose, lines in poses:
                    wide = max(len(line.strip()) for line in lines)
                    self.assertLessEqual(wide, FIT_COLS, f"{sp.name}.{st}.{pose}")

    def test_plain_ascii(self):
        # The source is UTF-8 and the firmware prints it a byte at a time, so
        # anything else comes out as the wrong glyph and throws the row off centre.
        for sp in self.species:
            for st, poses in sp.poses.items():
                for pose, lines in poses:
                    for line in lines:
                        self.assertTrue(line.isascii(), f"{sp.name}.{st}.{pose}: {line!r}")

    def test_table_matches_files(self):
        self.assertEqual(sorted(registered_symbols()), sorted(sp.symbol for sp in self.species))


if __name__ == "__main__":
    unittest.main()
