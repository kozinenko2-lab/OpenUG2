# H700: реализация на основе R36S PortMaster и NFSTools/GlobalLib

## Что действительно перенесено в код

### 1. R36S PortMaster — запуск и устройство пакета

Исходный код/идея: https://github.com/Detoy/OpenUG2
(PortMaster R36S, MIT OpenUG2 contributor license).

Файл: `portmaster/OpenUG2.sh`.

- Поиск PortMaster control.txt в `/opt/system/Tools/PortMaster`,
  `/opt/tools/PortMaster`, `$XDG_DATA_HOME/PortMaster`,
  `/roms/ports/PortMaster` и `/roms2/ports/PortMaster`.
- Загрузка модификации настроек `mod_$CFW_NAME.txt` и `get_controls`.
- Экспорт SDL2 controller mapping из `sdl_controllerconfig`.
- При доступности вызывает `pm_platform_helper` и `pm_finish`.
- Ищет по `DEVICE_ARCH` бинарник `nfsu2.aarch64`, либо `nfsu2`.
- Загрузка архитектурных библиотек `libs.aarch64` и общих `libs`.
- Переносимая структура `ports/OpenUG2.sh` + `ports/openug2/` в дополнение
  к H700 `/mnt/sdcard/ports/`. Логи и сохранения строго в папке игры.
- Клавиатурная обёртка GPTOKEYB **не включается автоматически**:
  у нас уже есть SDL_GameController, а GPTOKEYB мог бы дублировать ввод.
- Сохранены `--resolution`, `--world-radius`, `--texture-cache-mb`,
  `--career-save`, 2D HUD и GLES2 профиль качества. Разрешение задаётся
  через лаунчер, а не зашито в движок.
- Добавлены `portmaster/port.json`, `portmaster/gameinfo.xml`.
- `tools/portmaster_mock_test.sh` проверяет запуск с фиктивной прошивкой,
  контроллером и бинарником ARM64, *без ресурсов EA и настоящего GPU*.

**Ограничение:** R36S и RG40XX H обе используют ARM Linux и могут
использовать GLES2, но прошивка, sysroot и версии glibc/SDL2 не одинаковы.
Код скомпилированный для Ubuntu ARM64 не автоматически будет работать на
H700. Использовать H700-sysroot и проверять устройство необходимо.

### 2. Безопасный импортер PC-ресурсов из форка R36S

Файлы: `tools/import_nfsu2_data.py`,
`tools/import_nfsu2_data_test.py`, адаптированы из
https://github.com/Detoy/OpenUG2/tree/main/tools с сохранением
атрибуции MIT. Команда на ПК:

```bash
python3 tools/import_nfsu2_data.py --source "/path/to/NFS Underground 2" --output "/path/to/sdcard/ports/openug2/game"
```

Копирует нужные директории `TRACKS`, `CARS`, `GLOBAL` и необязательные
`FRONTEND`, `SOUND` и т.д., но не `SPEED2.EXE`. Не пишет ничего в
оригинальную установку. Отказывается работать с опасными путями и
символическими ссылками в исходных/целевых каталогах. Данные EA остаются
локальными и не входят в GitHub/пакет.

### 3. GlobalLib: двоичный C-декодер карьерных **записей**

Исследовательский первоисточник (MIT):
https://github.com/NFSTools/GlobalLib
и `GlobalLib/Support.Underground2/Gameplay`.

Новые файлы:
- `src/career_source_catalog.h/.c` — безопасный little-endian разбор
  отдельных извлечённых `GCareerRace`, `GCareerStage`, `Sponsor`.
- `tools/career_source_catalog_test.c` — синтетические записи, проверка
  границ буфера, отсутствия NUL, отрицательной награды, допустимого числа
  соперников и сохранения `out` неизменным при ошибке.

По `GCareerRace` читаются raw ID/trigger, `CashValue` (+0x30),
`BelongsToStage` (+0x37), количество соперников (+0x7C),
`UnlockMethod` (+0x0C), условия из +0x10...+0x13,
ID маршрутов/количество кругов из +0x18...+0x27.
`GCareerStage` содержит рекламодателей этапа, лимиты отображаемых
гонок и последнее обязательное событие; `Sponsor` — бонусы, выплату за
победу и требования по трём типам гонок.

Также введён двухпроходный итератор `career_source_iterate_main_block()`
для **уже найденного и извлечённого блока CareerManager**:
верхний ID `0x80034A10`, вложенные секции строк `0x00034A1D`,
гонок `0x00034A11` (шаг 0x88), этапов `0x00034A18` (шаг 0x50)
и спонсоров `0x00034A19` (шаг 0x10). Неизвестные секции
пропускаются с проверкой длины. Перед вызовом обработчика каждая
поддерживаемая запись проверяется отдельно, чтобы повреждённая запись
не могла частично увеличить счётчик побед.

**Что этот код пока НЕ делает:**
- не распаковывает `GlobalB.lzc` и не находит автоматически блок
  `CareerManager` внутри полного архива;
- не сопоставляет hashed ID с настоящими именами гонок и не определяет тип
  гонки только по ещё не проверенным enum значениями;
- не использует декодированные данные для начисления денег — пока основной
  движок награждает только подтверждённую legacy circuit победу
  прототипными 500 кредитами;
- не доказывает работоспособность карьеры до Caleb.

Это ограничение намеренное: нельзя гадать на произвольных байтах архива и
случайно раздавать деньги за несвязанные события.

## Следующий технический шаг

1. Получить структуру расположения `CareerManager` внутри
   `GlobalB.lzc` по GlobalLib (`LoadGlobalB.cs` и модуль `CareerManager`).
2. Добавить бинарный безопасный распаковщик/итератор секций с защитой от
   неправильных длин, индексов, циклов и разыменования смещений вне файла.
3. Прочитать metadata-каталог на легальной копии PC NFSU2 и сверить
   `CashValue`, `BelongsToStage`, unlock и реальные race ID.
4. Подключить именно подтверждённый event ID/type/payout к
   `career_record_win` и `career_save`; потом карта, магазины, спонсоры.
5. На настоящем RG40XX H проверить PortMaster-запуск, версию glibc,
   Mali GLES2, ввод, звук, пик RSS, смену районов и серию гонок.

Приёмочные тесты сейчас: `bash tools/portmaster_mock_test.sh`,
`python3 tools/import_nfsu2_data_test.py`,
`make career-source-test`, `make career-test` и GLES2 ARM64 workflow.

### Лицензии и самостоятельность

OpenUG2 и Detoy/OpenUG2 распространяются по MIT. GlobalLib также MIT.
Спецификацию форматов изучили по открытым полям, C-декодер написан отдельно;
чужой C# runtime не интегрирован. Никаких файлов PC/Xbox игры в проекте нет.
