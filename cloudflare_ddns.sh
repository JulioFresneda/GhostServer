#!/bin/bash

# ==========================================
# Cloudflare DDNS Update Script
# ==========================================

CONFIG_FILE="/home/juliofgx/CLionProjects/GhostServer/config.json"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Configuration file not found at $CONFIG_FILE"
    exit 1
fi

# Extract values directly from config.json using grep (regex)
API_TOKEN=$(grep -Po '"domainToken"\s*:\s*"\K[^"]+' "$CONFIG_FILE")
ZONE_NAME=$(grep -Po '"domain"\s*:\s*"\K[^"]+' "$CONFIG_FILE")

if [ -z "$API_TOKEN" ] || [ -z "$ZONE_NAME" ]; then
    echo "Error: Could not extract 'domainToken' or 'domain' from $CONFIG_FILE"
    exit 1
fi

RECORD_NAME="$ZONE_NAME"       # The specific record to update (usually same as ZONE_NAME)
PROXIED="true"                 # "true" for Cloudflare proxy (orange cloud), "false" for DNS only

# 1. Get current public IP
IP=$(curl -s https://api.ipify.org)
if [ -z "$IP" ]; then
    echo "Failed to fetch public IP."
    exit 1
fi
echo "Current Public IP: $IP"

# 2. Get Zone ID
ZONE_ID=$(curl -s -X GET "https://api.cloudflare.com/client/v4/zones?name=$ZONE_NAME" \
    -H "Authorization: Bearer $API_TOKEN" \
    -H "Content-Type: application/json" | grep -Po '(?<="id":")[^"]*' | head -1)

if [ -z "$ZONE_ID" ]; then
    echo "Failed to find Zone ID for $ZONE_NAME"
    exit 1
fi

# 3. Get Record ID
RECORD_ID=$(curl -s -X GET "https://api.cloudflare.com/client/v4/zones/$ZONE_ID/dns_records?name=$RECORD_NAME&type=A" \
    -H "Authorization: Bearer $API_TOKEN" \
    -H "Content-Type: application/json" | grep -Po '(?<="id":")[^"]*' | head -1)

if [ -z "$RECORD_ID" ]; then
    echo "Failed to find Record ID for $RECORD_NAME"
    exit 1
fi

# 4. Update DNS Record
RESPONSE=$(curl -s -X PUT "https://api.cloudflare.com/client/v4/zones/$ZONE_ID/dns_records/$RECORD_ID" \
    -H "Authorization: Bearer $API_TOKEN" \
    -H "Content-Type: application/json" \
    --data '{"type":"A","name":"'"$RECORD_NAME"'","content":"'"$IP"'","proxied":'"$PROXIED"'}')

if echo "$RESPONSE" | grep -q '"success":true'; then
    echo "Successfully updated $RECORD_NAME to $IP"
else
    echo "Failed to update DNS record. Response: $RESPONSE"
fi