# Lab4

## Basic

### HDMI output
according to TQIMX6_uboot

- uboot menu
    - Press ```whitespace ``` before ```Hit any key to stop autoboot: 1``` shows.

- setting boot args
    - Press ```0``` at uboot menu

- Switch fb0 output to HDMI
    - uboot --> 0 --> 3 --> 1 --> 2 --> s --> q

- Switch fb1 output to others
    - uboot --> 0 --> 3 --> 2 --> (output) --> s --> q

Note : remember press ```s``` before ```q``` in every step.

### Center video output

```cpp=
// screenshot.cpp

/*
    ___________________________
    |                         |   180
    |_________________________|
    |     |             |     |
    | 320 |    Video    | 320 |   720
    |_____|_____________|_____|
    |                         |   
    |_________________________|   180

      320      1280       320
*/

for (int i = 180; i < frame_size.height + 180; ++i) {
    ofs.seekp(i * pixel_bytes * fb_width + 320 * 2);
    ofs.write(reinterpret_cast<char*>(frame.ptr(i - 180)), pixel_bytes * frame_size.width);
}  

```

## Advance

### 5.1 Performance

- Loop unroll may help
- Since the top and the bottom side of picture.png are all black, we can only flush the center part of framebuffer.

```cpp=
// advance1.cpp

for (int i = 360; i < 720; ++i) {
    ofs.seekp(i * 2 * fb_info.xres_virtual);
    ofs.write(reinterpret_cast<char*>(frame.ptr(i, shift)), 2 * 1920);
}

```

### 5.2 Control

```lab4.cpp```
