==============================================================================
 Manhunt Achievements 2.0.0
 33 достижения PS4-издания Manhunt в PC-версии игры
==============================================================================

ЧТО НУЖНО

  Manhunt для Windows, Steam-сборка (manhunt.exe с SHA-256 5001b64f...6b52).
  На другой сборке плагин ничего не установит, но и не сломает игру.

  ASI Loader. Возьмите Ultimate ASI Loader для x86:
  https://github.com/ThirteenAG/Ultimate-ASI-Loader
  Положите dinput8.dll в папку игры и переименуйте в ddraw.dll.

  Windows Vista или новее. Ничего больше ставить не нужно: Visual C++
  Redistributable не требуется.

УСТАНОВКА

  Распакуйте архив в папку игры, чтобы получилось так:

    Manhunt\ManhuntAchievements.asi
    Manhunt\ManhuntAchievements.ini
    Manhunt\data\txd\achievements.txd

  ASI и INI можно перенести вместе в папку Manhunt\scripts, текстуру при этом
  оставьте в Manhunt\data\txd. Двух копий ASI быть не должно.

  PluginMH не нужен, но полностью совместим: его меню остаётся его, строка
  достижений добавляется снизу. Старый PluginMH_Achievements.asi удалите.

НАСТРОЙКИ (файл ManhuntAchievements.ini)

  Enabled          1 - включено.
  Sound            1 - звук разблокировки.
  Language         0 - язык игры, 1 - английский, 2 - русский.
  AllowWithCheats  1 - читы не мешают, как в оригинале;
                   0 - сцена с включённым читом не засчитывается.
  Log              0 - выключено, 1 - убийства и сцены, 2 - всё подряд.

  Скины кролика и обезьяны засчитываются и читом, и селектором скинов PluginMH,
  но только после пары эпизодов на пять звёзд - по тому же условию, по которому
  игра открывает эти читы сама.

УПРАВЛЕНИЕ

  Стрелки, колесо мыши, крестовина или левый стик - выбор карточки.
  LB/RB или L1/R1 - страница. Esc, B или круг - назад.
  Значки кнопок Xbox/PlayStation появятся, если установлен ManhuntGInput.

СОХРАНЕНИЯ

  Прогресс лежит в пользовательской папке Manhunt рядом с сохранениями игры:
  Achievements\achievements.dat и резервная копия .bak рядом. Прогресс от
  прежних версий переносится сам, оригиналы не трогаются.

ЧТО НОВОГО В 2.0

  Условия всех 33 достижений сверены со скриптом трофеев PS4-издания.
  Убийства краном, холодильником и взрывающимися бочками распознаются
  по самой игре. В тени засчитывается только тело, а не предмет в руках.
  Pink Mist требует снайперской винтовки и попадания в голову.
  Время считается игровыми часами, смерть откатывает прогресс к чекпоинту.
  Читы проверяются каждый кадр. Плагин больше не зависит от версии PluginMH
  и не перебивает чужие хуки.

АВТОРЫ

  Плагин выделен из Manhunt.PluginMH (ermaccer) с разрешения автора; оттуда же
  описание игровых классов. В исследовании RenderWare помогал Fire_Head.
  Условия достижений сверены со скриптом трофеев PS4-издания, который написал
  Ernesto Corvi. Manhunt принадлежит Rockstar Games.
{REPOSITORY}
==============================================================================
 ENGLISH
==============================================================================

  The 33 trophies of the PS4 release of Manhunt, as a plugin for the PC version.

  REQUIREMENTS. The Steam build of Manhunt (manhunt.exe, SHA-256 5001b64f...6b52
  - on any other build the plugin installs nothing and breaks nothing). An ASI
  loader: Ultimate ASI Loader x86, dinput8.dll renamed to ddraw.dll. Windows
  Vista or newer; no Visual C++ Redistributable needed.

  INSTALL. Extract into the game folder so that you get
  Manhunt\ManhuntAchievements.asi, Manhunt\ManhuntAchievements.ini and
  Manhunt\data\txd\achievements.txd. The ASI and INI may move together into
  Manhunt\scripts; leave the texture in Manhunt\data\txd. Never keep two copies
  of the ASI. PluginMH is not required but fully compatible - remove the old
  PluginMH_Achievements.asi.

  SETTINGS in the INI: Enabled, Sound, Language (0 game, 1 English, 2 Russian),
  AllowWithCheats (1 matches the original, which never checks cheats; 0
  disqualifies a scene played with one), Log (0 off, 1 kills and scenes, 2
  everything). The rabbit and monkey skins count through the cheat or through
  PluginMH's skin selector, but only once their episode pair is finished with
  five stars - the game's own unlock condition.

  CONTROLS. Arrows, wheel, D-pad or left stick pick a card; LB/RB or L1/R1
  change the page; Esc, B or Circle go back. Gamepad glyphs appear when
  ManhuntGInput is installed.

  PROGRESS lives in the user's Manhunt folder as Achievements\achievements.dat
  with a .bak beside it, imported from earlier versions automatically.

  CREDITS. Split off from Manhunt.PluginMH by ermaccer with the author's
  permission; the game class headers come from there, and Fire_Head is credited
  in it for the RenderWare research they rest on. Every condition was verified
  against the PS4 trophy script written by Ernesto Corvi. Manhunt belongs to
  Rockstar Games.
