#include "api.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <filesystem>
#include <regex>
namespace fs = std::filesystem;     


API::API(DatabaseHandler& dbHandler, const std::string& coversPath, const std::string& chunksPath)
    : db(dbHandler), coversPath(coversPath), chunksPath(chunksPath) {
    loadTokens();
}


void API::loadTokens() {
    std::vector<std::pair<std::string, std::string>> users = db.getAllUserTokens();
    for (const auto& user : users) {
        tokens[user.first] = user.second;  // user.first is userID, user.second is token
    }
}

void API::run(int port) {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/media/<string>/manifest")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
        ([this](const crow::request& req, const std::string& media_id) {
        return handleManifestRequest(req, media_id);
            });

    CROW_ROUTE(app, "/media/<string>/chunk/<string>")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
        ([this](const crow::request& req, const std::string& media_id, const std::string& chunk_name) {
        return handleChunkRequest(req, media_id, chunk_name);
            });

    CROW_ROUTE(app, "/media/<string>/subtitles/<string>")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
        ([this](const crow::request& req, const std::string& media_id, const std::string& language) {
        return subtitlesRequest(req, media_id, language);
            });



    CROW_ROUTE(app, "/cover/<string>").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req, const std::string& id) {
        return getCoverImage(req, id);
            });

    CROW_ROUTE(app, "/media/data").methods(crow::HTTPMethod::GET)([this](const crow::request& req) {
        return getMediaData(req);
        });

    // Route to get user-specific metadata JSON
    CROW_ROUTE(app, "/user/metadata").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return getUserMetadata(req);
        });


    // Route to add a new profile
    CROW_ROUTE(app, "/profile/add").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return addProfile(req);
        });

    // Route to delete a profile
    CROW_ROUTE(app, "/profile/delete").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return deleteProfile(req);
        });

    // Route to list all profiles for a specific user
    CROW_ROUTE(app, "/profile/list").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return listProfiles(req);
        });

    CROW_ROUTE(app, "/download/media_data").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req){

        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }

        std::string userID = x["userID"].s();
        std::string token = x["token"].s();

        crow::json::wvalue response;


        if (!checkToken(token, userID)) {
            response["status"] = "error";
            response["message"] = "Incorrect token.";
            return crow::response(401, response);
        }


        db.generateMediaDataJson();
        std::ifstream file("media_data.json");
        if (!file.is_open()) {
            return crow::response(500, "Failed to open media_data.json");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        crow::response res(buffer.str());
        res.add_header("Content-Type", "application/json");
        return res;
            });

    // Route to serve the pre-generated user metadata JSON file
    CROW_ROUTE(app, "/download/user_metadata").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
        
        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }

        std::string userID = x["userID"].s();
        std::string profileID = x["profileID"].s();
  

        // Generate user metadata JSON if needed
        db.generateUserMetadataJson(userID, profileID);

        std::ifstream file("user_metadata.json");
        if (!file.is_open()) {
            return crow::response(500, "Failed to open user_metadata.json");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        crow::response res(buffer.str());
        res.add_header("Content-Type", "application/json");
        return res;
            });

    app.port(port).multithreaded().run();
}


// In api.cpp, update handleManifestRequest:



bool API::validateRequest(const crow::request& req, std::string& userID, std::string& token) {
    // Check headers first
    auto userIdHeader = req.get_header_value("X-User-ID");
    auto tokenHeader = req.get_header_value("X-Auth-Token");

    if (!userIdHeader.empty() && !tokenHeader.empty()) {
        userID = userIdHeader;
        token = tokenHeader;
        return checkToken(token, userID);
    }

    // Check query parameters
    if (req.url_params.get("userID") && req.url_params.get("token")) {
        userID = req.url_params.get("userID");
        token = req.url_params.get("token");
        return checkToken(token, userID);
    }

    // Check if the route matches the supported patterns
    if (req.url.find("/media/") != std::string::npos) {
        // Extract the last segment of the route to identify if it's `manifest` or `chunk/<string>`
        std::string::size_type lastSlash = req.url.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string lastSegment = req.url.substr(lastSlash + 1);

            if (lastSegment == "manifest" || req.url.find("/chunk/") != std::string::npos) {
                // Check query parameters in the URL for `userID` and `token`
                auto userIdQuery = req.url_params.get("userID");
                auto tokenQuery = req.url_params.get("token");

                if (userIdQuery && tokenQuery) {
                    userID = userIdQuery;
                    token = tokenQuery;
                    return checkToken(token, userID);
                }
            }
        }
    }

    // Finally check the JSON body
    try {
        auto jsonBody = crow::json::load(req.body);
        if (!jsonBody) return false;

        userID = jsonBody["userID"].s();
        token = jsonBody["token"].s();
        return checkToken(token, userID);
    }
    catch (...) {
        return false;
    }

    return false;
}


crow::response API::subtitlesRequest(const crow::request& req, const std::string& media_id, const std::string& language) {
    std::string userID, token;

    

    // Construct the full path to the MPD file
    std::filesystem::path vttPath = std::filesystem::path(chunksPath) / media_id / "subtitles" / (language);

    try {
        // Read the MPD file
        std::ifstream file(vttPath);
        if (!file.is_open()) {
            return crow::response(500, "Failed to open VTT file");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string subtitlesContent = buffer.str();

        // Ensure proper UTF-8 BOM if needed
        if (subtitlesContent.length() >= 3 &&
            (unsigned char)subtitlesContent[0] != 0xEF &&
            (unsigned char)subtitlesContent[1] != 0xBB &&
            (unsigned char)subtitlesContent[2] != 0xBF) {
            subtitlesContent = std::string("\xEF\xBB\xBF") + subtitlesContent;
        }

        crow::response res;
        res.body = subtitlesContent;
        res.set_header("Content-Type", "text/vtt");
        return res;
    }
    catch (const std::exception& e) {
        return crow::response(500, std::string("Error processing VTT file: ") + e.what());
    }
}

crow::response API::handleManifestRequest(const crow::request& req, const std::string& media_id) {
    std::string userID, token;

    // Only validate auth for manifest requests
    if (!validateRequest(req, userID, token)) {
        return crow::response(401, "Invalid authentication");
    }

    // Construct the full path to the MPD file
    std::filesystem::path mpdPath = std::filesystem::path(chunksPath) / media_id / (media_id + ".mpd");

    try {
        // Read the MPD file
        std::ifstream file(mpdPath);
        if (!file.is_open()) {
            return crow::response(500, "Failed to open MPD file");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string manifestContent = buffer.str();

        // Construct base URL with authentication parameters
        std::string baseUrl = "http://localhost:18080/media/" + media_id + "/chunk";

        // Modified URL patterns to include auth params in initialization and media URLs
        auto replacePaths = [&manifestContent, &baseUrl, &userID, &token](const std::string& attribute, const std::string& replacement) {
            size_t pos = 0;
            while ((pos = manifestContent.find(attribute + "=", pos)) != std::string::npos) {
                size_t start = manifestContent.find("\"", pos) + 1;
                size_t end = manifestContent.find("\"", start);
                // Add auth params to URLs
                std::string newUrl = baseUrl + replacement;
                manifestContent.replace(start, end - start, newUrl);
                pos = end;
            }
            };

        // Update initialization segments
        replacePaths("initialization", "/init-stream$RepresentationID$.m4s");

        // Update media segments
        replacePaths("media", "/chunk-stream$RepresentationID$-$Number%05d$.m4s");

        std::string baseVttUrl = "http://localhost:18080/media/" + media_id + "/subtitles/";

        

        // Look for </Period> tag to insert before it
        size_t periodEnd = manifestContent.find("</Period>");
        if (periodEnd != std::string::npos) {
            std::string subtitleAdaptationSet = R"(
                <AdaptationSet id="2" contentType="text" mimeType="text/vtt" lang="en" segmentAlignment="true">
                    <Representation id="subtitles_en" mimeType="text/vtt" codecs="wvtt" bandwidth="256">
                        <BaseURL>)" + baseVttUrl + R"(en.vtt</BaseURL>
                    </Representation>
                </AdaptationSet>
                <AdaptationSet id="3" contentType="text" mimeType="text/vtt" lang="es" segmentAlignment="true">
                    <Representation id="subtitles_es" mimeType="text/vtt" codecs="wvtt" bandwidth="256">
                        <BaseURL>)" + baseVttUrl + R"(es.vtt</BaseURL>
                    </Representation>
                </AdaptationSet>
            )";
            //manifestContent.insert(periodEnd, subtitleAdaptationSet);
        } 

        crow::response res;
        res.body = manifestContent;
        res.set_header("Content-Type", "application/dash+xml");
        return res;
    }
    catch (const std::exception& e) {
        return crow::response(500, std::string("Error processing MPD file: ") + e.what());
    }
}




crow::response API::handleChunkRequest(const crow::request& req, const std::string& media_id, const std::string& chunk_name) {
    std::string userID, token;

    //if (!validateRequest(req, userID, token)) {
    //    return crow::response(401, "Invalid authentication");
    //}

    std::filesystem::path chunkPath = std::filesystem::path(chunksPath) / media_id / (chunk_name);

    std::ifstream file(chunkPath, std::ios::binary);
    if (!file.is_open()) {
        return crow::response(404, "Chunk not found");
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    crow::response res;
    res.body = std::move(content);
    res.set_header("Content-Type", "video/mp4");
    return res;
}


crow::response API::serveFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return crow::response(404, "File not found.");
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string fileContent(size, '\0');
    file.read(&fileContent[0], size);

    crow::response res;
    res.body = std::move(fileContent);
    res.set_header("Content-Type", "application/octet-stream");
    return res;
}


crow::response API::getMediaData(const crow::request& req) {
    // Generate the media data JSON file
    db.generateMediaDataJson();

    // Read the generated file
    std::ifstream file("media_data.json");
    if (!file.is_open()) {
        return crow::response(500, "Failed to open media_data.json");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return crow::response{ buffer.str() };
}

crow::response API::getUserMetadata(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string userID = x["userID"].s();
    std::string profileID = x["profileID"].s();

    // Generate the user metadata JSON file based on userID and profileID
    db.generateUserMetadataJson(userID, profileID);

    // Read the generated file
    std::ifstream file("user_metadata.json");
    if (!file.is_open()) {
        return crow::response(500, "Failed to open user_metadata.json");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return crow::response{ buffer.str() };
}




crow::response API::addProfile(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string userID = x["userID"].s();
    std::string profileID = x["profileID"].s();
    std::string pictureID = x["pictureID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;
    

    if (!checkToken(token, userID)) {
        response["status"] = "error";
        response["message"] = "Incorrect token.";
        return crow::response(401, response);
    }

    bool success = db.addProfile(userID, profileID, pictureID);
    if (success) {
        response["status"] = "success";
        response["message"] = "Profile created successfully.";
    }
    else {
        response["status"] = "error";
        response["message"] = "Failed to create profile. User may not exist or profile already exists.";
    }

    return crow::response(std::move(response));  // Use std::move here
}

crow::response API::deleteProfile(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string profileID = x["profileID"].s();
    std::string userID = x["userID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;

    if (!checkToken(token, userID)) {
        response["status"] = "error";
        response["message"] = "Incorrect token.";
        return crow::response(401, response);
    }


    bool success = db.deleteProfile(userID, profileID);
    

    if (success) {
        response["status"] = "success";
        response["message"] = "Profile deleted successfully.";
    }
    else {
        response["status"] = "error";
        response["message"] = "Failed to delete profile. Profile may not exist.";
    }

    return crow::response(std::move(response));  // Use std::move here
}

crow::response API::listProfiles(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string userID = x["userID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;

    if (!checkToken(token, userID)) {
        response["status"] = "error";
        response["message"] = "Incorrect token.";
        return crow::response(401, response);
    }

    std::vector<std::pair<std::string, std::string>> profiles;
    db.getProfiles(userID, profiles);





    crow::json::wvalue::list profileList;

    for (const auto& profile : profiles) {
        crow::json::wvalue profileJson;
        profileJson["profileID"] = profile.first;
        profileJson["pictureID"] = profile.second;
        profileList.push_back(std::move(profileJson));  // Use std::move for inner elements
    }

    response["profiles"] = std::move(profileList);  // Move the list instead of copying
    return crow::response(std::move(response));      // Move response to avoid copy
}


bool API::checkToken(const std::string& token, const std::string& userID) {
    auto it = tokens.find(userID);
    return (it != tokens.end() && it->second == token);
}


crow::response API::getCoverImage(const crow::request& req, const std::string& id) {
    // Parse JSON body to get token and userID
    auto bodyJson = json::parse(req.body, nullptr, false);
    if (bodyJson.is_discarded()) {
        return crow::response(400, "Invalid JSON in request body");
    }

    // Check if both token and userID are provided in the body
    if (!bodyJson.contains("token") || !bodyJson.contains("userID")) {
        return crow::response(400, "Missing token or userID in request body");
    }

    std::string token = bodyJson["token"].get<std::string>();
    std::string userID = bodyJson["userID"].get<std::string>();

    // Check if the provided token is valid for the given userID
    if (!checkToken(token, userID)) {
        // If the token is invalid, return a 401 Unauthorized response
        return crow::response(401, "Unauthorized access - invalid token");
    }

    // Retrieve the full path to the image using DatabaseHandler
    std::string fullPath = db.getImagePathById(id, coversPath);

    if (fullPath.empty()) {
        // If no image path is found, return a 404 error
        return crow::response(404, "Cover image not found");
    }

    // Open the image file in binary mode
    std::ifstream imageFile(fullPath, std::ios::binary);
    if (!imageFile.is_open()) {
        // Return 404 if the image file is missing on the server
        return crow::response(404, "Image file not found on server");
    }

    // Read the image content
    std::stringstream buffer;
    buffer << imageFile.rdbuf();
    imageFile.close();

    // Create the response with appropriate content type
    crow::response res(buffer.str());
    res.add_header("Content-Type", "image/jpeg");  // Adjust if images may be of a different format
    return res;
}

