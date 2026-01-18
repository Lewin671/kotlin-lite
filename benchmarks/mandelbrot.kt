fun main() {
    var count = 0
    var y = -1500
    while (y < 1500) {
        var x = -2000
        while (x < 1000) {
            var cr = x
            var ci = y
            var zr = 0
            var zi = 0
            var iter = 0
            var escaped = 0
            while (iter < 100) {
                var zr2 = (zr * zr) / 1000
                var zi2 = (zi * zi) / 1000
                if (zr2 + zi2 > 4000) {
                    escaped = 1
                    iter = 100
                } else {
                    zi = (2 * zr * zi) / 1000 + ci
                    zr = zr2 - zi2 + cr
                    iter = iter + 1
                }
            }
            if (escaped == 0) {
                count = count + 1
            }
            x = x + 20
        }
        y = y + 20
    }
    print_i32(count)
}
