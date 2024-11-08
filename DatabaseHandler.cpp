#include "DatabaseHandler.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <filesystem>
namespace fs = std::filesystem;


DatabaseHandler::DatabaseHandler(const std::string& dbPath) {
    try {
        db.open(dbPath.c_str());
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to open database: " + std::string(e.errorMessage()));
    }
}

DatabaseHandler::~DatabaseHandler() {
    db.close();
}

std::string DatabaseHandler::getToken(const std::string& userID) {
    try {
        // Query all profiles associated with the given userID
        CppSQLite3Statement stmt = db.compileStatement("SELECT token FROM User WHERE ID = ?;");
        stmt.bind(1, userID.c_str());
        CppSQLite3Query query = stmt.execQuery();

        if (!query.eof()) {
            return query.getStringField("token");
        }
        else {
            throw std::runtime_error("Token not found with user ID: " + userID);
        }
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to fetch profiles: " + std::string(e.errorMessage()));
    }
}

std::vector<std::pair<std::string, std::string>> DatabaseHandler::getAllUserTokens() {
    std::vector<std::pair<std::string, std::string>> tokens;

    try {
        CppSQLite3Query query = db.execQuery("SELECT ID, token FROM User;");
        while (!query.eof()) {
            std::string userID = query.getStringField("ID");
            std::string token = query.getStringField("token");
            tokens.emplace_back(userID, token);
            query.nextRow();
        }
    }
    catch (const CppSQLite3Exception& e) {
        std::cerr << "Failed to load user tokens: " << e.errorMessage() << std::endl;
    }

    return tokens;
}



bool DatabaseHandler::deleteProfile(const std::string& userID, const std::string& profileID) {
    try {
        // Delete profile from Profile table based on both userID and profileID
        CppSQLite3Statement stmt = db.compileStatement("DELETE FROM Profile WHERE userID = ? AND profileID = ?;");
        stmt.bind(1, userID.c_str());
        stmt.bind(2, profileID.c_str());
        int rowsAffected = stmt.execDML();

        return rowsAffected > 0;  // True if a row was deleted
    }
    catch (const CppSQLite3Exception& e) {
        std::cerr << "Failed to delete profile: " << e.errorMessage() << std::endl;
        return false;
    }
}


void DatabaseHandler::getProfiles(const std::string& userID, std::vector<std::pair<std::string, std::string>>& profiles) {
    try {
        // Query all profiles associated with the given userID
        CppSQLite3Statement stmt = db.compileStatement("SELECT profileID, pictureID FROM Profile WHERE userID = ?;");
        stmt.bind(1, userID.c_str());
        CppSQLite3Query query = stmt.execQuery();

        // Populate the profiles vector with pairs of profile ID and picture ID
        while (!query.eof()) {
            std::string profileID = query.getStringField("profileID");
            std::string pictureID = query.getStringField("pictureID");
            profiles.emplace_back(profileID, pictureID);
            query.nextRow();
        }
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to fetch profiles: " + std::string(e.errorMessage()));
    }
}



bool DatabaseHandler::addProfile(const std::string& userID, const std::string& profileID, const std::string& pictureID) {
    try {
        // Check if the user exists before adding a profile
        CppSQLite3Statement userCheckStmt = db.compileStatement("SELECT COUNT(*) FROM User WHERE ID = ?;");
        userCheckStmt.bind(1, userID.c_str());
        CppSQLite3Query userCheckQuery = userCheckStmt.execQuery();

        if (userCheckQuery.eof() || userCheckQuery.getIntField(0) == 0) {
            std::cerr << "User does not exist with ID: " << userID << std::endl;
            return false;
        }

        // Insert profile into Profile table
        CppSQLite3Statement stmt = db.compileStatement("INSERT INTO Profile (profileID, userID, pictureID) VALUES (?, ?, ?);");
        stmt.bind(1, profileID.c_str());
        stmt.bind(2, userID.c_str());
        stmt.bind(3, pictureID.c_str());
        stmt.execDML();

        return true;
    }
    catch (const CppSQLite3Exception& e) {
        std::cerr << "Failed to add profile: " << e.errorMessage() << std::endl;
        return false;
    }
}























std::vector<std::string> DatabaseHandler::getAllCollections() {
    std::vector<std::string> collections;
    try {
        CppSQLite3Query query = db.execQuery("SELECT collection_title FROM Collection;");
        while (!query.eof()) {
            collections.push_back(query.getStringField("collection_title"));
            query.nextRow();
        }
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to fetch collections: " + std::string(e.errorMessage()));
    }
    return collections;
}

std::string DatabaseHandler::getCollectionById(const std::string& collectionId) {
    try {
        CppSQLite3Statement stmt = db.compileStatement("SELECT collection_title FROM Collection WHERE ID = ?;");
        stmt.bind(1, collectionId.c_str());
        CppSQLite3Query query = stmt.execQuery();

        if (!query.eof()) {
            return query.getStringField("collection_title");
        }
        else {
            throw std::runtime_error("Collection not found with ID: " + collectionId);
        }
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to fetch collection by ID: " + std::string(e.errorMessage()));
    }
}

std::vector<std::string> DatabaseHandler::getAllMediaIdsFromCollection(const std::string& collectionId) {
    std::vector<std::string> mediaIds;
    try {
        CppSQLite3Statement stmt = db.compileStatement("SELECT medias FROM Collection WHERE ID = ?;");
        stmt.bind(1, collectionId.c_str());
        CppSQLite3Query query = stmt.execQuery();

        if (!query.eof()) {
            std::string mediaIdsJson = query.getStringField("medias");

            // Parse JSON manually, assuming it's a comma-separated list within square brackets
            std::string id;
            for (char c : mediaIdsJson) {
                if (c == ',' || c == ']' || c == '[') {
                    if (!id.empty()) {
                        mediaIds.push_back(id);
                        id.clear();
                    }
                }
                else if (c != ' ' && c != '"') {
                    id += c;
                }
            }
            if (!id.empty()) mediaIds.push_back(id);
        }
        else {
            throw std::runtime_error("Collection not found with ID: " + collectionId);
        }
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to fetch media IDs from collection: " + std::string(e.errorMessage()));
    }
    return mediaIds;
}

std::string DatabaseHandler::getMediaDataById(const std::string& mediaId) {
    try {
        CppSQLite3Statement stmt = db.compileStatement("SELECT title, description, producer, rating FROM Media WHERE ID = ?;");
        stmt.bind(1, mediaId.c_str());
        CppSQLite3Query query = stmt.execQuery();

        if (!query.eof()) {
            std::string title = query.getStringField("title");
            std::string description = query.getStringField("description");
            std::string producer = query.getStringField("producer");
            double rating = query.getFloatField("rating");

            return "Title: " + title + "\nDescription: " + description + "\nProducer: " + producer + "\nRating: " + std::to_string(rating);
        }
        else {
            throw std::runtime_error("Media not found with ID: " + mediaId);
        }
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to fetch media data by ID: " + std::string(e.errorMessage()));
    }
}

std::vector<std::string> DatabaseHandler::getAllMediaMetadataByMediaId(const std::string& mediaId) {
    std::vector<std::string> metadataEntries;
    try {
        CppSQLite3Statement stmt = db.compileStatement("SELECT * FROM mediaMetadata WHERE mediaID = ?;");
        stmt.bind(1, mediaId.c_str());
        CppSQLite3Query query = stmt.execQuery();

        while (!query.eof()) {
            std::string userId = query.getStringField("userID");
            double percentageWatched = query.getFloatField("percentage_watched");
            std::string language = query.getStringField("language_chosen");
            std::string subtitles = query.getStringField("subtitles_chosen");

            metadataEntries.push_back("User ID: " + userId +
                ", Percentage Watched: " + std::to_string(percentageWatched) +
                ", Language: " + language +
                ", Subtitles: " + subtitles);
            query.nextRow();
        }
    }
    catch (const CppSQLite3Exception& e) {
        throw std::runtime_error("Failed to fetch media metadata by media ID: " + std::string(e.errorMessage()));
    }
    return metadataEntries;
}





std::vector<std::string> DatabaseHandler::getColumnNames(const std::string& tableName) {
    std::vector<std::string> columnNames;

    // PRAGMA command to get table information
    CppSQLite3Statement stmt = db.compileStatement(("PRAGMA table_info(" + tableName + ");").c_str());
    CppSQLite3Query query = stmt.execQuery();

    while (!query.eof()) {
        columnNames.push_back(query.getStringField("name"));  // "name" column contains the field names
        query.nextRow();
    }

    return columnNames;
}



void DatabaseHandler::generateMediaDataJson() {
    json mediaDataJson;

    // Get column names for the Collection and Media tables
    std::vector<std::string> collectionColumns = getColumnNames("Collection");
    std::vector<std::string> mediaColumns = getColumnNames("Media");

    // Fetch all collections
    CppSQLite3Query collectionQuery = db.execQuery("SELECT * FROM Collection;");
    while (!collectionQuery.eof()) {
        json collectionJson;

        // Populate JSON for collection row dynamically
        for (const auto& col : collectionColumns) {
            collectionJson[col] = collectionQuery.getStringField(col.c_str());
        }

        // Fetch media items associated with this collection
        CppSQLite3Statement mediaStmt = db.compileStatement("SELECT * FROM Media WHERE collection_id = ?;");
        mediaStmt.bind(1, collectionQuery.getStringField("ID"));
        CppSQLite3Query mediaQuery = mediaStmt.execQuery();

        // Add media items to collection JSON
        json mediaArray = json::array();
        while (!mediaQuery.eof()) {
            json mediaJson;

            // Populate JSON for media row dynamically
            for (const auto& col : mediaColumns) {
                mediaJson[col] = mediaQuery.getStringField(col.c_str());
            }

            mediaArray.push_back(mediaJson);
            mediaQuery.nextRow();
        }
        collectionJson["medias"] = mediaArray;
        mediaDataJson["collections"].push_back(collectionJson);
        collectionQuery.nextRow();
    }

    // Write the generated JSON to file
    std::ofstream file("media_data.json");
    file << mediaDataJson.dump(4);  // pretty-print with 4-space indentation
    file.close();
}

void DatabaseHandler::generateUserMetadataJson(const std::string& userID, const std::string& profileID) {
    json userMetadataJson;

    // Get column names for the mediaMetadata table
    std::vector<std::string> metadataColumns = getColumnNames("mediaMetadata");

    // Fetch user metadata for the given userID and profileID
    CppSQLite3Statement stmt = db.compileStatement("SELECT * FROM mediaMetadata WHERE userID = ? AND profileID = ?;");
    stmt.bind(1, userID.c_str());
    stmt.bind(2, profileID.c_str());
    CppSQLite3Query query = stmt.execQuery();

    while (!query.eof()) {
        json metadataJson;

        // Populate JSON dynamically based on columns
        for (const auto& col : metadataColumns) {
            metadataJson[col] = query.getStringField(col.c_str());
        }

        userMetadataJson["mediaMetadata"].push_back(metadataJson);
        query.nextRow();
    }

    // Write to JSON file
    std::ofstream file("user_metadata.json");
    file << userMetadataJson.dump(4);  // pretty-print with 4-space indentation
    file.close();
}



std::string DatabaseHandler::getImagePathById(const std::string& id, const std::string& coversPath) {
    try {
        // Check if the ID exists in the Collection table
        CppSQLite3Statement stmtCollection = db.compileStatement("SELECT image_path FROM Collection WHERE ID = ?;");
        stmtCollection.bind(1, id.c_str());
        CppSQLite3Query queryCollection = stmtCollection.execQuery();

        fs::path coversFolder = coversPath;


        if (!queryCollection.eof()) {
            // If found in Collection, return the full path
            fs::path imagePath = queryCollection.getStringField("image_path");
            fs::path fullPath = coversFolder / imagePath;
            std::string fullPathStr = fullPath.string();
            return fullPathStr;
        }

        // If not found in Collection, check the Media table
        CppSQLite3Statement stmtMedia = db.compileStatement("SELECT image_path FROM Media WHERE ID = ?;");
        stmtMedia.bind(1, id.c_str());
        CppSQLite3Query queryMedia = stmtMedia.execQuery();

        if (!queryMedia.eof()) {
            // If found in Media, return the full path
            fs::path imagePath = queryCollection.getStringField("image_path");
            fs::path fullPath = coversFolder / imagePath;
            std::string fullPathStr = fullPath.string();
            return fullPathStr;
        }
    }
    catch (const CppSQLite3Exception& e) {
        std::cerr << "Database error: " << e.errorMessage() << std::endl;
    }

    // Return an empty string if no match is found
    return "";
}
