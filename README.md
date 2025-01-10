# Welcome to the GhostServer!
![server](screenshots/server.png)

This is the server-side of the GhostStream project. The soul.

## What do you need?
You will need some packages. I used VCPKG, because this project was developed in VS 2022. You can use it too, or just install the packages manually.

- sqlite3
- crow
- nlohmann-json
- jwt-cpp
- openssl
- picojson
- curl

## Let's talk about IPs
Yeah, I have DDNS too. The solution I found is pretty simple: DuckDNS.
With DuckDNS, the client will always being able to reach the server, no matter what public IP has the server
right now.

Just create a domain, download the agent to keep it updated and hardcode the domain in the config JSON. That's it.

## Initialize the server

### Step 1: Where to store the media
First of all, you need a path to store the media. I recommend a new partition, or a new SSD.
In this path, you will store:
- Media covers
- Media chunks
- Media subtitles
- Server database

About media chunks, this server streams with DASH protocol. So, we don't need the actual media file, just the chunks. 
Anyway, keep in mind that this server will use a lot of space, depending on the amount of media you have.

Of course, we both know that you will not store any pirated media, just the record of the wedding of your cousin.

Do you have the path? Great! Let's move on.

### Step 2: Initialize the database
This server uses a SQLite3 database. To create it, just run the *init.py* script in the selected path.
This script will create the database and the tables.

### Step 3: Adding users
What is an user? It is not a profile like a Netflix user. An user is a client that will connect with the server, and have multiple profiles.

The *add_user.py* file is for this. For each user (**not profile**) you want to add, run this script with --user and --token parameters.
The token can be anything, you will have to decide one, and share it with the user. It will serve to authenticate the user and he will be able to get the JWT tokens.

That's it.

### Step 4: Run the server
Run the executable. It is stored in the /bin/ folder. The server uses the port 38080, keep that in mind. Also, remember to update the *config.json* paths.
As long as this executable is running, the server will be available to the clients.

I think that's all. If you have any questions, feel free to ask to my email.
