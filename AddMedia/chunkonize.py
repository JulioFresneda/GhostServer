import os
import json
import subprocess
from pathlib import Path
import os
import subprocess
import json

from subtitles import extract_subtitles_to_vtt


def get_recommended_bitrate(width, height):
    """Calculate recommended video bitrate based on resolution."""
    if width >= 3840 or height >= 2160:  # 4K
        return "45000k"  # 45 Mbps
    elif width >= 2560 or height >= 1440:  # 1440p
        return "20000k"  # 20 Mbps
    elif width >= 1920 or height >= 1080:  # 1080p
        return "10000k"  # 10 Mbps
    elif width >= 1280 or height >= 720:  # 720p
        return "6000k"  # 6 Mbps
    else:  # 480p or lower
        return "3000k"  # 3 Mbps


def prepare_media_for_streaming(media_file_path, output_folder, media_id):
    output_folder = os.path.normpath(output_folder)
    os.makedirs(output_folder, exist_ok=True)
    mpd_file_path = os.path.join(output_folder, f"{media_id}.mpd")

    # Probe for streams info
    probe_cmd = [
        "ffprobe",
        "-v", "quiet",
        "-print_format", "json",
        "-show_streams",
        media_file_path
    ]
    probe_result = subprocess.run(probe_cmd, capture_output=True, text=True)
    if probe_result.returncode != 0:
        raise RuntimeError(f"FFprobe failed: {probe_result.stderr}")

    info = json.loads(probe_result.stdout)


    #original_bitrate = 0
    #for stream in info["streams"]:
    #    if stream["codec_type"] == "video":
    #        original_bitrate =
    #if original_bitrate > 0:
    #    print(f"Original bitrate: {original_bitrate / 1000} kbps")
    #else:
    #    video_stream = next((s for s in info["streams"] if s["codec_type"] == "video"), None)
    #    if not video_stream:
    #        raise RuntimeError("No video stream found")

    #    width = int(video_stream.get("width", 1920))
    #    height = int(video_stream.get("height", 1080))
    #    original_bitrate = get_recommended_bitrate(width, height)
    #    print(f"Source resolution: {width}x{height}, Selected bitrate: {original_bitrate}")







    # Build the basic FFmpeg command
    cmd = [
        "ffmpeg",
        "-i", media_file_path,

        # Video stream - maintaining source resolution
        "-map", "0:v:0",
        "-c:v", "libx265",
        "-crf", "28", "-preset", "medium"
    ]

    # Count audio streams and add them to command
    audio_stream_count = sum(1 for s in info.get("streams", []) if s.get("codec_type") == "audio")

    for i in range(audio_stream_count):
        cmd.extend([
            "-map", f"0:a:{i}",
            f"-c:a:{i}", "aac",
            f"-b:a:{i}", "192k",  # Increased audio quality to 192k for better sound
        ])

    # Build adaptation sets string
    adaptation_sets = [
        # Video adaptation set
        "id=0,streams=0",
    ]

    # Add audio adaptation sets
    for i in range(audio_stream_count):
        adaptation_sets.append(f"id={i + 1},streams={i + 1}")

    # Add DASH output options
    cmd.extend([
        "-f", "dash",
        "-use_template", "1",
        "-use_timeline", "1",
        "-adaptation_sets", " ".join(adaptation_sets),
        "-seg_duration", "4",
        "-init_seg_name", "init-stream$RepresentationID$.m4s",
        "-media_seg_name", "chunk-stream$RepresentationID$-$Number%05d$.m4s",
        "-y",
        mpd_file_path
    ])

    # Debug: print the command
    print("FFmpeg command:\n", " ".join(cmd))

    # Run FFmpeg
    try:
        subprocess.run(cmd, check=True, cwd=output_folder)
        print(f"Streaming files prepared successfully in {output_folder}")
    except subprocess.CalledProcessError as e:
        print(f"Error preparing streaming files for {media_id}: {e}")




def chunkonize(output_folder, collection, items):

    # Get media file path and media ID from the metadata


    if collection["type"] == "movie":
        media_file_path = collection.get("media_filepath")
        media_id = collection.get("ID")
        #prepare_media_for_streaming(media_file_path, os.path.join(output_folder, media_id), media_id)
        extract_subtitles_to_vtt(media_file_path, os.path.join(output_folder, media_id, "subtitles"))

    elif collection["type"] == "serie":
        for episode in items:
            media_file_path = episode.get("media_filepath")
            media_id = episode.get("ID")
            prepare_media_for_streaming(media_file_path, os.path.join(output_folder, media_id), media_id)
            extract_subtitles_to_vtt(media_file_path, os.path.join(output_folder, media_id, "subtitles"))


    # Get media file path and media ID from the metadata
    for metadata in items:
        media_file_path = metadata.get("media_filepath")
        media_id = metadata.get("ID")


        prepare_media_for_streaming(media_file_path, output_folder, media_id)


