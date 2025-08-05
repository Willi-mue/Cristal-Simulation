import os
import shutil
import subprocess


output_video = 'results/output_video.mp4'

frame_rate = 80
scale = 800  

# Check if frames were saved correctly
image_files = [f for f in os.listdir("frames") if f.endswith('.ppm')]
if not image_files:
    print("No image files found in the 'frames' directory. Check the image saving process.")
else:
    # Use FFmpeg to create the video
    cmd = [
        "ffmpeg",
        "-y",
        "-framerate", str(frame_rate),
        "-i", "frames/frame_%d.ppm",
        "-vf", f"scale={scale}:{scale}",
        "-c:v", "hevc_nvenc",
        "-b:v", "33M",
        "-r", f"{frame_rate}",  # Output frame rate (change as needed)
        "-pix_fmt", "yuv420p",
        "results/output_video.mp4"
    ]

    subprocess.run(cmd, check=True)

    # Remove the frames directory
    shutil.rmtree("frames")

    print("Video saved as output_video.mp4")