import sys


def solve():
    stream = sys.stdin.buffer
    stream.readline()

    largest = second = 0
    pending = b""

    while True:
        block = stream.read(65536)
        tokens = (pending + block).split()

        if block and block[-1] > 32:
            pending = tokens.pop()
        else:
            pending = b""

        for value in map(int, tokens):
            if value > largest:
                second = largest
                largest = value
            elif value > second:
                second = value

        if not block:
            break

    print(max(1, largest // 2, second - 1))


if __name__ == "__main__":
    solve()