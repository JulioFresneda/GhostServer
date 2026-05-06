#include <iostream>
#include <fstream>
#include <string>
#include "DatabaseHandler.h"
#include "json.hpp" // nlohmann/json header
#include "api.h"

using json = nlohmann::json;

void loadPaths(const std::string& configFilePath, std::string& databasePath, std::string& coversPath, std::string& chunksPath, std::string& domain, std::string& domainToken) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open configuration file: " + configFilePath);
    }

    json configJson;
    configFile >> configJson; // Read JSON file into configJson object

    if (configJson.contains("databasePath") && configJson["databasePath"].is_string()) {
        databasePath = configJson["databasePath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'databasePath' not found or incorrect type.");
    }
    if (configJson.contains("coversPath") && configJson["coversPath"].is_string()) {
        coversPath = configJson["coversPath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'coversPath' not found or incorrect type.");
    }
    if (configJson.contains("chunksPath") && configJson["chunksPath"].is_string()) {
        chunksPath = configJson["chunksPath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'chunksPath' not found or incorrect type.");
    }
    if (configJson.contains("domain") && configJson["domain"].is_string()) {
        domain = configJson["domain"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: domain not found or incorrect type.");
    }
    if (configJson.contains("domainToken") && configJson["domainToken"].is_string()) {
        domainToken = configJson["domainToken"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'domainToken' not found or incorrect type.");
    }
}


int main() {
    try {

        std::cout << R"(
            
            ,---.  .           .  .---.                      
            |  -'  |-. ,-. ,-. |- \___  ,-. ,-. .  , ,-. ,-. 
            |  ,-' | | | | `-. |      \ |-' |   | /  |-' |   
            `---|  ' ' `-' `-' `' `---' `-' '   `'   `-' '   
             ,-.|                                            
             `-+'                                            
                                           
        )" << std::endl;

        // Specify the path to your configuration JSON file
        std::string configFilePath = "../config.json"; // Adjust the path as needed

        // Check if config exists, if not ask the user
        std::ifstream checkConfig(configFilePath);
        if (!checkConfig.is_open()) {
            std::cout << "Could not find config.json at " << configFilePath << std::endl;
            std::cout << "Please enter the absolute path to config.json: ";
            std::getline(std::cin, configFilePath);
        } else {
            checkConfig.close();
        }

        // Load the database path from the config file
        std::string databasePath; 
        std::string coversPath;
        std::string chunksPath;
        std::string domain;
        std::string domainToken;
        
        loadPaths(configFilePath, databasePath, coversPath, chunksPath, domain, domainToken);
        std::cout << "Database path loaded from config: " << databasePath << std::endl;

        // Create and initialize the DatabaseHandler with the database path
        DatabaseHandler dbHandler(databasePath);

        // Start your server logic here, for example:
        std::cout << "Server starting..." << std::endl;

        // Initialize the API with the database handler
        

        API api(dbHandler, coversPath, chunksPath, domain, domainToken);
        // Disabled to prevent overriding the Cloudflare Tunnel CNAME record:
        // std::cout << "Public IP: " << api.getPublicIP(domain) << std::endl;
        
        // Run the API server on a specified port
        api.run(8443); // 8443 is widely used for private secure streaming and is supported by Cloudflare

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
