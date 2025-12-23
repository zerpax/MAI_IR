from bs4 import BeautifulSoup
import numpy as np
from collections import Counter
import matplotlib.pyplot as plt

import sys
sys.path.append("build\Debug")
import tokenizer
import stemmer 
import zipf
from index import BooleanIndex

import yaml

from pymongo import MongoClient

def db_setup(config):
    url = config['db']['url']
    db_name = config['db']['name']
    collection_name = config['db']['collection']
    client = MongoClient(url)
    database = client[db_name]
    collection = database[collection_name]
    return collection



def extract_text(soup) -> str:
    title = ''
    authors = ''
    text = ''
    try:
        title = soup.find('h1', class_ = 'article-title').text.strip()
    except:
        pass

    # print(title)

    try:
        for author in soup.find('div', class_ = 'author').find('div', class_ = 'multiple-authors-wrapper').find_all('a'):
            authors += author
    except:
        authors = soup.find('div', class_ = 'author').find('a').text.strip()
    # print(authors)

    try:
        for paragraph in soup.find_all('p', class_ = "paragraph" ):
            text += paragraph.text.strip()
    except:
        pass
    text = text.strip()
    # print(text)
    
    combined = title + '\n' +  authors + '\n' + text

    return combined



def build_index(config, index):
    collection = db_setup(config)

    cursor = collection.find({}, {
        "_id": 1,
        "html": 1
    })

    doc_count = 0

    for doc in cursor:
        doc_id = doc["_id"]
        html = doc.get("html", "")

        if not html:
            continue

        try:
            soup = BeautifulSoup(html, "html.parser")
            text = extract_text(soup)

            tokens = tokenizer.tokenize(text)
            stems = []
            for token in tokens:
                stems.append(stemmer.stem(token))
            index.add_document(doc_id, stems)
            doc_count += 1

        except Exception as e:
            print(f"Failed to parse document with id = {doc_id}, continuing")
            continue


            
    print(f"Indexed {doc_count} documents.")




def main():
    if len(sys.argv) != 2:
        print("Usage: index.py <index_file>")
        return
    else:
        index_file = sys.argv[1]

    config = {}
    with open("config.yaml", "r", encoding="utf-8") as file:
        config = yaml.safe_load(file)


    index = BooleanIndex()
    build_index(config, index)
    
    index.save(index_file)
    print(f"Saved index into {index_file}")

if __name__ == '__main__':
    main()






















# def parse_file(filename):
#     file = open(filename, 'r', encoding='utf-8')
#     text = file.read()
#     file.close()
#     soup = BeautifulSoup(text, 'html.parser')
#     return extract_text(soup)

# def count_freq(tokens):
#     freq = Counter(tokens)
#     freq = freq.most_common()
#     return freq

# def plot_freq(tokens):
#     freq = count_freq(tokens)
    
#     # Separate ranks and frequencies
#     ranks = np.arange(1, len(freq) + 1)
#     frequencies = np.array([f for _, f in freq])

#     # Zipf prediction: f ~ 1/r (scaled by max frequency)
#     zipf = frequencies[0] / ranks

    
#     plt.figure(figsize=(8,6))
#     plt.loglog(ranks, frequencies, marker=".", linestyle="", label="Corpus")
#     plt.loglog(ranks, zipf, linestyle="--", label="Zipf's law")
#     plt.xlabel("Rank of term")
#     plt.ylabel("Frequency")
#     plt.title("Term frequency distribution with Zipf's law")
#     plt.legend()
#     plt.grid(True, which="both", ls="--")
#     plt.show()


