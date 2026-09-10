"""Apply the optional UI bridge to the explicitly selected ManhuntGInput project."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
target = Path(sys.argv[1]).resolve()
source = target / 'src/dllmain.cpp'
tests = target / 'tests/input_tests.cpp'
code = source.read_text(encoding='utf-8-sig')
test_code = tests.read_text(encoding='utf-8-sig')
if '#include "ui_bridge.h"' in code:
    raise SystemExit('Bridge already installed; refusing to reapply source edits.')
def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('Source differs from expected integration point: ' + old)
    return text.replace(old, new, 1)
code = replace_once(code, '#include <atomic>', '#include <atomic>\n#include "ManhuntGInputUI.h"')
code = replace_once(code, 'bool g_gamepadUiActive = false;',
    'bool g_gamepadUiActive = false;\nstd::atomic<bool> g_uiBridgeReady{false};\nstd::uint32_t g_uiInputFrame = 0;')
code = replace_once(code, 'void UpdateFrontendInput() {',
    'void UpdateFrontendInput() {\n    ++g_uiInputFrame;')
code = replace_once(code, 'if (TryInstallHooks()) {',
    'if (TryInstallHooks()) {\n            g_uiBridgeReady.store(true, std::memory_order_release);')
code += '\n#include "ui_bridge.h"\n'
test_code = replace_once(test_code, 'int main() {',
    '#include "ui_bridge_tests.h"\n\nint main() {\n    TestExternalUiBridge();')
# Preserve exact pre-integration files for review/recovery in the selected project.
backup = target / 'build/ui-bridge-backup'
backup.mkdir(parents=True, exist_ok=False)
(backup / 'dllmain.cpp').write_bytes(source.read_bytes())
(backup / 'input_tests.cpp').write_bytes(tests.read_bytes())
for relative, content in {
    'src/dllmain.cpp': code,
    'tests/input_tests.cpp': test_code,
    'src/ManhuntGInputUI.h': (root / 'source/integration/ManhuntGInputUI.h').read_text(encoding='utf-8'),
    'src/ui_bridge.h': (root / 'integration/ginput/ui_bridge.h').read_text(encoding='utf-8'),
    'tests/ui_bridge_tests.h': (root / 'integration/ginput/ui_bridge_tests.h').read_text(encoding='utf-8'),
}.items():
    (target / relative).write_text(content, encoding='utf-8')
print('UI bridge added to', target)
