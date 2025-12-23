import sys
sys.path.append("build\Debug")
from index import BooleanIndex
import stemmer 

def parse_query(q: str) -> list[str]:
    tokens = q.lower().strip().split()

    if len(tokens) == 1:
        tokens[0] = stemmer.stem(tokens[0])
        return [tokens[0]]

    if len(tokens) == 2 and tokens[0].upper() == "not":
        tokens[1] = stemmer.stem(tokens[1])
        return ["not", tokens[1]]

    if len(tokens) == 3:
        left, op, right = tokens
        left = stemmer.stem(left)
        right = stemmer.stem(right)
        op = op.upper()

        if op in ("and", "or"):
            return [left, op, right]

    raise ValueError("Invalid query format")


def main():
    if len(sys.argv) < 3:
        print("Usage: search.py <index_file> <query>")
        return

    index_file = sys.argv[1]
    query = " ".join(sys.argv[2:])

    index = BooleanIndex()
    index.load(index_file)

    try:
        tokens = parse_query(query)
    except ValueError as e:
        print(e)
        return

    result = index.evaluate_query(tokens)

    if not result:
        print("No documents found")
        return

    print("Document ids:")
    for doc_id in result:
        print(doc_id)

if __name__ == "__main__":
    main()