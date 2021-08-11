#!/usr/bin/env python3

import argparse
from PIL import Image
import struct

parser = argparse.ArgumentParser(description="Converts an image to a font bin for nds-bootstrap")
parser.add_argument("input", metavar="igm_font.png", type=str, nargs=1, help="image to convert from")
parser.add_argument("output", metavar="font.bin", type=str, nargs=1, help="file to output to")

args = parser.parse_args()

with Image.open(args.input[0]) as img:
	count = img.size[0] * img.size[1] // 8 // 8

	px = img.load()
	with open(args.output[0], "wb") as file:
		for c in range(count):
			for i in range(8):
				y = (c // 16) * 8 + i
				x = (c % 16) * 8
				p = 0
				for j in range(8):
					p = p | ((px[x + j, y] & 1) << j)
				file.write(struct.pack("B", p))

	print(f"{args.output[0]} created with {count} tiles.")
