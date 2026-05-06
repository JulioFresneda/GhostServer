import sqlite3
import json
import hashlib
import os
from pathlib import Path
import logging

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class DatabaseError(Exception):
    """Custom exception for database operations"""
    pass

def hash_id(input_string):
    """Generate a unique hash for IDs"""
    return hashlib.sha1(input_string.encode()).hexdigest()

def validate_collection_data(collection_data):
    """Validate required collection fields"""
    required_fields = ["collection_title"]
    missing_fields = [field for field in required_fields if not collection_data.get(field)]
    if missing_fields:
        raise ValueError(f"Missing required fields: {', '.join(missing_fields)}")

def insert_collection(cursor, collection_data):
    try:
        # Validate input data
        validate_collection_data(collection_data)
        
        # Generate a unique ID if not present
        collection_data["ID"] = collection_data.get("ID", hash_id(collection_data["collection_title"]))

        # Insert collection data
        cursor.execute('''
        INSERT OR REPLACE INTO Collection (ID, collection_title, collection_description, collection_rating, collection_type, genres, producer, image_path)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            collection_data["ID"],
            collection_data.get("collection_title"),
            collection_data.get("collection_description"),
            collection_data.get("collection_rating"),
            collection_data.get("collection_type"),
            json.dumps(collection_data.get("genres", [])),  # Encode list as JSON
            collection_data.get("producer"),
            collection_data.get("image_path")
        ))
        
        logger.info(f"Successfully inserted collection: {collection_data['collection_title']}")
        return collection_data["ID"]  # Return collection ID for future reference
        
    except sqlite3.Error as e:
        logger.error(f"Database error while inserting collection: {e}")
        raise DatabaseError(f"Failed to insert collection: {e}")
    except Exception as e:
        logger.error(f"Unexpected error while inserting collection: {e}")
        raise

def insert_episodes(cursor, episodes_data, collection_title, collection_id):
    """Insert each episode in the Media table for a series collection."""
    try:
        for i, episode_data in enumerate(episodes_data, start=1):
            episode_title = episode_data.get("title", f"Episode {i}")
            episode_id = episode_data.get("ID", hash_id(f"{collection_title}_{episode_title}"))

            cursor.execute('''
            INSERT OR REPLACE INTO Media (ID, title, year, description, producer, rating, season, episode, resolution, image_path, type, collection_id)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (
                episode_id,
                episode_title,
                episode_data.get("year"),
                episode_data.get("description"),
                episode_data.get("producer"),
                episode_data.get("rating"),
                episode_data.get("season"),
                episode_data.get("episode", i),
                episode_data.get("resolution", 'Undefined'),
                episode_data.get("image_path"),
                "episode",
                collection_id
            ))
            logger.info(f"Successfully inserted episode: {episode_title}")
            
    except sqlite3.Error as e:
        logger.error(f"Database error while inserting episodes: {e}")
        raise DatabaseError(f"Failed to insert episodes: {e}")
    except Exception as e:
        logger.error(f"Unexpected error while inserting episodes: {e}")
        raise

def insert_movie(cursor, movie_data):
    """Insert movie data as a single entry in the Media table."""
    try:
        if not movie_data.get("title"):
            raise ValueError("Movie title is required")
            
        movie_id = movie_data.get("ID", hash_id(movie_data["title"]))

        cursor.execute('''
        INSERT OR REPLACE INTO Media (ID, title, year, description, producer, rating, season, episode, resolution, image_path, type, collection_id, genres)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            movie_id,
            movie_data["title"],
            movie_data.get("year"),
            movie_data.get("description"),
            movie_data.get("producer"),
            movie_data.get("rating"),
            None,  # No season for movies
            None,  # No episode for movies
            movie_data.get("resolution", "Undefined"),
            movie_data.get("image_path"),
            "movie",
            None,  # No collection ID for standalone movies,
            json.dumps(movie_data.get("genres", []))
        ))
        logger.info(f"Successfully inserted movie: {movie_data['title']}")
        
    except sqlite3.Error as e:
        logger.error(f"Database error while inserting movie: {e}")
        raise DatabaseError(f"Failed to insert movie: {e}")
    except Exception as e:
        logger.error(f"Unexpected error while inserting movie: {e}")
        raise

def process_data(db_path, collection, items):
    """Process each collection and its episodes or movie data from JSON files."""
    # Connect to the database using context manager
    try:
        with sqlite3.connect(db_path) as conn:
            cursor = conn.cursor()
            if items != None:
                # Insert collection and get ID
                collection_id = insert_collection(cursor, collection)
                insert_episodes(cursor, items, collection["collection_title"], collection_id)

            else:
                insert_movie(cursor, collection)
                logger.info(f"Inserted movie: {collection.get('title')}")


            conn.commit()
            logger.info("Database updated successfully with all collections, episodes, and movies.")
            
    except sqlite3.Error as e:
        logger.error(f"Database error: {e}")
        raise DatabaseError(f"Database operation failed: {e}")
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        raise


