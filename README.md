# Manhunt Achievements

33 достижения PS4-издания Manhunt в PC-версии игры — отдельный ASI-плагин.
Выделен из [Manhunt.PluginMH](https://github.com/ermaccer/Manhunt.PluginMH)
**ermaccer** с разрешения автора; описание игровых классов оттуда же, в
исследовании RenderWare помогал **Fire_Head**, условия сверены со скриптом
трофеев PS4-издания **Ernesto Corvi**. Подробности — [CREDITS.md](CREDITS.md).

*[English below](#english).*

- Галерея в главном меню и в меню паузы, английский и русский текст, секретные
  достижения, уведомления.
- Клавиатура, мышь и геймпад; значки Xbox/PlayStation — через ManhuntGInput.
- Свой файл прогресса с резервной копией и импортом прежнего профиля.
- PluginMH не нужен, но полностью совместим: его меню остаётся его, строка
  достижений добавляется снизу. Старый `PluginMH_Achievements.asi` удалите.

## Что нужно

| | |
| --- | --- |
| Игра | Steam-сборка, `manhunt.exe` SHA-256 `5001b64f…6b52`. На другой сборке плагин сверит байты, не найдёт своих и ничего не поставит — игра не сломается. |
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
| `Sound` | `1` — звук разблокировки (встроен в ASI, отдельные WAV не нужны). |
| `Language` | `0` — язык игры, `1` — английский, `2` — русский. |
| `AllowWithCheats` | `1` — читы не мешают, как в оригинале; `0` — сцена с читом не засчитывается. |
| `Log` | `0` — выключено, `1` — убийства и сцены, `2` — всё подряд. |

Скрипт трофеев PS4 читы не проверяет вовсе, поэтому `1` повторяет оригинал.
Скины кролика и обезьяны при любом значении засчитываются и читом, и селектором
PluginMH — но только после пары эпизодов на пять звёзд, то есть по условию, по
которому игра открывает эти читы сама.

## Управление, сохранения, сборка

Стрелки, колесо мыши, крестовина или левый стик — выбор карточки; LB/RB или
L1/R1 — страница; Esc, B или круг — назад. Значки геймпада требуют ManhuntGInput
с экспортом `ManhuntGInput_GetUiState` ABI 1; без него — обычные подсказки.

Прогресс — в пользовательской папке Manhunt: `Achievements/achievements.dat` и
`.bak` рядом. При первом запуске переносится из прежних расположений, оригиналы
не трогаются.

Сборка: Visual Studio 2022+, Windows SDK и заголовки RenderWare (проприетарные,
в репозиторий не входят — положить в `third_party/rw` или указать `RWSDK`,
подойдёт [fakerw](https://github.com/GTAmodding/re3/tree/master/src/fakerw)).

```powershell
./tools/build.ps1 -Configuration Release -Test
./tools/package.ps1 -Version 2.0.0 -Repository https://github.com/...
```

`package.ps1` раскладывает `dist/<версия>` на две части: `mod-sites` — архив для
игрока с одним `README.txt` внутри, `github` — дерево репозитория. Текст для
игрока лежит в `packaging/README.txt`, его и правьте.

Устройство, адреса хуков и правила распознавания — `docs/DEVELOPMENT.md`. Что
изменилось — [CHANGELOG.md](CHANGELOG.md).

---

## English

The 33 trophies of the PS4 release of Manhunt, as a standalone ASI plugin for
the PC version. Split off from
[Manhunt.PluginMH](https://github.com/ermaccer/Manhunt.PluginMH) by **ermaccer**
with the author's permission; the game class headers come from there, the
RenderWare research behind them was helped by **Fire_Head**, and every condition
was checked against the PS4 trophy script by **Ernesto Corvi**. See
[CREDITS.md](CREDITS.md).

Gallery in the main and pause menus, English and Russian text, secret
achievements, notifications, keyboard/mouse/gamepad control, its own progress
file. PluginMH is not required but fully compatible — its menu stays its own and
the achievements row is appended below. Remove the old
`PluginMH_Achievements.asi`.

**Requirements.** The Steam build of Manhunt, `manhunt.exe` SHA-256
`5001b64f…6b52` (on any other build the plugin checks the bytes, finds none of
its sites and installs nothing). An ASI loader —
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) x86,
`dinput8.dll` renamed to `ddraw.dll`. Windows Vista or newer; no Visual C++
Redistributable needed.

**Install** by extracting `ManhuntAchievements.asi`, `ManhuntAchievements.ini`
and `data/txd/achievements.txd` into the game folder. The ASI and the INI may
move together into `Manhunt/scripts`; leave the texture in `Manhunt/data/txd`.
Never keep two copies of the ASI.

**Settings** in the INI: `Enabled`, `Sound`, `Language` (`0` game, `1` English,
`2` Russian), `AllowWithCheats` (`1` matches the original, which never checks
cheats at all; `0` disqualifies a scene played with one), `Log` (`0` off, `1`
kills and scenes, `2` everything). The rabbit and monkey skins count either way
— through the cheat or through PluginMH's skin selector — but only once their
episode pair is finished with five stars, the game's own unlock condition.

**Controls.** Arrows, wheel, D-pad or left stick pick a card; LB/RB or L1/R1
change the page; Esc, B or Circle go back. Gamepad glyphs need ManhuntGInput
exporting `ManhuntGInput_GetUiState` ABI 1.

**Progress** lives in the user's Manhunt folder as
`Achievements/achievements.dat` with a `.bak` beside it, imported from earlier
locations on first run.

**Building** needs Visual Studio 2022+, the Windows SDK and RenderWare headers,
which are proprietary and not part of this repository — put them in
`third_party/rw` or point `RWSDK` at them;
[fakerw](https://github.com/GTAmodding/re3/tree/master/src/fakerw) works. Then
`./tools/build.ps1 -Configuration Release -Test`. Internals and hook addresses
are documented in `docs/DEVELOPMENT.md` (Russian).
