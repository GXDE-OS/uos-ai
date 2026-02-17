# 优先使用Qt6 lupdate工具
if [ -f "/usr/lib/qt6/bin/lupdate" ]; then
    lupdate="/usr/lib/qt6/bin/lupdate" 
elif [ -f "/usr/lib/qt5/bin/lupdate" ]; then
    lupdate="/usr/lib/qt5/bin/lupdate"
else
    lupdate="lupdate"
fi

# 优先使用Qt6 lrelease工具
if [ -f "/usr/lib/qt6/bin/lrelease" ]; then
    lrelease="/usr/lib/qt6/bin/lrelease" 
elif [ -f "/usr/lib/qt5/bin/lrelease" ]; then
    lrelease="/usr/lib/qt5/bin/lrelease"
else
    lrelease="lrelease"
fi

${lupdate} src/ -ts -no-obsolete translations/uos-ai-assistant_zh_CN.ts
${lupdate} plugin-tray/ -ts -no-obsolete translations/uos-ai-tray_zh_CN.ts
${lupdate} plugin-settings/ -ts -no-obsolete translations/uos-ai-settings_zh_CN.ts

desk_ts_list=(`ls translations/desktop/*.ts`)
for ts in "${desk_ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    ${lrelease} "${ts}"
done

ts_list=(`ls translations/*.ts`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    ${lrelease} "${ts}"
done
