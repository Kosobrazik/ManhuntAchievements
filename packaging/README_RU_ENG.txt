==============================================================================
 Manhunt Achievements — Достижения / Achievements
==============================================================================

==============================================================================
ENGLISH GUIDE
==============================================================================

  The 33 PS4 trophies of Manhunt for the PC version, with a gallery in the
  main/pause menus, secret achievements and unlock notifications. This is a
  standalone ASI plugin; PluginMH and ManhuntGInput are optional.

REQUIREMENTS

  Manhunt for Windows (Steam version) and a 32-bit ASI loader.
  Windows Vista or newer. Other game builds may not be compatible.
  No Visual C++ Redistributable is needed.

INSTALLATION

  1. Install a Win32/x86 ASI loader using its instructions, even on 64-bit Windows.
  2. Extract into the game folder containing manhunt.exe:

    Manhunt\ManhuntAchievements.asi
    Manhunt\ManhuntAchievements.ini
    Manhunt\data\txd\achievements.txd
    Manhunt\data\ManhuntAchievements\lang\template.txt.example

     ASI and INI may move together into Manhunt\scripts if supported by the
     loader. Leave data in the game root. Keep one ASI copy and back up your
     INI before updating. Without the TXD, cards have no artwork but still work.
  3. Open the achievement gallery from the main or pause menu.

  There is one ManhuntAchievements.ini. Remove the old
  PluginMH_Achievements.asi when switching to the standalone plugin.

CONTROLS

  Arrow keys, mouse wheel, D-pad or left stick select cards. LB/RB or L1/R1
  change pages; Esc, B or Circle go back. Xbox/PlayStation icons require a
  compatible ManhuntGInput.

SETTINGS

  ManhuntAchievements.ini

  The INI lives beside the ASI. Language comes first, then main settings;
  external translations and diagnostics are at the bottom. Comments are in
  English and Russian. Restart after manual changes.

  Language          auto follows the game / MHLL; ru Russian, en English;
                    de German, fr French, es Spanish, it Italian.
  Enabled           1 enables the plugin, 0 disables it.
  Sound             Embedded unlock sound; no separate WAV is needed.
  AllowWithCheats    1 matches the PS4 script, where cheats do not block trophies;
                    0 disqualifies a scene played with a cheat.
  TranslationFiles  External translations, used with Language=auto.
  Log               0 off; 1 kills/scenes; 2 detailed troubleshooting.

  Rabbit/monkey skins count through cheats or PluginMH's selector, but only
  after their episode pair is completed with five stars, as in the game.

SAVES

  Progress is in the user's Manhunt folder beside game saves:
  Achievements\achievements.dat and a .bak backup. Previous progress is imported
  automatically without deleting the originals. Existing game star ratings
  count for the four three-star scene-block achievements; other conditions
  are recorded while the plugin is running.

TROUBLESHOOTING

  Check the game build, Win32 ASI loader and ASI/INI locations.
  Missing artwork: check data\txd\achievements.txd. Missing gamepad icons:
  check compatible ManhuntGInput. No translation: check Language=auto, Match,
  UTF-8 encoding and the .txt extension.

  Set Log=1 or Log=2, restart and reproduce the issue. Save ManhuntAchievements.log beside
  the ASI and include it with the report. Level 2 is for one troubleshooting
  run; restore Log=0 afterwards.

UNINSTALLATION

  Close the game and remove this mod's ASI and INI files. Remove its log
  if one was created. Other installed mods do not need to be removed.
  Achievement progress is kept separately and can be preserved for reinstalling.

LANGUAGE AND TRANSLATIONS

  Language=auto follows the game, including MHLL translations. ru, en, de, fr,
  es, it select built-in languages and disable external gallery translations.
  Full English names work case-insensitively; legacy values 0–6 still load.
  Unknown values fall back to auto. Only the
  achievements interface changes. The game font must support the alphabet.

  1. Copy data\ManhuntAchievements\lang\template.txt.example into the same
     folder with a .txt name, for example polski.txt.
  2. Set Match to the exact controls-type label in the game translation
     (GXT key CONTTYP, the first controls-menu row).
  3. Translate Gallery.* and the 33 entries: number/name, then a description
     on the next line. Save UTF-8. Keep both %d fields in Gallery.TitleCount.
     Use the character codes supported by the translation's GXT and font.
  4. Keep Language=auto and the default TranslationFiles, then restart. Changing
     language through MHLL selects the matching file.

  The .example file is not loaded. Several translations may coexist. Missing
  strings remain English. The template explains the full format.

CREDITS

  Thanks to ermaccer for PluginMH and making its source code available.
  https://github.com/ermaccer/Manhunt.PluginMH
  Split from PluginMH with the author's permission; game class headers
  come from that project. Fire_Head helped with RenderWare research.
  Conditions were checked against Ernesto Corvi's PS4 trophy script.
  Unofficial fan mod. Manhunt belongs to Rockstar Games.

==============================================================================
ИНСТРУКЦИЯ НА РУССКОМ
==============================================================================

  33 достижения PS4-издания Manhunt в PC-версии: галерея в главном меню и
  паузе, секретные достижения и уведомления о разблокировке. Это отдельный
  ASI-плагин; PluginMH и ManhuntGInput необязательны.

ЧТО НУЖНО

  Manhunt для Windows (Steam-версия) и 32-битный ASI Loader.
  Windows Vista или новее. Другие сборки игры могут быть несовместимы.
  Visual C++ Redistributable не требуется.

УСТАНОВКА

  1. Установите ASI Loader по его инструкции. Нужен Win32/x86 даже на
     64-битной Windows.
  2. Распакуйте файлы в папку игры, где лежит manhunt.exe:

    Manhunt\ManhuntAchievements.asi
    Manhunt\ManhuntAchievements.ini
    Manhunt\data\txd\achievements.txd
    Manhunt\data\ManhuntAchievements\lang\template.txt.example

     ASI и INI можно перенести вместе в Manhunt\scripts, если загрузчик
     поддерживает эту папку. Папку data оставьте в корне игры.
     Не оставляйте две копии ASI. Перед обновлением сохраните свой INI.
     Без TXD достижения работают, но карточки остаются без картинок.
  3. Запустите игру и откройте галерею достижений в главном меню или паузе.

  Используется один ManhuntAchievements.ini. Старый
  PluginMH_Achievements.asi удалите при переходе на отдельный плагин.

УПРАВЛЕНИЕ

  Стрелки, колесо мыши, крестовина или левый стик — выбор карточки.
  LB/RB или L1/R1 — страница. Esc, B или круг — назад.
  Значки Xbox/PlayStation доступны с совместимым ManhuntGInput.

НАСТРОЙКИ

  ManhuntAchievements.ini

  INI лежит рядом с ASI. Язык — первый параметр, затем основные настройки;
  внешний перевод и диагностика — внизу. Подписи INI — на английском и русском.
  После ручного изменения перезапустите игру.

  Language          auto — язык игры / MHLL; ru — русский, en — английский;
                    de — немецкий, fr — французский, es — испанский, it — итальянский.
  Enabled           1 — включено, 0 — выключено.
  Sound             1 — звук разблокировки, встроенный в ASI.
  AllowWithCheats    1 — читы не мешают, как в скрипте трофеев PS4;
                    0 — сцена с включённым читом не засчитывается.
  TranslationFiles  Путь к внешним переводам, применяется при Language=auto.
  Log               0 — выключено; 1 — убийства и сцены; 2 — подробный журнал.

  Скины кролика и обезьяны засчитываются и читом, и селектором PluginMH,
  но только после пары эпизодов на пять звёзд — по условию самой игры.

СОХРАНЕНИЯ

  Прогресс лежит в пользовательской папке Manhunt рядом с сохранениями:
  Achievements\achievements.dat и резервная копия .bak. Прежний прогресс
  переносится автоматически, оригиналы не удаляются.

  Звёзды, заработанные до установки мода, засчитываются: четыре достижения
  «три звезды на каждой сцене блока из пяти» читают рейтинги самой игры.
  Остальные условия учитываются при работающем плагине.

ЕСЛИ ЧТО-ТО НЕ РАБОТАЕТ

  Проверьте сборку игры, Win32 ASI Loader и расположение ASI/INI.
  Нет картинок: проверьте data\txd\achievements.txd.
  Нет значков геймпада: проверьте совместимый ManhuntGInput.
  Нет перевода: проверьте Language=auto, Match, UTF-8 и расширение .txt.

  Для диагностики поставьте Log=1 или Log=2, перезапустите игру и повторите
  проблему. Сохраните ManhuntAchievements.log рядом с ASI и приложите
  при сообщении об ошибке.
  Log=2 предназначен для одного разбора; после проверки верните Log=0.

УДАЛЕНИЕ

  Закройте игру и удалите ASI и INI этого мода, а также его журнал,
  если он был создан. Остальные установленные моды удалять не нужно.
  Прогресс достижений хранится отдельно: его можно оставить для переустановки.

ЯЗЫК И ПЕРЕВОДЫ

  Language=auto следует языку игры, включая переводы через MHLL. ru, en, de,
  fr, es, it выбирают встроенный язык и отключают внешний перевод галереи.
  Полные названия English, Russian и другие также принимаются без учёта
  регистра. Старые значения 0–6 сохранены; неизвестное значение включает auto.
  Настройка относится только к достижениям. Буквы выбранного языка должны
  поддерживаться шрифтом игры.

  1. Скопируйте data\ManhuntAchievements\lang\template.txt.example в ту же
     папку под именем с окончанием .txt, например polski.txt.
  2. Укажите Match = точная подпись «тип управления» вашего перевода игры
     (GXT-ключ CONTTYP, первая строка меню управления).
  3. Переведите Gallery.* и список 33 достижений: номер и название, затем
     описание на следующей строке. Сохраните UTF-8. В Gallery.TitleCount
     сохраните два %d для счётчика. Следуйте кодам букв и шрифту GXT.
  4. Оставьте Language=auto и стандартный TranslationFiles, перезапустите игру.
     При смене языка через MHLL мод выбирает файл с подходящим Match.

  Файлы .example не загружаются. Несколько переводов могут лежать рядом.
  Пропущенные строки остаются английскими; образец объясняет формат.

БЛАГОДАРНОСТИ

  Спасибо ermaccer за PluginMH и опубликованные исходники.
  https://github.com/ermaccer/Manhunt.PluginMH
  Выделен из PluginMH с разрешения автора; описание классов
  игры взято оттуда же. В исследовании RenderWare помогал Fire_Head.
  Условия сверены со скриптом трофеев PS4, написанным Ernesto Corvi.
  Неофициальный фанатский мод. Manhunt принадлежит Rockstar Games.
{REPOSITORY}
