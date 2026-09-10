"""Read-only checks of hook ranges and signature discovery in either ASI order."""
from pathlib import Path
import argparse
import re
import pefile

parser = argparse.ArgumentParser()
parser.add_argument('--ginput', type=Path, required=True)
parser.add_argument('--exe', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = (args.ginput / 'src/dllmain.cpp').read_text(encoding='utf-8-sig')
pe = pefile.PE(str(args.exe))
image = bytearray(pe.get_memory_mapped_image())
base = pe.OPTIONAL_HEADER.ImageBase
section = next(s for s in pe.sections if s.Name.rstrip(b'\0') == b'.text')
start, end = section.VirtualAddress, section.VirtualAddress + section.Misc_VirtualSize
patterns = {}
locations = {}
for name, body in re.findall(r'constexpr std::uint8_t k(\w+)Signature\[\] = \{(.*?)\};', source, re.S):
    body = re.sub(r'//[^\n]*', '', body)
    raw = bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', body))
    mask = re.search(rf'k{name}Mask\[\] = "([x?]+)"', source)
    mask = mask[1] if mask else 'x' * len(raw)
    pattern = b''.join(b'.' if m == '?' else re.escape(bytes([b])) for b, m in zip(raw, mask))
    found = [m.start() + start for m in re.finditer(pattern, image[start:end], re.S)]
    assert len(found) == 1, (name, found)
    patterns[name], locations[name] = pattern, found[0]

sites = (root / 'source/code/plugin/HookSites.h').read_text()
achievements = []
# Deferred installation runs after ASI loading and before frontend resource loading.
bootstrap = 0x4BDBED - base
assert image[bootstrap:bootstrap + 5] == bytes.fromhex('E8 3E 9C 01 00')
achievements.append((bootstrap, 5, 'deferred game initialization'))
# Historical frame repair is commented out and is not a release write.
for address, size, raw, name in re.findall(r'\{ (0x[0-9A-F]+), (\d+), \{ ([^}]+) \}, "([^"]+)" \}', sites):
    address = int(address, 16) - base
    expected = bytes(int(x.strip(), 16) for x in raw.split(','))
    assert image[address:address + int(size)] == expected, name
    if name != 'executable signature':
        achievements.append((address, int(size), name))

# Lengths/offsets follow TryInstallHooks and its helper detours in this revision.
hooks = {
    'InputFrame': (0, 5), 'ActionQuery': (0, 7), 'AxisBranch': (0, 6),
    'CameraBranch': (0, 7), 'NativeEvent': (0, 11), 'FrontendConfirm': (0, 5),
    'AssignTarget': (0, 5), 'MovieUpdate': (0, 5), 'RumbleStop': (5, 5),
    'OverlayLayout': (0, 8), 'GlyphCharacter': (0, 8),
    'TriangleToken': (0, 7), 'CircleToken': (0, 7), 'CrossToken': (0, 7),
    'SquareToken': (0, 7), 'TextLookupWrapper': (4, 5)
}
ginput = [(locations[name] + offset, size, name) for name, (offset, size) in hooks.items()]
for a, n, name in achievements:
    for b, m, other in ginput:
        assert max(a, b) >= min(a + n, b + m), (name, other, hex(a + base))

# Do not patch a process or executable: invalidate ranges only in this bytearray.
after_achievements = image.copy()
for a, n, _ in achievements:
    after_achievements[a:a+n] = b'\xCC' * n
for name, pattern in patterns.items():
    found = [m.start() + start for m in re.finditer(pattern, after_achievements[start:end], re.S)]
    assert found == [locations[name]], ('GInput discovery after achievements', name)
after_ginput = image.copy()
for a, n, _ in ginput:
    after_ginput[a:a+n] = b'\xCC' * n
for a, n, name in achievements:
    assert after_ginput[a:a+n] == image[a:a+n], ('Achievement validation after GInput', name)

assert (root / 'source/integration/ManhuntGInputUI.h').read_text() == (args.ginput / 'src/ManhuntGInputUI.h').read_text()
gamepad_pe = pefile.PE(str(args.ginput / 'build/ManhuntGInput.asi'))
assert any(s.name == b'ManhuntGInput_GetUiState' for s in gamepad_pe.DIRECTORY_ENTRY_EXPORT.symbols)
achievement_pe = pefile.PE(str(root / 'build/Release/ManhuntAchievements.asi'))
assert not any('ginput' in i.dll.decode().lower() or 'xinput' in i.dll.decode().lower() for i in achievement_pe.DIRECTORY_ENTRY_IMPORT)
print(f'PASS: {len(achievements)} achievement / {len(ginput)} GInput hook ranges do not overlap')
print('PASS: both signature-discovery orders, matching ABI header/export, no mandatory GInput/XInput import')
print('Static compatibility only; live rendering and controller operation still require an in-game test.')
