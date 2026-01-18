fun is_prime(n: Int): Int {
    if (n <= 1) return 0
    var i = 2
    while (i * i <= n) {
        if (n % i == 0) return 0
        i = i + 1
    }
    return 1
}

fun main() {
    var count = 0
    var n = 2
    while (n < 500000) {
        if (is_prime(n) == 1) {
            count = count + 1
        }
        n = n + 1
    }
    print_i32(count)
}
