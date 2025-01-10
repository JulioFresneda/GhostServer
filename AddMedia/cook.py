import hashlib
import os
import json
from time import sleep

import requests
from pathlib import Path
import subprocess


def get_resolution_category(file_path):
    try:
        file_path = Path(os.getcwd()) / file_path
        file_path = file_path.resolve()
        # Run ffprobe to get the video stream resolution
        result = subprocess.run(
            ["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
             "stream=width,height", "-of", "json", str(file_path)],
            capture_output=True, text=True, check=True
        )
        sleep(0.5)
        metadata = json.loads(result.stdout)
        width = metadata["streams"][0]["width"]
        height = metadata["streams"][0]["height"]

        # Classify resolution based on width and height
        if width >= 3840 and height >= 2160:
            return "4K"
        elif width >= 1920 and height >= 1080:
            return "Full HD"
        elif width >= 1280 and height >= 720:
            return "HD"
        elif width < 1280 and height < 720:
            return "Low Quality"
        else:
            return "Undefined"
    except (subprocess.CalledProcessError, KeyError, IndexError, json.JSONDecodeError):
        return "Undefined"

# Helper function to fetch metadata from OMDb
def fetch_metadata(imdb_id, api_key, base_url, folder_path = None, collection_title = None):
    params = {'i': imdb_id, 'apikey': api_key}
    response = requests.get(base_url, params=params)

    if response.status_code == 200 and response.json().get('Response') == 'True':
        metadata = response.json()

        # Get the cover image URL
        cover_url = metadata.get('Poster')

        # If a cover image URL is available, download and save it
        if folder_path is not None and cover_url and cover_url != "N/A":
            try:
                img_response = requests.get(cover_url, stream=True)
                img_response.raise_for_status()

                # Ensure the folder exists
                os.makedirs(folder_path, exist_ok=True)

                # Define full path with the filename
                if collection_title is None:
                    file_name = f"{hash_id(metadata.get('Title'))}.png"
                else:
                    file_name = hash_id(f"{collection_title}_{metadata.get('Title')}") + ".png"

                file_path = os.path.join(folder_path, file_name)

                # Save the image
                with open(file_path, 'wb') as file:
                    for chunk in img_response.iter_content(1024):
                        file.write(chunk)
                print(f"Image saved successfully as {file_path}")

            except requests.RequestException as e:
                print(f"Error downloading image: {e}")
        else:
            print("No cover image available.")

        return metadata
    else:
        print(f"Error fetching metadata for IMDb ID {imdb_id}")
        return None

# Function to create Collection JSON
def create_collection_json(folder_name, media_dir, metadata, episode_files):
    collection_json = {
        "ID": folder_name,
        "collection_title": metadata.get("Title"),
        "collection_description": metadata.get("Plot"),
        "collection_rating": float(metadata.get("imdbRating", 0)),
        "collection_type": "serie" if metadata.get("Type") == "series" else "movies",
        "genres": metadata.get("Genre", "").split(", "),
        "producer": metadata.get("Director"),
        "numberOfEpisodes": len(episode_files),
        "image_path": f"./{media_dir}/{folder_name}/{folder_name}.png"
    }
    return collection_json


# Function to create Episodes JSON
def create_episodes_json(episodes_metadata, episode_files, collection_title):
    episodes = []
    for metadata, file in zip(episodes_metadata, sorted(episode_files)):
        resolution = get_resolution_category(file)
        episode_json = {
            "ID": hash_id(f"{collection_title}_{metadata.get('Title')}"),
            "title": f"{metadata.get('Title')}",
            "year": int(metadata.get("Year").split("-")[0]),
            "description": metadata.get("Plot"),
            "producer": metadata.get("Director"),
            "rating": float(metadata.get("imdbRating", 0)),
            "genres": metadata.get("Genre", "").split(", "),
            "season": metadata.get("Season", ""), # Calculate season based on index
            "episode": metadata.get("Episode", ""),
            "type":"episode",
            "resolution": resolution,
            "media_filepath": str(Path(os.getcwd()) / file)

        }
        episodes.append(episode_json)
    return episodes

def hash_id(input_string):
    """Generate a unique hash for IDs"""
    return hashlib.sha1(input_string.encode()).hexdigest()
# Main function to process media directory
def process_media_directory(covers_folder, media_folder, media_info, api_key, base_url):
        if media_info["type"] == "standalone_movie":
            imdb_url = media_info.get("url")
            if imdb_url:
                imdb_id = imdb_url.split("/")[-1] if imdb_url[-1] != '/' else imdb_url.split("/")[-2]
                metadata = fetch_metadata(imdb_id, api_key, base_url, covers_folder)

                if metadata:
                    resolution = get_resolution_category(media_info["filepath"])
                    movie_json = {
                        "ID": hash_id(metadata.get("Title")),
                        "title": metadata.get("Title"),
                        "year": int(metadata.get("Year")),
                        "description": metadata.get("Plot"),
                        "producer": metadata.get("Director"),
                        "rating": float(metadata.get("imdbRating", 0)),
                        "genres": metadata.get("Genre", "").split(", "),
                        "season": None,
                        "episode": None,
                        "type": "movie",
                        "resolution": resolution,
                        "media_filepath": str(media_info["filepath"])
                    }
                    return movie_json, None

        elif media_info["type"] == "serie":
            imdb_url = media_info['collection_url']

            imdb_id = imdb_url.split("/")[-1] if imdb_url[-1] != '/' else imdb_url.split("/")[-2]
            metadata = fetch_metadata(imdb_id, api_key, base_url, covers_folder)


            media_files = sorted(media_info["filepath"].glob("*.*"))


            collection_json = create_collection_json(media_info["filepath"], media_folder, metadata, media_files)

            episodes_metadata = []
            for episode_url in media_info['episodes_url']:
                imdb_id = episode_url.split("/")[-1] if episode_url[-1] != '/' else episode_url.split("/")[-2]
                episodes_metadata.append(fetch_metadata(imdb_id, api_key, base_url, covers_folder, metadata.get('Title')))

            episodes_json = create_episodes_json(episodes_metadata,
                                                 [m for m in media_files if '.json' not in m.suffix],
                                                 metadata.get("Title"))

            return collection_json, episodes_json

        else:
            movies_json = []
            collection_rating = 0
            for movie_filename, imdb_url in media_info["movies"].items():

                imdb_id = imdb_url.split("/")[-1] if imdb_url[-1] != '/' else imdb_url.split("/")[-2]
                metadata = fetch_metadata(imdb_id, api_key, base_url, covers_folder)

                filepath = os.path.join(media_info["filepath"], movie_filename)
                resolution = get_resolution_category(filepath)

                movie_json = {
                    "ID": hash_id(metadata.get("Title")),
                    "title": metadata.get("Title"),
                    "year": int(metadata.get("Year")),
                    "description": metadata.get("Plot"),
                    "producer": metadata.get("Director"),
                    "rating": float(metadata.get("imdbRating", 0)),
                    "genres": metadata.get("Genre", "").split(", "),
                    "season": None,
                    "episode": None,
                    "type": "movie",
                    "resolution": resolution,
                    "media_filepath": str(filepath)
                }

                collection_rating += movie_json['rating']
                movies_json.append(movie_json)

            collection_rating /= len(media_info["movies"])
            media_info["rating"] = collection_rating

            return media_info, movies_json




