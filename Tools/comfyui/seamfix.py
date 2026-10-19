#!/usr/bin/env python3
"""seamfix – macht eine Parallax-Ebene horizontal kachelbar, indem die Naht per Inpainting in ComfyUI
geschlossen wird: Bild um die halbe Breite rollen (Naht liegt dann in der Mitte), mittleren Streifen
maskieren, mit Flux neu malen, zurückrollen.

  python Tools/comfyui/seamfix.py in.png out.png [--band 256] [--prompt "..."] [--ckpt flux1-dev-fp8.safetensors]
Benötigt laufendes ComfyUI (Standard http://127.0.0.1:8188) und Pillow.
"""
import argparse
import io
import json
import time
import urllib.parse
import urllib.request
import uuid

from PIL import Image, ImageFilter

DEFAULT_PROMPT = ("pixel art, 2D side-scroller background layer, seamless continuation of the surrounding scene, "
                  "same style, same colors, same scale, flat orthographic side view, night, no text")


def http(server, path, data=None, headers=None, method=None):
    req = urllib.request.Request(server + path, data=data, headers=headers or {}, method=method)
    with urllib.request.urlopen(req, timeout=120) as r:
        return r.read()


def upload(server, img, name):
    boundary = uuid.uuid4().hex
    buf = io.BytesIO(); img.save(buf, "PNG")
    body = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"image\"; filename=\"{name}\"\r\n"
            f"Content-Type: image/png\r\n\r\n").encode() + buf.getvalue() + f"\r\n--{boundary}\r\nContent-Disposition: form-data; name=\"overwrite\"\r\n\r\ntrue\r\n--{boundary}--\r\n".encode()
    res = json.loads(http(server, "/upload/image", body, {"Content-Type": f"multipart/form-data; boundary={boundary}"}))
    return res["name"]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src"); ap.add_argument("dst")
    ap.add_argument("--server", default="http://127.0.0.1:8188")
    ap.add_argument("--band", type=int, default=256, help="Breite des neu gemalten Streifens in px")
    ap.add_argument("--prompt", default=DEFAULT_PROMPT)
    ap.add_argument("--ckpt", default="flux1-dev-fp8.safetensors")
    ap.add_argument("--lora", default="", help="optionales LoRA (Dateiname), z. B. flux_retro_pixel.safetensors")
    ap.add_argument("--lora-strength", type=float, default=0.85)
    ap.add_argument("--steps", type=int, default=20)
    ap.add_argument("--seed", type=int, default=4242)
    a = ap.parse_args()

    img = Image.open(a.src).convert("RGB")
    w, h = img.size
    half = w // 2
    rolled = Image.new("RGB", (w, h))
    rolled.paste(img.crop((half, 0, w, h)), (0, 0))
    rolled.paste(img.crop((0, 0, half, h)), (w - half, 0))
    mask = Image.new("L", (w, h), 0)
    x0 = (w - a.band) // 2
    mask.paste(255, (x0, 0, x0 + a.band, h))
    # weiche Maskenkante, damit der Übergang nicht hart ist
    mask = mask.filter(ImageFilter.GaussianBlur(a.band / 8))

    stamp = uuid.uuid4().hex[:8]
    img_name = upload(a.server, rolled, f"be_seam_{stamp}.png")
    mask_name = upload(a.server, mask, f"be_seam_{stamp}_mask.png")

    wf = {
        "4": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": a.ckpt}},
        "20": {"class_type": "LoadImage", "inputs": {"image": img_name}},
        "21": {"class_type": "LoadImage", "inputs": {"image": mask_name}},
        "22": {"class_type": "ImageToMask", "inputs": {"image": ["21", 0], "channel": "red"}},
        "23": {"class_type": "VAEEncode", "inputs": {"pixels": ["20", 0], "vae": ["4", 2]}},
        "24": {"class_type": "SetLatentNoiseMask", "inputs": {"samples": ["23", 0], "mask": ["22", 0]}},
        "6": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["4", 1], "text": a.prompt}},
        "7": {"class_type": "ConditioningZeroOut", "inputs": {"conditioning": ["6", 0]}},
        "11": {"class_type": "FluxGuidance", "inputs": {"conditioning": ["6", 0], "guidance": 3.5}},
        "3": {"class_type": "KSampler", "inputs": {"seed": a.seed, "steps": a.steps, "cfg": 1.0, "sampler_name": "euler", "scheduler": "simple", "denoise": 1.0,
                                                   "model": ["4", 0], "positive": ["11", 0], "negative": ["7", 0], "latent_image": ["24", 0]}},
        "8": {"class_type": "VAEDecode", "inputs": {"samples": ["3", 0], "vae": ["4", 2]}},
        "25": {"class_type": "ImageCompositeMasked", "inputs": {"destination": ["20", 0], "source": ["8", 0], "x": 0, "y": 0, "resize_source": False, "mask": ["22", 0]}},
        "9": {"class_type": "SaveImage", "inputs": {"filename_prefix": "be_seamfix/out", "images": ["25", 0]}},
    }
    if a.lora:
        wf["10"] = {"class_type": "LoraLoader", "inputs": {"model": ["4", 0], "clip": ["4", 1], "lora_name": a.lora, "strength_model": a.lora_strength, "strength_clip": a.lora_strength}}
        wf["6"]["inputs"]["clip"] = ["10", 1]
        wf["3"]["inputs"]["model"] = ["10", 0]

    res = json.loads(http(a.server, "/prompt", json.dumps({"prompt": wf, "client_id": "be-seamfix"}).encode(), {"Content-Type": "application/json"}))
    if res.get("node_errors"):
        raise SystemExit("Node-Fehler: " + json.dumps(res["node_errors"], indent=1))
    pid = res["prompt_id"]
    t0 = time.time()
    while True:
        time.sleep(2)
        hist = json.loads(http(a.server, f"/history/{pid}")).get(pid)
        if hist and hist.get("outputs"):
            break
        if hist and hist.get("status", {}).get("status_str") == "error":
            raise SystemExit("Job-Fehler: " + json.dumps(hist["status"].get("messages"), indent=1))
        if time.time() - t0 > 600:
            raise SystemExit("Timeout")
    out = next(iter(hist["outputs"].values()))["images"][0]
    q = urllib.parse.urlencode({"filename": out["filename"], "subfolder": out.get("subfolder", ""), "type": out.get("type", "output")})
    data = http(a.server, "/view?" + q)
    fixed = Image.open(io.BytesIO(data)).convert("RGB")
    # zurückrollen, damit die ursprüngliche Komposition erhalten bleibt
    back = Image.new("RGB", (w, h))
    back.paste(fixed.crop((w - half, 0, w, h)), (0, 0))
    back.paste(fixed.crop((0, 0, w - half, h)), (half, 0))
    back.save(a.dst)
    print(f"{a.dst}: Naht geschlossen ({time.time() - t0:.0f} s)")


if __name__ == "__main__":
    main()
