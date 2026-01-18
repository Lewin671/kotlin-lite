fun main() {
    var i = 0
    var sum = 0
    while (i < 2000000000) {
        sum = sum + 1
        i = i + 1
    }
    print_i32(sum)
}
