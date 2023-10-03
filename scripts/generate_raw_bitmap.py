#!/usr/bin/env python
import pathlib
from argparse import ArgumentParser
from PIL import Image

INKPLATE_WIDTH = 1200
INKPLATE_HEIGHT = 825


def main():
    args_parser = ArgumentParser()
    args_parser.add_argument('input', help='path to input file', type=pathlib.Path)
    args_parser.add_argument('output', help='path to output file', type=pathlib.Path)
    args_parser.add_argument("--mode", choices=["1bpp", "4bpp"], default="1bpp", help="file format version")

    args = args_parser.parse_args()

    # 1. load
    image = Image.open(args.input)

    # 2. resize
    image = image.resize((INKPLATE_WIDTH, INKPLATE_HEIGHT))

    # 3. dither
    if args.mode == "1bpp":
        image = image.convert("1")

        # 4. save 1bpp
        pixels = image.getdata()
        with open(args.output, "wb") as output_file:
            for pixel_i in range(0, len(pixels), 8):
                byte = 0
                for bit_i in range(8):
                    pixel = pixels[pixel_i + bit_i]
                    if pixel == 0:
                        byte |= 1 << bit_i
                output_file.write(byte.to_bytes(1, byteorder="big"))
    else:
        image = image.convert("L", palette=Image.ADAPTIVE, colors=8)

        # 4. save 4bpp
        pixels = image.getdata()
        with open(args.output, "wb") as output_file:
            for pixel_i in range(0, len(pixels), 2):
                byte = 0
                for bit_i in range(2):
                    pixel = pixels[pixel_i + bit_i] // 32
                    byte |= pixel << (bit_i * 4)
                output_file.write(byte.to_bytes(1, byteorder="big"))


if __name__ == '__main__':
    main()
