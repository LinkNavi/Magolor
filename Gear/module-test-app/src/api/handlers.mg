using Std.IO;
using core.types;
using core.ops;
using utils.helpers;

fn handleRequest(x: int, y: int) -> int {
    Std.print("api.handlers: handling request\n");
    let data = processData(x, y);
    let point = makePoint(x, y);
    return data + point;
}
