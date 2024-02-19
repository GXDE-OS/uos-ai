lupdate  src/ -ts -no-obsolete translations/uos-ai-assistant_zh_CN.ts
lupdate  plugin-tray/ -ts -no-obsolete translations/uos-ai-tray_zh_CN.ts
lupdate  plugin-settings/ -ts -no-obsolete translations/uos-ai-settings_zh_CN.ts

desk_ts_list=(`ls translations/desktop/*.ts`)
for ts in "${desk_ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    lrelease "${ts}"
done

ts_list=(`ls translations/*.ts`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    lrelease "${ts}"
done
