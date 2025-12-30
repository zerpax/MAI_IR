import sys
import json
import yaml
from pymongo import MongoClient

sys.path.append("build\\Debug")
from index import BooleanIndex
import stemmer



def db_setup(config):
    url = config['db']['url']
    db_name = config['db']['name']
    collection_name = config['db']['collection']
    client = MongoClient(url)
    database = client[db_name]
    collection = database[collection_name]
    return collection



def parse_query(q: str) -> list[str]:
    tokens = q.lower().strip().split()
    print(tokens)
    if len(tokens) == 1:
        return [stemmer.stem(tokens[0])]

    if len(tokens) == 2 and tokens[0].upper() == "not":
        return ["NOT", stemmer.stem(tokens[1])]

    if len(tokens) == 3:
        left, op, right = tokens

        if op in ("and", "or"):
            return [
                stemmer.stem(left),
                op,
                stemmer.stem(right)
            ]

    raise ValueError("Invalid query format")


def make_snippet(text: str, terms: list[str], size: int = 160) -> str:
    text = text.lower()
    for term in terms:
        pos = text.find(term)
        if pos != -1:
            start = max(0, pos - 40)
            end = pos + size
            return text[start:end].replace("\n", " ")
    return text[:size].replace("\n", " ")


def main():
    if len(sys.argv) < 3:
        print("Usage: search.py <index_file> <query>")
        return

    index_file = sys.argv[1]
    query = " ".join(sys.argv[2:])

    with open("config.yaml", "r", encoding="utf-8") as file:
        config = yaml.safe_load(file)

    collection = None
    try:
        collection = db_setup(config)
        collection.find_one()
        mongo_available = True
    except Exception as e:
        print("MongoDB not available, falling back to IDs only.")
        mongo_available = False

    index = BooleanIndex()
    index.load(index_file)

    try:
        tokens = parse_query(query)
    except ValueError as e:
        print(e)
        return

    doc_ids = index.evaluate_query(tokens)

    if not doc_ids:
        print("No documents found")
        return

    terms = [t for t in tokens if t not in ("AND", "OR", "NOT")]
    
    if mongo_available:
        documents = list(
            collection.find(
                {"_id": {"$in": list(doc_ids)}},
                {"html": 1, "url": 1, "source_name": 1, "date": 1}
            )
        )

        results = []
        for doc in documents:
            html_text = doc.get("html", "")
            results.append({
                "id": str(doc["_id"]),
                "url": doc.get("url"),
                "source": doc.get("source_name"),
            })

        output = {
            "query": query,
            "total": len(results),
            "results": results
        }

        print(json.dumps(output, indent=2, ensure_ascii=False))
    else:
        print("Document IDs:", doc_ids)


if __name__ == "__main__":
    main()
