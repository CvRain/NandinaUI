"""页面 golden 的最小图像 I/O：读 BMP（探针输出）、读写 PNG（仓库内候选图）。

只实现 golden 需要的子集：未压缩 24/32 位 BMP、8 位 RGBA PNG（编码只写 filter 0；
解码支持 5 种 filter）。不引入第三方依赖，避免为了 golden 往构建里加图像库。
"""

from __future__ import annotations

import struct
import zlib


class Image:
    __slots__ = ("width", "height", "pixels")  # pixels: bytearray RGBA

    def __init__(self, width: int, height: int, pixels: bytearray) -> None:
        self.width = width
        self.height = height
        self.pixels = pixels

    def at(self, x: int, y: int) -> tuple[int, int, int, int]:
        offset = (y * self.width + x) * 4
        return tuple(self.pixels[offset : offset + 4])  # type: ignore[return-value]


def read_bmp(path: str) -> Image:
    data = open(path, "rb").read()
    if data[:2] != b"BM":
        raise ValueError("not a BMP")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    header_size = struct.unpack_from("<I", data, 14)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    bits = struct.unpack_from("<H", data, 28)[0]
    compression = struct.unpack_from("<I", data, 30)[0]
    if header_size < 40 or bits not in (24, 32) or width <= 0 or height == 0:
        raise ValueError(f"unsupported BMP (bits={bits}, compression={compression})")
    # SDL_SaveBMP 对 32 位表面写 BI_BITFIELDS（compression 3）：掩码紧跟 40 字节头。
    masks = None
    if compression == 3 and bits == 32:
        masks = struct.unpack_from("<IIII", data, 54)
    elif compression != 0:
        raise ValueError(f"unsupported BMP compression {compression}")
    top_down = height < 0
    height = abs(height)
    stride = ((width * bits + 31) // 32) * 4
    pixels = bytearray(width * height * 4)
    for row in range(height):
        source_row = row if top_down else height - 1 - row
        row_start = pixel_offset + source_row * stride
        for column in range(width):
            offset = row_start + column * (bits // 8)
            if masks is not None:
                value = struct.unpack_from("<I", data, offset)[0]
                channels = []
                for mask in masks:
                    if mask == 0:
                        channels.append(255)
                        continue
                    shift = (mask & -mask).bit_length() - 1
                    width_bits = mask.bit_length() - shift
                    sample = (value & mask) >> shift
                    channels.append((sample * 255) // ((1 << width_bits) - 1))
                red, green, blue, alpha = channels
            else:
                blue, green, red = data[offset], data[offset + 1], data[offset + 2]
                alpha = data[offset + 3] if bits == 32 else 255
            target = (row * width + column) * 4
            pixels[target : target + 4] = bytes((red, green, blue, alpha))
    return Image(width, height, pixels)


def read_png(path: str) -> Image:
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    offset = 8
    width = height = 0
    compressed = bytearray()
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, depth, color = struct.unpack_from(">IIBB", payload, 0)
            if depth != 8 or color not in (2, 6):
                raise ValueError(f"unsupported PNG (depth={depth}, color={color})")
        elif kind == b"IDAT":
            compressed += payload
        elif kind == b"IEND":
            break
    raw = zlib.decompress(bytes(compressed))
    channels = 3 if color == 2 else 4
    stride = width * channels
    pixels = bytearray(width * height * 4)
    previous = bytearray(stride)
    position = 0
    for row in range(height):
        filter_type = raw[position]
        position += 1
        line = bytearray(raw[position : position + stride])
        position += stride
        if filter_type == 1:
            for index in range(channels, stride):
                line[index] = (line[index] + line[index - channels]) & 0xFF
        elif filter_type == 2:
            for index in range(stride):
                line[index] = (line[index] + previous[index]) & 0xFF
        elif filter_type == 3:
            for index in range(stride):
                left = line[index - channels] if index >= channels else 0
                line[index] = (line[index] + ((left + previous[index]) >> 1)) & 0xFF
        elif filter_type == 4:
            for index in range(stride):
                left = line[index - channels] if index >= channels else 0
                up = previous[index]
                up_left = previous[index - channels] if index >= channels else 0
                estimate = left + up - up_left
                pa, pb, pc = abs(estimate - left), abs(estimate - up), abs(estimate - up_left)
                predictor = left if pa <= pb and pa <= pc else (up if pb <= pc else up_left)
                line[index] = (line[index] + predictor) & 0xFF
        elif filter_type != 0:
            raise ValueError(f"unsupported PNG filter {filter_type}")
        for column in range(width):
            source = column * channels
            target = (row * width + column) * 4
            pixels[target : target + 4] = (
                bytes((line[source], line[source + 1], line[source + 2], 255))
                if channels == 3
                else bytes(line[source : source + 4])
            )
        previous = line
    return Image(width, height, pixels)


def write_png(path: str, image: Image) -> None:
    def chunk(kind: bytes, payload: bytes) -> bytes:
        return (
            struct.pack(">I", len(payload))
            + kind
            + payload
            + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
        )

    rows = bytearray()
    stride = image.width * 4
    for row in range(image.height):
        rows.append(0)
        rows += image.pixels[row * stride : (row + 1) * stride]
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", image.width, image.height, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(bytes(rows), 9))
        + chunk(b"IEND", b"")
    )
    open(path, "wb").write(png)


def load(path: str) -> Image:
    if path.lower().endswith(".png"):
        return read_png(path)
    return read_bmp(path)
