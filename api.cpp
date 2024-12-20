#include "api.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
using json = nlohmann::json;
#include <filesystem>
#include <regex>
namespace fs = std::filesystem;     
#include "jwt-cpp/jwt.h"

const std::string SECRET_KEY = "carmen";

std::string generateJWT(const std::string& userID) {
    return jwt::create()
        .set_issuer("api_service")
        .set_subject(userID)
        .set_issued_at(std::chrono::system_clock::now())
        //.set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(1)) // Token expires in 1 hour
        .sign(jwt::algorithm::hs256{ SECRET_KEY });
}

bool validateJWT(const std::string& token, std::string& userID) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ SECRET_KEY })
            .with_issuer("api_service");
        verifier.verify(decoded);

        userID = decoded.get_subject(); // Extract user ID from the subject claim
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "JWT validation error: " << e.what() << std::endl;
        return false;
    }
}

bool API::validateRequest(const crow::request& req, std::string& userID) {
    // Check headers first
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty()) {
        return false;
    }

    // Expect "Bearer <token>"
    const std::string bearerPrefix = "Bearer ";
    if (authHeader.find(bearerPrefix) != 0) {
        return false;
    }

    // Extract the JWT from the header
    std::string token = authHeader.substr(bearerPrefix.size());
    return validateJWT(token, userID);
}

API::API(DatabaseHandler& dbHandler, const std::string& coversPath, const std::string& chunksPath)
    : db(dbHandler), coversPath(coversPath), chunksPath(chunksPath) {
    loasPasswords();
}


void API::loasPasswords() {
    std::vector<std::pair<std::string, std::string>> users = db.getAllUserPasswords();
    for (const auto& user : users) {
        passwords[user.first] = user.second;  // user.first is userID, user.second is pass
    }
}

void API::run(int port) {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/auth/login").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return login(req);
        });

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
        return getMediaMetadata(req);
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
        return downloadMediaData(req);
            });

    // Route to serve the pre-generated user metadata JSON file
    CROW_ROUTE(app, "/download/media_metadata").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
        
        return downloadMediaMetadata(req);
            });

    CROW_ROUTE(app, "/update_media_metadata").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return updateMediaMetadata(req);
        });

    app.bindaddr("0.0.0.0").port(port).multithreaded().run();
}

crow::response API::login(const crow::request& req) {
    auto bodyJson = crow::json::load(req.body);
    if (!bodyJson) {
        return crow::response(400, "Invalid JSON");
    }

    // Extract `userID` and `password` from the request body
    std::string userID = bodyJson["userID"].s();
    std::string password = bodyJson["password"].s();

    // Validate the user credentials using your database
    if (checkToken(userID, password)) { // Replace with your actual validation logic
        return crow::response(401, "Invalid userID or password");
    }

    // Generate a JWT if the credentials are valid
    std::string token = generateJWT(userID);              // Sign the token with your secret key

    // Respond with the token
    crow::json::wvalue response;
    response["token"] = token;
    return crow::response(response);
}


// In api.cpp, update handleManifestRequest:

crow::response API::downloadMediaMetadata(const crow::request& req) {
    
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }
    
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string profileID = x["profileID"].s();

    
    // Generate user metadata JSON if needed
    db.generateMediaMetadataJson(userID, profileID);

    std::ifstream file("media_metadata.json");
    if (!file.is_open()) {
        return crow::response(500, "Failed to open media_metadata.json");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    crow::response res(buffer.str());
    res.add_header("Content-Type", "application/json");
    return res;
}


crow::response API::downloadMediaData(const crow::request& req) {
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
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
}

crow::response API::subtitlesRequest(const crow::request& req, const std::string& media_id, const std::string& language) {
    std::string userID;

    //if (!validateRequest(req, userID)) {
    //    return crow::response(401, "Invalid authentication");
    //}

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
    std::string userID;

    //if (!validateRequest(req, userID)) {
    //    return crow::response(401, "Invalid authentication");
    //}

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
        std::string baseUrl = "http://" + getPublicIP() + ":38080/media/" + media_id + "/chunk/";

        // Modified URL patterns to include auth params in initialization and media URLs
        auto replacePaths = [&manifestContent, &baseUrl](const std::string& attribute, const std::string& replacement) {
            size_t pos = 0;
            while ((pos = manifestContent.find(attribute + "=", pos)) != std::string::npos) {
                size_t start = manifestContent.find("\"", pos) + 1;
                size_t end = manifestContent.find("\"", start);
                // Construct the new URL
                std::string newUrl = baseUrl + replacement;
                manifestContent.replace(start, end - start, newUrl);
                pos = end;
            }
            };

        // Replace `initialization` and `media` attributes
        replacePaths("initialization", "init-stream$RepresentationID$.m4s");
        replacePaths("media", "chunk-stream$RepresentationID$-$Number%05d$.m4s");

        // Update initialization segments
        //replacePaths("initialization", "/init-stream$RepresentationID$.m4s");

        // Update media segments
        //replacePaths("media", "/chunk-stream$RepresentationID$-$Number%05d$.m4s");

        std::string baseVttUrl = "http://" + getPublicIP() + ":38080 / media / " + media_id + " / subtitles / ";

        

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
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }

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

crow::response API::getMediaMetadata(const crow::request& req) {
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }

    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string profileID = x["profileID"].s();

    // Generate the user metadata JSON file based on userID and profileID
    db.generateMediaMetadataJson(userID, profileID);

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

crow::response API::updateMediaMetadata(const crow::request& req) {
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }

    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string profileID = x["profileID"].s();
    std::string mediaID = x["mediaID"].s();
    double percentageWatched = std::stod(x["percentageWatched"].s());
    std::string languageChosen = x["languageChosen"].s();
    std::string subtitlesChosen = x["subtitlesChosen"].s();



    crow::json::wvalue response;



    int success = db.insertMediaMetadata(userID, profileID, mediaID, percentageWatched, languageChosen, subtitlesChosen);
    if (success == 0) {
        response["status"] = "success";
        response["message"] = "Profile created successfully.";
    }
    else {
        response["status"] = "error";
        response["message"] = "Failed to update media metadata.";
    }

    return crow::response(std::move(response));  // Use std::move here
}




crow::response API::addProfile(const crow::request& req) {
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }

    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string profileID = x["profileID"].s();
    std::string pictureID = x["pictureID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;
    

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
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }

    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string profileID = x["profileID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;


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
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }

 

    crow::json::wvalue response;

  

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
    auto it = passwords.find(userID);
    return (it != passwords.end() && it->second == token);
}


crow::response API::getCoverImage(const crow::request& req, const std::string& id) {
    std::string userID;

    if (!validateRequest(req, userID)) {
        return crow::response(401, "Invalid authentication");
    }

    // Parse JSON body to get token and userID
    auto bodyJson = json::parse(req.body, nullptr, false);
    if (bodyJson.is_discarded()) {
        return crow::response(400, "Invalid JSON in request body");
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

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userData) {
    userData->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string API::getPublicIP() {
    CURL* curl;
    CURLcode res;
    std::string publicIP; // Variable to store the IP address

    curl = curl_easy_init(); // Initialize cURL
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org"); // Set the URL
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // Set callback function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &publicIP); // Set data for the callback
        res = curl_easy_perform(curl); // Perform the request
        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl); // Clean up before returning
            return ""; // Return an empty string on failure
        }
        curl_easy_cleanup(curl); // Clean up
    }
    else {
        std::cerr << "Failed to initialize cURL." << std::endl;
        return ""; // Return an empty string if cURL failed to initialize
    }
    return publicIP; // Return the public IP as a string
}
