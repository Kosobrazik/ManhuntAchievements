# Авторство и условия · Credits and terms

Плагин производный. Ниже — что откуда взято и на каких условиях
распространяется. *(English summary at the end.)*

## ermaccer — [Manhunt.PluginMH](https://github.com/ermaccer/Manhunt.PluginMH)

Автор разрешил выделить достижения в самостоятельный плагин при условии
публикации открытого исходного кода. Baseline — commit `26f91c8`.

Оттуда унаследовано всё описание игры: `source/code/manhunt/*` (19 файлов
перенесены без изменений, 16 сокращены и дополнены под нужды достижений),
`source/MemoryMgr.h` и `source/code/RenderWare.h`. Чтение фактического размера
больших файлов сохраняет поведение `eCustomTableOfContents` от ermaccer —
благодаря ему снят лимит 12 МБ на `frontend_pc.txd`. Комментарии и уведомления
об авторстве в перенесённых файлах сохранены как есть.

Собственный код проекта — `source/code/plugin`, `source/code/core`,
`source/integration`, `tests`, `tools`: сама система достижений, их условия,
профиль, галерея, совместимость с чужими плагинами.

## Fire_Head

В оригинальном PluginMH указан за большую помощь в исследовании RenderWare. На
нём стоят унаследованные описания игровых объектов и вся отрисовка галереи.

## Ernesto Corvi

Скрипт трофеев PS4-издания (`SLUS-20827`), по которому сверены все 33 условия, —
его работа. Скрипт входит в состав PS4-издания и здесь не публикуется; куда его
положить для сверки, сказано в `docs/DEVELOPMENT.md`.

## DK22Pac и участники plugin-sdk

Подход к хукам и обвязка восходят к
[plugin-sdk](https://github.com/DK22Pac/plugin-sdk), commit `5d7c561`, zlib-лицензия
(Dmitry K., fastman92, LINK/2012). Её требования — не выдавать чужую работу за
свою и помечать изменённое — выполнены этим файлом. `source/MemoryMgr.h` —
известный в GTA-моддинге MemoryMgr, попавший сюда через PluginMH.

## Criterion Software и Rockstar Games

Заголовки RenderWare принадлежат Criterion, проприетарны и в репозиторий не
входят (см. `third_party/rw/README.md`). Manhunt, названия, звуки и графика
принадлежат Rockstar Games. Файлов игры плагин не содержит, кроме штатного звука
интерфейса, встроенного в ASI, и значков в `data/txd/achievements.txd`; по
требованию правообладателя они будут убраны.

## Условия

Свой код проекта можно изучать, изменять и распространять при сохранении
указания авторства и ссылок отсюда. Унаследованные из PluginMH файлы держатся на
личном разрешении ermaccer, а не на лицензии, — расширить его в одностороннем
порядке нельзя: чтобы выпустить проект под MIT, zlib или подобной лицензией,
нужно согласие ermaccer. Плагин распространяется как есть, без гарантий.

## English

Derived work. The game class headers (`source/code/manhunt/*`, `MemoryMgr.h`,
`RenderWare.h`) come from **ermaccer**'s
[Manhunt.PluginMH](https://github.com/ermaccer/Manhunt.PluginMH), used with the
author's permission on the condition that the source stays open; **Fire_Head** is
credited there for the RenderWare research they rest on. Every achievement
condition was verified against the PS4 trophy script written by **Ernesto
Corvi**, which is not redistributed here. Hooking follows
[plugin-sdk](https://github.com/DK22Pac/plugin-sdk) (zlib). RenderWare headers
belong to Criterion and are not part of this repository; Manhunt and its assets
belong to Rockstar Games. The project's own code may be used and modified with
attribution; the inherited files rest on ermaccer's personal permission, so
releasing the whole under a named open-source license needs his agreement.
Provided as is, without warranty.
