import os
import shutil
import sqlite3
import json
import hashlib


def create_tables():
    # Connect to SQLite database (or create it if it doesn't exist)
    conn = sqlite3.connect("ghost.db")
    cursor = conn.cursor()

    # Create Media table with collection_id as a foreign key
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS Media (
        ID TEXT PRIMARY KEY,
        title TEXT NOT NULL,
        year TEXT,
        description TEXT,
        producer TEXT,
        rating REAL,
        season INTEGER,
        episode INTEGER,
        resolution TEXT CHECK(resolution IN ('HD', 'Full HD', '4K', 'Low Quality', 'Undefined')) NOT NULL,
        image_path TEXT,
        genres TEXT,  -- JSON-encoded list of genres
        type TEXT CHECK(type IN ('movie', 'episode')) NOT NULL,
        collection_id TEXT,  -- Foreign key linking to Collection
        FOREIGN KEY (collection_id) REFERENCES Collection(ID)
    )
    ''')

    # Create Collection table with genres and collection_type
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS Collection (
        ID TEXT PRIMARY KEY,
        collection_title TEXT NOT NULL,
        collection_description TEXT,
        collection_rating REAL,
        collection_type TEXT CHECK(collection_type IN ('movies', 'serie')) NOT NULL,
        genres TEXT,  -- JSON-encoded list of genres
        producer TEXT,
        image_path TEXT
    )
    ''')



    # Create User table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS User (
        ID TEXT PRIMARY KEY,
        token TEXT NOT NULL
            )
    ''')

    # Create Profile table
    # Create Profile table with a composite unique constraint on (userID, profileID)
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS Profile (
            profileID TEXT,       -- nickname as profile identifier
            userID TEXT,          -- user ID associated with the profile
            pictureID TEXT,       -- ID of the profile picture
            FOREIGN KEY (userID) REFERENCES User(ID),
            UNIQUE (userID, profileID)  -- Enforces unique profile names within the same user
        )
    ''')

    # Create mediaMetadata table with a composite primary key on (userID, profileID, mediaID)
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS mediaMetadata (
            userID TEXT,                -- User ID associated with the profile
            profileID TEXT,             -- Profile identifier
            mediaID TEXT,               -- Media identifier
            percentage_watched REAL,    -- Percentage of media watched
            language_chosen TEXT,       -- Language preference
            subtitles_chosen TEXT,      -- Subtitles preference
            PRIMARY KEY (userID, profileID, mediaID),   -- Composite primary key
            FOREIGN KEY (userID, profileID) REFERENCES Profile(userID, profileID),
            FOREIGN KEY (mediaID) REFERENCES Media(ID)
        )
    ''')

    # Commit changes and close the connection
    conn.commit()
    conn.close()

def hash_id(input_string):
    """Generate a unique hash for IDs"""
    return hashlib.sha1(input_string.encode()).hexdigest()

# Run the table creation function
create_tables()

source = "ghost.db"
destination = os.path.join("..", "ghost.db")

# Move and replace the file in the parent folder
shutil.move(source, destination)