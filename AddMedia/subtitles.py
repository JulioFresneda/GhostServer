import subprocess
import json
import os

def extract_subtitles_to_vtt(input_file, output_dir):
    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # Probe the input file to get its stream information
    probe_cmd = ["ffprobe", "-v", "error", "-show_streams", "-print_format", "json", input_file]
    result = subprocess.run(probe_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    probe_data = json.loads(result.stdout)

    # Find subtitle streams
    subtitle_streams = [stream for stream in probe_data["streams"] if stream["codec_type"] == "subtitle"]

    if not subtitle_streams:
        print("No subtitle streams found.")
        return

    # Extract each subtitle stream as a .vtt file
    for i, stream in enumerate(subtitle_streams):
        output_file = os.path.join(output_dir, f"subtitle_{i}.vtt")
        ffmpeg_cmd = [
            "ffmpeg",
            "-i", input_file,
            f"-map", f"0:s:{i}",
            "-c:s", "webvtt",
            output_file,
            "-y"  # Overwrite if file exists
        ]
        print(f"Extracting subtitle stream {i} to {output_file}...")
        subprocess.run(ffmpeg_cmd)

    print(f"Extraction completed. Subtitles saved to {output_dir}")