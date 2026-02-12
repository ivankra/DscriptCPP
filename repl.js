// Basic REPL script using eval() and dscript's built-ins

var console = {log: println};
var __lastLine;

for (;;) {
  print('> ');  // dscript's print() doesn't append newline

  var __line = readln();
  if (__line == null || (__line == '' && __lastLine == '')) {  // probably EOF
    break;
  }

  __lastLine = __line;

  try {
    var __res = eval(__line);
    if (typeof __res != "undefined") {
      console.log("" + __res);
    }
  } catch (__err) {
    console.log("" + __err);
  }
}
