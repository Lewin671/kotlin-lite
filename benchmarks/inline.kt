fun f1(x: Int): Int { return x + 1 }
fun f2(x: Int): Int { return f1(x) * 2 }
fun f3(x: Int): Int { return f2(x) - 1 }
fun f4(x: Int): Int { return f3(x) + f1(x) }
fun f5(x: Int): Int { return f4(x) + f2(x) }
fun f6(x: Int): Int { return f5(x) - f3(x) }

fun main() {
    var i = 0
    var sum = 0
    while (i < 500000000) {
        sum = sum + f6(i % 10)
        i = i + 1
    }
    print_i32(sum)
}
