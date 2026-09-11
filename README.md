# Manhunt Achievements

The 33 achievements of the PS4 release of Manhunt, brought to the PC version as
a standalone ASI plugin.

It was split off from [Manhunt.PluginMH](https://github.com/ermaccer/Manhunt.PluginMH)
by **ermaccer** with the author's permission; the description of the game's
classes and structures comes from there, and the RenderWare research it rests on
was helped along by **Fire_Head**. Every unlock condition was verified against
the PS4 trophy script written by **Ernesto Corvi**, so the rules come from the
console release instead of being guessed at. See [CREDITS.md](CREDITS.md).

**[⬇ Download the latest release](https://github.com/Kosobrazik/ManhuntAchievements/releases/latest)** — the ready-to-play archive, no building needed.

*[Русская версия ниже](#русская-версия).*

- Achievement gallery in the main menu and in the pause menu, English and
  Russian text, secret achievements, unlock notifications.
- Keyboard, mouse and gamepad; Xbox/PlayStation glyphs through ManhuntGInput.
- Its own progress file with a backup and import of an earlier profile.
- PluginMH is not required but fully compatible: its menu stays its own and the
  achievements row is appended below it. Delete the old
  `PluginMH_Achievements.asi`.

## Requirements

| | |
| --- | --- |
| Game | The Steam build, `manhunt.exe` SHA-256 `5001b64f…6b52`. On any other build the plugin checks the bytes of every site, finds none of its own and installs nothing — the game keeps running. |
| Loader | [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) x86: put `dinput8.dll` into the game folder and rename it to `ddraw.dll`. |
| System | Windows Vista or newer. No Visual C++ Redistributable needed: static runtime, and the only imports are `kernel32`, `user32` and `winmm`. |

## Installation

Extract into the game folder:

```text
Manhunt/
├─ ManhuntAchievements.asi
├─ ManhuntAchievements.ini
└─ data/txd/achievements.txd
```

The ASI and the INI may move together into `Manhunt/scripts`; leave the texture
in `Manhunt/data/txd`. Never keep two copies of the ASI. Without
`achievements.txd` the achievements still work, the gallery cards just have no
artwork.

## Settings in `ManhuntAchievements.ini`

| Key | Meaning |
| --- | --- |
| `Enabled` | `1` — on. |
| `Sound` | `1` — unlock sound, embedded in the ASI, no extra files needed. |
| `Language` | `0` — follow the game, `1` — English, `2` — Russian. |
| `AllowWithCheats` | `1` — cheats do not interfere, as in the original; `0` — a scene played with a cheat does not count. |
| `Log` | `0` — off, `1` — kills and scenes, `2` — everything. |

The PS4 trophy script never looks at cheats at all, so `1` matches the original
and `0` makes the plugin stricter than it. Either way the rabbit and monkey
skins count both through the cheat and through PluginMH's skin selector — but
only once the matching pair of episodes is finished with five stars, the very
condition the game itself uses to unlock those cheats.

The log is written next to the ASI. Level `2` is extremely chatty and meant for
a single troubleshooting run, not for playing.

## Controls, saves, building

Arrow keys, the mouse wheel, the D-pad or the left stick pick a card; LB/RB or
L1/R1 change the page; Esc, B or Circle go back. Gamepad glyphs need a
ManhuntGInput build exporting `ManhuntGInput_GetUiState` ABI 1; without it the
gallery uses ordinary prompts.

Progress lives in the user's Manhunt folder as `Achievements/achievements.dat`
with a `.bak` beside it. On the first run it is imported from the earlier
locations; the originals are neither deleted nor modified.

Building needs Visual Studio 2022 or newer, the Windows SDK and RenderWare
headers — those are proprietary and are not part of this repository, so put them
into `third_party/rw` or point `RWSDK` at them;
[fakerw](https://github.com/GTAmodding/re3/tree/master/src/fakerw) works, as it
does for the original PluginMH.

```powershell
./tools/build.ps1 -Configuration Release -Test
./tools/package.ps1 -Version 2.0.0 -Repository https://github.com/Kosobrazik/ManhuntAchievements
```

`package.ps1` splits `dist/<version>` in two: `mod-sites` carries the player
archive with a single `README.txt` inside, `github` carries the repository tree.
The player's text lives in `packaging/README.txt` — that is the file to edit.

Internals, hook addresses and detection rules are in
[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) (Russian). What changed —
[CHANGELOG.md](CHANGELOG.md). Authorship and terms — [CREDITS.md](CREDITS.md).

---

# Русская версия

33 достижения PS4-издания Manhunt в PC-версии игры — отдельный ASI-плагин.

Выделен из [Manhunt.PluginMH](https://github.com/ermaccer/Manhunt.PluginMH)
**ermaccer** с разрешения автора; описание игровых классов и структур оттуда же,
а в исследовании RenderWare, на котором оно стоит, помогал **Fire_Head**. Все
условия сверены со скриптом трофеев PS4-издания, который написал **Ernesto
Corvi**, — то есть правила взяты у консольной версии, а не угаданы. Подробности
— [CREDITS.md](CREDITS.md).

**[⬇ Скачать последнюю версию](https://github.com/Kosobrazik/ManhuntAchievements/releases/latest)** — готовый архив, собирать ничего не нужно.

- Галерея достижений в главном меню и в меню паузы, английский и русский текст,
  секретные достижения, уведомления о разблокировке.
- Клавиатура, мышь и геймпад; значки Xbox/PlayStation — через ManhuntGInput.
- Свой файл прогресса с резервной копией и импортом прежнего профиля.
- PluginMH не нужен, но полностью совместим: его меню остаётся его, строка
  достижений добавляется снизу. Старый `PluginMH_Achievements.asi` удалите.

## Что нужно

| | |
| --- | --- |
| Игра | Steam-сборка, `manhunt.exe` SHA-256 `5001b64f…6b52`. На другой сборке плагин сверит байты каждого участка, не найдёт своих и ничего не поставит — игра не сломается. |
| Загрузчик | [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) x86: `dinput8.dll` в папку игры, переименовать в `ddraw.dll`. |
| Система | Windows Vista или новее. Visual C++ Redistributable не нужен: статический runtime, импорты только `kernel32`, `user32`, `winmm`. |

## Установка

Распаковать в папку игры:

```text
Manhunt/
├─ ManhuntAchievements.asi
├─ ManhuntAchievements.ini
└─ data/txd/achievements.txd
```

ASI и INI можно перенести вместе в `Manhunt/scripts`, текстуру оставить в
`Manhunt/data/txd`. Двух копий ASI быть не должно. Без `achievements.txd`
достижения работают, но карточки останутся без картинок.

## Настройки `ManhuntAchievements.ini`

| Параметр | Значение |
| --- | --- |
| `Enabled` | `1` — включено. |
| `Sound` | `1` — звук разблокировки, встроен в ASI, отдельные WAV не нужны. |
| `Language` | `0` — язык игры, `1` — английский, `2` — русский. |
| `AllowWithCheats` | `1` — читы не мешают, как в оригинале; `0` — сцена с читом не засчитывается. |
| `Log` | `0` — выключено, `1` — убийства и сцены, `2` — всё подряд. |

Скрипт трофеев PS4 читы не проверяет вовсе, поэтому `1` повторяет оригинал, а
`0` делает плагин строже него. Скины кролика и обезьяны при любом значении
засчитываются и читом, и селектором скинов PluginMH — но только после пары
эпизодов на пять звёзд, то есть по условию, по которому игра открывает эти читы
сама.

Лог пишется рядом с ASI. Уровень `2` очень многословен и предназначен для одного
разбирательства, а не для игры.

## Управление, сохранения, сборка

Стрелки, колесо мыши, крестовина или левый стик — выбор карточки; LB/RB или
L1/R1 — страница; Esc, B или круг — назад. Значки геймпада требуют ManhuntGInput
с экспортом `ManhuntGInput_GetUiState` ABI 1; без него — обычные подсказки.

Прогресс — в пользовательской папке Manhunt: `Achievements/achievements.dat` и
`.bak` рядом. При первом запуске переносится из прежних расположений, оригиналы
не удаляются и не изменяются.

Сборка: Visual Studio 2022 или новее, Windows SDK и заголовки RenderWare —
проприетарные, в репозиторий не входят, поэтому положите их в `third_party/rw`
или укажите путь переменной `RWSDK`; подойдёт и
[fakerw](https://github.com/GTAmodding/re3/tree/master/src/fakerw), как для
оригинального PluginMH.

```powershell
./tools/build.ps1 -Configuration Release -Test
./tools/package.ps1 -Version 2.0.0 -Repository https://github.com/Kosobrazik/ManhuntAchievements
```

`package.ps1` раскладывает `dist/<версия>` на две части: `mod-sites` — архив для
игрока с одним `README.txt` внутри, `github` — дерево репозитория. Текст для
игрока лежит в `packaging/README.txt`, его и правьте.

Устройство, адреса хуков и правила распознавания —
[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md). Что изменилось —
[CHANGELOG.md](CHANGELOG.md). Авторство и условия — [CREDITS.md](CREDITS.md).
