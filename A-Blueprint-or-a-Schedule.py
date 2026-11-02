import sys
from array import array


def solve():
    read = sys.stdin.buffer.readline
    n, m, k = map(int, read().split())

    if k == 1:
        sys.stdout.write("TREE\n1\n")
        return

    graph = [array("H") for _ in range(n)]
    reverse = [array("H") for _ in range(n)]
    indegree = [0] * n

    for _ in range(m):
        u, v = map(int, read().split())
        u -= 1
        v -= 1
        graph[u].append(v)
        reverse[v].append(u)
        indegree[v] += 1

    tree = [[] for _ in range(k)]

    for _ in range(k - 1):
        a, b = map(int, read().split())
        a -= 1
        b -= 1
        tree[a].append((b, True))
        tree[b].append((a, False))

    degree = [len(neighbors) for neighbors in tree]
    leaves = [u for u in range(k) if degree[u] == 1]
    parent = [-1] * k

    topo = [u for u in range(n) if indegree[u] == 0]

    for u in topo:
        for v in graph[u]:
            indegree[v] -= 1
            if indegree[v] == 0:
                topo.append(v)

    previous = [-1] * n
    following = [-1] * n

    for i in range(n - 1):
        u, v = topo[i], topo[i + 1]
        following[u] = v
        previous[v] = u

    head, tail = topo[0], topo[-1]

    colors = [0] * n
    blocked = [0] * n
    leaf_color = [0] * k
    color = 0

    for index in range(k - 1):
        leaf = leaves[index]

        for p, leaf_to_parent in tree[leaf]:
            if degree[p]:
                break

        parent[leaf] = p
        degree[leaf] = 0
        degree[p] -= 1

        if degree[p] == 1:
            leaves.append(p)

        color += 1

        if not leaf_to_parent:
            u = tail
            step = previous
            neighbors = reverse
        else:
            u = head
            step = following
            neighbors = graph

        while u != -1:
            nxt = step[u]

            if blocked[u] != color:
                colors[u] = color
                left, right = previous[u], following[u]

                if left == -1:
                    head = right
                else:
                    following[left] = right

                if right == -1:
                    tail = left
                else:
                    previous[right] = left

                for v in neighbors[u]:
                    blocked[v] = color

            u = nxt

        leaf_color[leaf] = color

        if head == -1:
            sys.stdout.write(
                "COLOR\n" + " ".join(map(str, colors)) + "\n"
            )
            return

    image = [0] * k
    image[p] = head
    order = [p]

    outgoing_color = [0] * k
    incoming_color = [0] * k

    for u in order:
        p = image[u]

        for v in graph[p]:
            outgoing_color[colors[v]] = v

        for v in reverse[p]:
            incoming_color[colors[v]] = v

        for v, forward in tree[u]:
            if v != parent[u]:
                lookup = outgoing_color if forward else incoming_color
                image[v] = lookup[leaf_color[v]]
                order.append(v)

    sys.stdout.write(
        "TREE\n" + " ".join(str(v + 1) for v in image) + "\n"
    )


if __name__ == "__main__":
    solve()