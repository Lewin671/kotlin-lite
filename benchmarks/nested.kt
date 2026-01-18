fun main() {
    var i = 0
    var count = 0
    while (i < 40000) {
        var j = 0
        while (j < 40000) {
            count = count + 1
            j = j + 1
        }
        i = i + 1
    }
    print_i32(count)
}
