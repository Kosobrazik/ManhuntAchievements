# Заголовки RenderWare

Здесь должны лежать заголовки RenderWare SDK (`rwcore.h`, `rwplcore.h`,
`rpworld.h`, `rphanim.h` и их зависимости). Они принадлежат Criterion Software,
распространяются по её условиям и поэтому в репозиторий не входят.

Проект собирается с любым набором, который даёт эти объявления:

- заголовки RenderWare 3.x для D3D8 — тот же вариант, что использует
  оригинальный Manhunt;
- [fakerw](https://github.com/GTAmodding/re3/tree/master/src/fakerw) — открытая
  замена объявлений, которую рекомендует и
  [Manhunt.PluginMH](https://github.com/ermaccer/Manhunt.PluginMH).

Положите файлы прямо в эту папку либо укажите свой путь при сборке:

```powershell
$env:RWSDK = 'C:\path\to\rwsdk\include\d3d8'
./tools/build.ps1 -Configuration Release -Test
```

Без переменной `RWSDK` проект берёт эту папку — так задано в
`source/ManhuntAchievements.vcxproj`.

Заголовки нужны только как объявления типов: библиотека RenderWare в сборку не
линкуется, игра приносит её с собой.
