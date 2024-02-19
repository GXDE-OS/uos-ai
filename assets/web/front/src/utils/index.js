// 请求c++
export const Qrequest = (request, ...args) => {
    return new Promise((resolve, reject) => {
        request(...args, (res) => {
            resolve(res)
        })
    })
}

export const hexToRgb = function (val) {
    //HEX十六进制颜色值转换为RGB(A)颜色值
    // 16进制颜色值的正则
    const reg = /^#([0-9a-fA-f]{3}|[0-9a-fA-f]{6})$/;
    // 把颜色值变成小写
    let color = val.toLowerCase();
    let result = "";
    if (reg.test(color)) {
        // 如果只有三位的值，需变成六位，如：#fff => #ffffff
        if (color.length === 4) {
            let colorNew = "#";
            for (var i = 1; i < 4; i += 1) {
                colorNew += color.slice(i, i + 1).concat(color.slice(i, i + 1));
            }
            color = colorNew;
        }
        // 处理六位的颜色值，转为RGB
        const colorChange = [];
        for (var i = 1; i < 7; i += 2) {
            colorChange.push(parseInt("0x" + color.slice(i, i + 2)));
        }
        result = "rgb(" + colorChange.join(",") + ")";
        return {
            rgb: result,
            rgbStr: colorChange.join(","),
            r: colorChange[0],
            g: colorChange[1],
            b: colorChange[2],
        };
    } else {
        result = "无效";
        return {
            rgb: result
        };
    }
}
