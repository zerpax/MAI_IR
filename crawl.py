import os
import requests
from time import sleep
from datetime import datetime
from urllib.parse import urljoin, urlparse, urlunparse
from bs4 import BeautifulSoup
import hashlib
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


def html_hash(html: str) -> str:
    return hashlib.sha256(html.encode('utf-8')).hexdigest()


def crawl(config):
    user_agent = "Uni_Assignment_Bot/0.1 (nik_fedorov_2004@mail.ru)"
    headers = {"User-Agent": user_agent}
    collection = db_setup(config)

    visited = set(doc['url'] for doc in collection.find({}, {'url': 1}))
    queue = [config['logic']['start_url']]

    max_depth = config['logic']['max_depth']
    start_id = config['logic']['start_id']
    i = start_id

    while queue and i - start_id < max_depth:
        current_url = queue.pop(0)
        print("loading:", current_url)

        response = requests.get(current_url, headers=headers)
        soup = BeautifulSoup(response.text, 'html.parser')


        if  any(current_url.startswith(prefix) for prefix in config['logic']['parseable_pages'])\
            and "/page/" not in current_url:
            html = response.text
            new_hash = html_hash(html)

            existing_doc = collection.find_one({"url": current_url})

            if existing_doc:
                if existing_doc.get("html_hash") != new_hash:
                    collection.update_one(
                        {"url": current_url},
                        {
                            "$set": {
                                "html": html,
                                "html_hash": new_hash,
                                "date": datetime.utcnow()
                            }
                        }
                    )
                    print("updated")
                else:
                    print("not changed")
            else:
                collection.insert_one({
                    "_id": i,
                    "url": current_url,
                    "html": html,
                    "html_hash": new_hash,
                    "source_name": urlparse(current_url).hostname,
                    "date": datetime.utcnow(),
                })
                i += 1
                print("saved")

        for a in soup.find_all("a", href=True):
            link = urlunparse(urlparse(a['href'])._replace(fragment=''))
            normalized = urljoin(current_url, link)

            if (
                normalized not in visited and
                any(normalized.startswith(prefix) for prefix in config['logic']['accessable_pages'])
            ):
                visited.add(normalized)
                queue.append(normalized)
                config['logic']['start_url'] = normalized
                config['logic']['start_id'] = i + 1

        sleep(config['logic']['delay'])

    return config


def main():
    try:
        with open("config.yaml", "r", encoding="utf-8") as file:
            config = yaml.safe_load(file)

        config = crawl(config)

    except KeyboardInterrupt:
        print("Crawl interrupted")

    finally:
        with open("config.yaml", "w") as file:
            yaml.safe_dump(config, file)


if __name__ == "__main__":
    main()
