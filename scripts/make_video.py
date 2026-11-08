"""Render the simulation frames (frame_0.png, frame_1.png, ...) into a video with ffmpeg.

The encoder is picked from the output extension unless --encoder is given:
  .mp4  -> H.264 (default; plays in the VS Code preview, browsers, GitHub and most players)
  .webm -> VP9
Avoid HEVC (hevc_nvenc): the VS Code preview and many browsers cannot play it.
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

DEFAULT_ENCODERS = {
    ".webm": "libvpx-vp9",
    ".mp4": "libx264",
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--frames", type=Path, default=Path("out/frames"), help="directory with frame_N.png files")
    parser.add_argument(
        "--output", type=Path, default=Path("out/output_video.mp4"), help="video file to write (.mp4 or .webm)"
    )
    parser.add_argument("--fps", type=int, default=30, help="frames per second")
    parser.add_argument("--scale", type=int, default=800, help="output width and height in pixels")
    parser.add_argument(
        "--encoder",
        help="ffmpeg video encoder, overrides the choice by extension (e.g. h264_nvenc for an NVIDIA GPU)",
    )
    parser.add_argument("--cleanup", action="store_true", help="delete the frames directory afterwards")
    args = parser.parse_args()

    if shutil.which("ffmpeg") is None:
        print("ffmpeg was not found. Install it and make sure it is on PATH.", file=sys.stderr)
        return 1
    if not args.frames.is_dir() or not any(args.frames.glob("frame_*.png")):
        print(f"No frame_*.png files found in '{args.frames}'. Run the simulation first.", file=sys.stderr)
        return 1

    suffix = args.output.suffix.lower()
    encoder = args.encoder or DEFAULT_ENCODERS.get(suffix)
    if encoder is None:
        print(f"Unknown video format '{suffix}'. Use .webm or .mp4, or pass --encoder.", file=sys.stderr)
        return 1

    if encoder.endswith("_nvenc"):
        quality = ["-b:v", "33M"]
    elif encoder.startswith("libvpx"):
        quality = ["-crf", "30", "-b:v", "0", "-row-mt", "1"]
    else:
        quality = ["-crf", "18", "-preset", "medium"]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        "ffmpeg", "-y", "-loglevel", "error",
        "-framerate", str(args.fps),
        "-i", str(args.frames / "frame_%d.png"),
        # nearest-neighbour keeps the cells sharp instead of blurring them
        "-vf", f"scale={args.scale}:{args.scale}:flags=neighbor",
        "-c:v", encoder, *quality,
        "-pix_fmt", "yuv420p",
        str(args.output),
    ]
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as error:
        print(f"ffmpeg failed with exit code {error.returncode}", file=sys.stderr)
        return 1

    if args.cleanup:
        shutil.rmtree(args.frames)

    print(f"Video saved as {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
