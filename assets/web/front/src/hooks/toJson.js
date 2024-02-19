export default function (str) {
  if(!isNaN(Number(str))) return str
  if (typeof str == "string") {
    try {
      const data = JSON.parse(str);
      return data;
    } catch (e) {
      return str;
    }
  }
  return str
}
