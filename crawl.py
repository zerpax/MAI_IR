import os
import requests
from time import sleep 
from datetime import datetime 
from urllib.parse import urljoin, urlparse, urlunparse
from bs4 import BeautifulSoup

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





def crawl(config):
    user_agent = "Uni_Assignment_Bot/0.1 (nik_fedorov_2004@mail.ru)"
    headers = {"User-Agent" : user_agent} 
    collection = db_setup(config)
    

    visited = set(doc['url'] for doc in collection.find({}, {'url':1}))
    print("visited: ", visited)
    queue = [config['logic']['start_url']]

    max_depth = config['logic']['max_depth']
    
    start_id = config['logic']['start_id']
    i = start_id
    while queue and i - start_id < max_depth:    
        current_url = queue.pop(0)
        print("loading: ", current_url)
        visited.add(current_url)
        

        response = requests.get(current_url, headers=headers)
        soup = BeautifulSoup(response.text, 'html.parser')
        try:
            collection.insert_one({
                "_id": i,
                "url": current_url,
                "html": response.text,
                "source_name": urlparse(current_url).hostname,
                "date": datetime.utcnow(),
            })
            i += 1
        except Exception:
            print(f"Failed to add {current_url}, continuing")
            continue
        for a in soup.find_all("a", href = True):
            link = urlunparse(urlparse(a['href'])._replace(fragment = '')) #removing anchor

            normalized = urljoin(current_url, link)
            if normalized not in visited and any([normalized.startswith(prefix) for prefix in config['logic']['parseable_pages']]):
                queue.append(normalized)
                config['logic']['start_url'] = normalized
                config['logic']['start_id'] = i
                
        print("done")
        sleep(config['logic']['delay'])
    return config


def main():
    try:
        config = {}
        with open("config.yaml", "r", encoding="utf-8") as file:
            config = yaml.safe_load(file)

        print(config)
        config = crawl(config)

    except (KeyboardInterrupt):
        print("Crawl interrupteds")
    finally:
        with open("config.yaml", "w") as file:
            yaml.safe_dump(config, file)

if __name__ == "__main__":
    main()