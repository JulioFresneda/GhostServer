import subprocess
import json
import os
from pathlib import Path


def extract_subtitles_to_vtt(input_file, output_dir):
    """
    Extract English and Spanish subtitles from a media file and save them as WebVTT files.
    Supports conversion from SRT to WebVTT format.
    """
    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # Define language mapping (including common variants)
    language_mapping = {
        'eng': 'en.vtt',
        'en': 'en.vtt',
        'english': 'en.vtt',
        'spa': 'es.vtt',
        'es': 'es.vtt',
        'spanish': 'es.vtt',
        'esp': 'es.vtt'
    }

    # Probe the input file to get detailed stream information
    probe_cmd = [
        "ffprobe",
        "-v", "error",
        "-print_format", "json",
        "-show_streams",
        "-select_streams", "s",
        input_file
    ]

    try:
        result = subprocess.run(probe_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        result.check_returncode()
        probe_data = json.loads(result.stdout)
    except (subprocess.CalledProcessError, json.JSONDecodeError) as e:
        print(f"Error probing file: {e}")
        return

    # Keep track of which languages we've found
    found_languages = set()

    # Process each subtitle stream
    for stream in probe_data.get("streams", []):
        if stream.get("codec_type") != "subtitle":
            continue

        # Get stream index and metadata
        index = stream.get("index", 0)
        tags = stream.get("tags", {})

        # Try to get language from different possible metadata locations
        language = None

        # Check language tag
        if "language" in tags:
            language = tags["language"].lower()

        # If no language tag, try to determine from title if it exists
        elif "title" in tags:
            title = tags["title"].lower()
            if "english" in title or "eng" in title:
                language = "eng"
            elif "spanish" in title or "spa" in title or "esp" in title:
                language = "spa"

        # Skip if we can't determine the language or already processed this language
        if not language or language not in language_mapping or language in found_languages:
            continue

        output_file = os.path.join(output_dir, language_mapping[language])
        print(f"Extracting {language} subtitles from stream {index} to {output_file}")

        # Construct FFmpeg command with explicit subtitle format conversion
        ffmpeg_cmd = [
            "ffmpeg",
            "-i", input_file,
            "-map", f"0:{index}",
            "-c:s", "webvtt",
            "-f", "webvtt",
            "-y",
            output_file
        ]

        try:
            # Execute FFmpeg command
            process = subprocess.run(
                ffmpeg_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            process.check_returncode()
            print(f"Successfully extracted {language} subtitles")
            found_languages.add(language)

        except subprocess.CalledProcessError as e:
            print(f"Error extracting subtitles for stream {index}: {e}")
            print(f"FFmpeg error output: {e.stderr}")
            print("Command used:", " ".join(ffmpeg_cmd))
            continue

    # Report if any language wasn't found
    for lang, filename in [('English', 'en.vtt'), ('Spanish', 'es.vtt')]:
        if not os.path.exists(os.path.join(output_dir, filename)):
            print(f"Warning: No {lang} subtitles were found in the file")

    print(f"Subtitle extraction completed. Files saved to {output_dir}")