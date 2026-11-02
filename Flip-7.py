import sys


def should_draw(remaining, hand, score):
    gain = 0
    bust = 0

    for value in range(1, 13):
        count = remaining[value]
        if hand & (1 << value):
            bust += count
        else:
            gain += count * value

    return gain > score * bust


def solve():
    read = sys.stdin.buffer.readline
    line = read()
    if not line:
        return

    n, me = map(int, line.split())

    remaining = list(range(13))
    on_table = [0] * 13
    hands = [0] * n

    deck_size = 78
    finished = 0
    score = 0

    write = sys.stdout.write
    flush = sys.stdout.flush

    while True:
        fields = read().split()
        if not fields or fields[0] == b"-1":
            return

        player = int(fields[0])
        card = 0

        if player == me:
            draw = should_draw(remaining, hands[me], score)
            write("1\n" if draw else "0\n")
            flush()

            if draw:
                line = read()
                if not line:
                    return

                card = int(line)
                if card == -1:
                    return
            else:
                finished += 1

        elif fields[1] == b"0":
            finished += 1

        else:
            card = int(fields[2])

        if card:
            remaining[card] -= 1
            deck_size -= 1
            on_table[card] += 1

            bit = 1 << card
            if hands[player] & bit:
                finished += 1
                if player == me:
                    score = 0
            else:
                hands[player] |= bit
                if player == me:
                    score += card

            if deck_size == 0:
                remaining = [
                    value - on_table[value]
                    for value in range(13)
                ]
                deck_size = sum(remaining)

        if finished == n:
            finished = 0
            hands = [0] * n
            on_table = [0] * 13
            score = 0


if __name__ == "__main__":
    solve()