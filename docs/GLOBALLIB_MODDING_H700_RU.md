# GlobalLib: модификации OpenUG2 без изменения оригинала

Источник: https://github.com/NFSTools/GlobalLib — MIT, copyright 2020 MaxHwoy & NFS Tools.

## Исследовано
GlobalLib работает с GlobalA.bun, GlobalB.lzc, TPK, FNG, CarTypeInfo,
Preset Rides, GCareerRace, GCareerStage, Sponsor, WorldShop и сохранением
редактированных BIN. CareerManager/Reading/ReadGCareerRaces.cs использует
записи 0x88, а Writing/Assemble.cs пересобирает таблицу строк и блоки.

GlobalLib — C# для .NET Framework 4.6 с Windows Forms/WPF и DevILNet.
Исходную DLL не встраиваем в порт H700: на ARM64/MuOS используем
самостоятельный C99 reader и простой локальный текстовый формат.

## Реализовано сейчас
Файлы: src/ug2_mods.c, src/ug2_mods.h, tools/ug2_mod_edit.py.
Моды живут в ports/openug2/mods/career_rewards.txt. Исходный
GLOBAL/GlobalB.lzc остаётся нетронутым. Мод считается валидным,
только если ID гонки есть в оригинальном каталоге NFS Underground 2.

Пример содержимого файла career_rewards.txt:

    OPENUG2_CAREER_MOD_V1
    # оригинальная карьерная гонка, пока используется только в каталоге
    S3_SPRINT_6=1000
    # награда в существующей прототипной AI circuit гонке (было 500)
    @PROTOTYPE_CIRCUIT_CASH=750

Шаблон: portmaster/openug2/mods/career_rewards.example.txt.
Скопируйте его как career_rewards.txt, раскомментируйте нужные строки.
Лаунчер PortMaster при наличии файла сам передаёт --career-mod.
Отдельный скрипт запуска не требуется.

Важно:
- @PROTOTYPE_CIRCUIT_CASH меняет сумму после подтверждённой победы
  над AI в уже работающей гонке, если включён --career-save.
- S3_SPRINT_6 и другие настоящие ID проходят проверку и меняют
  загруженный каталог наград. Их **фактическое начисление** при
  прохождении карьеры пока НЕ подключено: сначала нужно правильно
  связать WorldEvent с GCareerRace по проверенным данным.
- Ошибки формата, дубликаты ID, значения вне 0..10 000 000,
  неизвестные события отклоняют весь мод, не меняя каталог.
- Размер текстового мода не должен превышать 64 КиБ.
- Бинарные ресурсы EA, файлы GlobalB и сохранения в GitHub не входят.

## Быстрое редактирование на ПК
Собрать локальную программу чтения каталога:

    make ug2-career-dump ug2-mods-test

Просмотреть ID оригинальных событий:

    python3 tools/ug2_mod_edit.py --globalb "/games/NFSU2/GLOBAL/GlobalB.lzc" list

Добавить или изменить выплату определённого события:

    python3 tools/ug2_mod_edit.py --globalb "/games/NFSU2/GLOBAL/GlobalB.lzc" --mod "/sdcard/ports/openug2/mods/career_rewards.txt" set S3_SPRINT_6 1000

Изменить реально работающую временную выплату AI-circuit:

    python3 tools/ug2_mod_edit.py --globalb "/games/NFSU2/GLOBAL/GlobalB.lzc" --mod "/sdcard/ports/openug2/mods/career_rewards.txt" set-prototype 750

Редактор защищает оригинальный GlobalB, отказывается менять символические
ссылки, записывает только отдельный текстовый файл через временный файл
и переименование. Требует Python 3 на ПК; на устройстве только C99-модуль.

## Следующие этапы
1. Достоверно сопоставить оригинальные ID GCareerRace с событиями карты
   и финишами, подключить оригинальную и модифицированную выплату.
2. Добавить безопасные изменения этапов, спонсоров, автосалонов и unlock,
   проверяя взаимные зависимости.
3. При необходимости сделать экспорт/конвертер формата GlobalLib в
   портируемые моды для OpenUG2, без установки .NET на H700.
4. Только после проверки — отдельный ПК writer GlobalB с резервными копиями.

Проверки: make ug2-mods-test, python3 tools/ug2_mod_edit_test.py;
ARM64/GLES2 GitHub Actions проверяет также компиляцию игрового бинарника.
Настоящая полная карьера пока не реализована.
