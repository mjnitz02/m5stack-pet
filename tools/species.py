"""
Parse the ASCII species in src/buddies/*.cpp. Stdlib only, shared by the
renderer and the tests.
"""
import re
from pathlib import Path

PROJECT = Path(__file__).resolve().parent.parent
SRC = PROJECT / "src" / "buddies"
STATES = ["sleep", "idle", "play", "attention", "celebrate", "dizzy", "heart"]

# Characters that fit across the 135px screen at the home screen's 2x scale
# (12px per glyph). Wider art would have its edges cut off; the tests refuse it.
FIT_COLS = 11

FUNC = re.compile(r"static void do(\w+)\(uint32_t t\)\s*\{")
POSE = re.compile(r"static const char\* const (\w+)\[\d+\]\s*=\s*\{(.*?)\};", re.DOTALL)
STR = re.compile(r'"((?:[^"\\]|\\.)*)"')
SPEC = re.compile(r'extern const Species (\w+) = \{\s*"(\w+)",\s*(0x[0-9A-Fa-f]+)')
TABLE = re.compile(r"SPECIES_TABLE\[\]\s*=\s*\{(.*?)\};", re.DOTALL)


class Species:
    def __init__(self, path):
        text = path.read_text()
        m = SPEC.search(text)
        if not m:
            raise ValueError(f"{path.name}: no Species definition")
        self.path = path
        self.symbol, self.name, self.color565 = m.group(1), m.group(2), int(m.group(3), 16)
        # {state: [(pose_name, [lines])]} in source order
        self.poses = {}
        funcs = list(FUNC.finditer(text))
        for i, f in enumerate(funcs):
            end = funcs[i + 1].start() if i + 1 < len(funcs) else len(text)
            self.poses[f.group(1).lower()] = [
                (p.group(1), [re.sub(r"\\(.)", r"\1", s) for s in STR.findall(p.group(2))])
                for p in POSE.finditer(text[f.end():end])
            ]

    def first(self, state):
        poses = self.poses.get(state)
        return poses[0][1] if poses else []

    @property
    def rgb(self):
        v = self.color565
        return ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31)

    @property
    def ink_width(self):
        """Widest line of any pose, ignoring the padding the renderer trims."""
        return max(len(line.strip()) for ps in self.poses.values() for _, ls in ps for line in ls)


def load_all():
    return sorted((Species(p) for p in SRC.glob("*.cpp")), key=lambda s: s.name)


def registered_symbols():
    """Species symbols listed in SPECIES_TABLE in buddy.cpp."""
    text = (PROJECT / "src" / "buddy.cpp").read_text()
    return re.findall(r"&(\w+)", TABLE.search(text).group(1))
