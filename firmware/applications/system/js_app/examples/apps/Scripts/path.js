let storage = require("storage");

print("__dirname скрипта: " + __dirname);
print("__filename скрипта: " + __filename);
if (storage.fileExists(__dirname + "/math.js")) {
    print("math.js здесь есть.");
} else {
    print("math.js здесь нет.");
}
