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

def extract_text_pitchfork(soup) -> str:
    title = ''
    authors = ''
    text = ''
    try:
        title = soup.find('h1').text.strip()
    except:
        pass

    # print(title)

    try:
        for author in soup.find('main').find("span", itemprop="author").find_all('a'):
            authors += " " + author.text

    except:
        pass

    try:
        for paragraph in soup.find("div", attrs={"data-attribute-verso-pattern": "article-body"}).find_all('p'):
            text += paragraph.text.strip()
    except:
        pass
    text = text.strip()
    # print(text)
    
    combined = title + '\n' +  authors + '\n' + text

    return combined


def extract_text_billboard(soup) -> str:
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
        try:
            authors = soup.find('div', class_ = 'author').find('a').text.strip()
        except:
            pass
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
        "html": 1,
        "source_name":1
    })

    doc_count = 0

    for doc in cursor:
        doc_id = doc.get("_id", "")
        html = doc.get("html", "")
        source = doc.get("source_name", "")
        if doc_id % 100 == 0:
            print("processing ",doc_id)
            print(doc_count)
        if not html:
            continue

        # try:
        soup = BeautifulSoup(html, "html.parser")
        if "billboard" in source:
            text = extract_text_billboard(soup)
        elif "pitchfork" in source:
            text = extract_text_pitchfork(soup)
        else:
            continue

        tokens = tokenizer.tokenize(text)
        stems = []
        for token in tokens:
            stems.append(stemmer.stem(token))
        index.add_document(doc_id, stems)
        doc_count += 1

        # except Exception as e:
        #     print(e)
        #     print(f"Failed to parse document with id = {doc_id}, continuing")
        #     continue


            
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



