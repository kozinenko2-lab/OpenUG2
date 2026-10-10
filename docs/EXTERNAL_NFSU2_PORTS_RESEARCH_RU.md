# Исследование сторонних портов и исходников NFS Underground 2 — 10.10.2026

**Цель:** сократить работу до полностью играбельной карьеры на Anbernic RG40XX H (H700, ARM64, 1 ГБ RAM, SDL2, OpenGL ES2, PortMaster), не смешивая несовместимые версии игры и не копируя данные EA в GitHub.

## Краткий вывод

Наибольшая непосредственная польза **не от Switch-рендерера**, а от открытых декодеров карьерных данных NFSU2 PC в `NFSTools/GlobalLib`. Следом — независимый PortMaster-порт `Detoy/OpenUG2` на R36S. Порт `antoxa2584x/nfsu2-sw` действительно рекомпилирует **полную Xbox-версию** игры, но построен вокруг Xbox XBE/ресурсов, NV2A и современных OpenGL/Vulkan; для GLES2/Mali H700 это самостоятельный большой проект.

## Подтверждённые источники

| Проект | Код и состояние | Использование для H700 |
|---|---|---|
| [NFSTools/GlobalLib](https://github.com/NFSTools/GlobalLib) | MIT, C#, PC GlobalA/GlobalB; декодирует GCareerRace, GCareerStage, Sponsor, WorldChallenge, WorldShop, GCarUnlock и SMS | **Первый приоритет.** Чтение событий/цен/условий из легальных PC-файлов вместо временных чисел. Весь парсинг нужно валидировать на игре |
| [Detoy/OpenUG2](https://github.com/Detoy/OpenUG2) | Реальный PortMaster ARM64/GLES2 прототип для R36S; работа L4RA подтверждается автором, карьеры и геймпада нет | Сопоставить launcher, `port.json`, `gameinfo.xml`, sysroot и importer, не заменять наши карьерные изменения |
| [antoxa2584x/nfsu2-sw](https://github.com/antoxa2584x/nfsu2-sw) | Xbox NTSC-U static recomp `default.xbe` -> C, Linux/Switch; Linux заявляет рабочие Boot/Movies/Profile/Main Menu/Quick Race/Career; на Switch заявлены меню/гонки, **не полное прохождение** | **Отдельный исследовательский путь**, не готовая ARM64/GLES2 сборка для H700; пригоден для проверки поведения и техники рекомпиляции |
| [kvnxp/nfsu2-sw](https://github.com/kvnxp/nfsu2-sw) | Форк Xbox recomp с дополнительным macOS ARM64 и клавиатурным управлением | Изучить переносимость host/backend, отладку; не решает GLES2 автоматически |
| [yugecin/nfsu2-re](https://github.com/yugecin/nfsu2-re) | Подробный PC reverse-engineering, документация и игровые hooks | Сопоставление структур и поведения; не готовый Linux порт |
| [cobanov/nfsu2-decomp](https://github.com/cobanov/nfsu2-decomp) | Начальная matching decomp GameCube/PC: README на 10.10.26 сообщает **0 matched functions** | Следить за развитием; прямо сейчас код карьеры не получить |
| [PoQue00/OpenUG2-VITA](https://github.com/PoQue00/OpenUG2-VITA) | Форк OpenUG2 с обычным Linux/SDL2 Makefile, без обнаруженной цели VitaSDK/VPK | Не считать готовым Vita-портом |
| [kollehond/OpenUG2_360](https://github.com/kollehond/OpenUG2_360) | Форк с целью попробовать сборку для Xbox 360; обычный Makefile | Исследовательский форк, не готовое решение |
| [whoismept/OpenUG2](https://github.com/whoismept/OpenUG2) | Чистая реконструкция PC-движка; по README ещё прототип | Наша базовая архитектура: уже умеет GLES2 и PC-форматы |

### Детали по Switch-порту

- Последний подтверждённый релиз: [v0.5, 8 октября 2026](https://github.com/antoxa2584x/nfsu2-sw/releases/tag/v0.5) — отражения на авто, внутриигровое масштабирование, исправления.
- Реализованы Xbox APU (звук), запуск профиля, загрузочные ролики, сохранения в `game/UDATA`, SDL/контроллеры, в том числе split screen. Автор **не сообщает о гарантированном полном прохождении карьеры на Switch**.
- В старых [замерах](https://github.com/antoxa2584x/nfsu2-sw/blob/main/PERF_NOTES.md) на Switch: около 19,5–22,7 fps в гонке после оптимизаций (29.09.2026; **не актуальный benchmark v0.5** и не оценка H700).
- Renderer GL использует `#version 330 core` в `xboxrecomp/src/nv2a_gl/gl_psh.c`, `glBindVertexArray`, `glMapBufferRange`, `glBlitFramebuffer`. Это **не GLES2 renderer**. Альтернативный Vulkan backend неприменим к H700.
- Xbox XBE/папки `NFSUNDER` и PC `TRACKS/CARS/GLOBALB` — разные наборы файлов. Не пытаться скопировать Xbox ELF/NRO или Xbox текстуры в наш PC-ориентированный порт.
- Показанная в `xboxrecomp/LICENSE` лицензия MIT относится к инструменту, а не автоматически к сгенерированному коду оригинального Xbox EXE. Никаких декомпилированных или рекомпилированных proprietary EA функций не вставлять в наш публичный clean-room OpenUG2.
- Время и память реального ARM64 Linux + GLES2 исполнения на H700 пока неизвестны. Требуется исследовательский стенд, а не обещание запуска.

### Самое полезное для реализации реальной карьеры: GlobalLib

Репозиторий: https://github.com/NFSTools/GlobalLib

`GlobalLib/Support.Underground2/Gameplay/GCareerRace/Functional/Disassemble.cs` содержит описания полей **внутри бинарной записи** (не обязательно адресов всего файла):
- `+0x10..+0x13`: вариант условий открытия (завершить URL/гонки, выбранный спонсор);
- `+0x18..+0x27`: track ID, круги и обратное направление по этапам;
- `+0x30`: `CashValue` — сумма награды **из авторского файла**, вместо нашей временной награды 500;
- `+0x37`: `BelongsToStage`;
- отдельные настройки соперников и требований гонки.

`GCareerStage/Functional/Disassemble.cs` содержит спонсоров текущего этапа, лимиты доступных на карте гонок и `LastStageEvent`.
`Sponsor/Functional/Disassemble.cs` содержит `CashValuePerWin`, `SignCashBonus`, `PotentialCashBonus` и 3 обязательных спонсорских типа гонок.
`GCarUnlock` описывает обязательные победы для открытия автомобиля, `WorldShop` — условия появления магазина, `WorldChallenge` — задания и требования.

Важно: это результаты стороннего reverse engineering. Значения и указатели должны быть защищены bounds-checks, verified identifiers и тестами на PC-файлах, которыми пользователь законно владеет. GlobalLib написан на C#: **не подключать C# runtime на H700**; переносить проверенные спецификации в небольшой чистый C parser либо готовить собственный метадата-каталог на ПК.

### Ближайший практически проверяемый шаг

1. В отдельной ветке создать `career_source_catalog.c/h`, загрузчик авторских данных `GlobalB.lzc` PC, поддерживая сперва только поля race ID, stage, CashValue, unlock conditions и event-type.
2. Входные проверки: размер каждого чанка, границы массивов, допустимые числа, отсутствие дублирующих ID, умение безопасно отбрасывать незнакомые блоки. Синтетические фикстуры и гонка ARM64 CI.
3. Сверить значения с собственным легальным игровым набором. Не добавлять сгенерированные данные из оригинальной игры в GitHub.
4. Передавать `verified_race_id`, `prize`, `race_type` в `Career` при честном окончании гонки. Убрать прототипные 500 кредитов, но **не начислять** очки за соло/отладочный заезд.
5. Следующим шагом — на основе `GCareerStage` и `Sponsor` открыть честный набор этапов/контрактов, затем графика/гараж/магазины.

## Приоритет

**Высокий:** GlobalLib `GCareerRace`/`GCareerStage`/`Sponsor`; Detoy launcher и импортер.

**Средний:** `nfsu2-sw` как независимая эталонная реализация поведения оригинальной игры; yugecin/nfsu2-re для PC-структур.

**Низкий:** OpenUG2-VITA, OpenUG2_360, молодая decomp без matched functions; прямой перенос Xbox renderer в GLES2 на одногигабайтный H700 до измерения затрат.

Дисциплина: не говорить «полная карьера работает», пока нет сохранённого и повторно загруженного прохождения от начальной гонки до Caleb на настоящем H700.
