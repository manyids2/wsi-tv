# ssh-wsi

SSH server to serve whole slide images. Made for kitty terminal.

## Installation

```bash
make
```

## Usage

```bash
./wsi-tv <slidepath>
```

## Keys

- `q` - quit
- `j` - down
- `k` - up
- `l` - right
- `h` - left
- `i` - zoom in
- `o` - zoom out
- `1`, `2`, ... - jump to level

## Limitations

- Needs terminal support for `ws_pixel_x` and `ws_pixel_y` with
  `ioctl` request to find terminal width and height in pixels.
- Needs terminal support for [kitty image protocol](https://sw.kovidgoyal.net/kitty/graphics-protocol/).

## Program structure
