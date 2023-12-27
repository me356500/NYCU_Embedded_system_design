# opencv-face

## opencv-contrib
- Download opencv_contrib - [link](https://github.com/opencv/opencv_contrib/tree/3.4.7)
- 版本為 3.4.7

## Build
大概看一下 opencv-contrib 的 readme

1. `sudo cmake-gui`
2. `OPENCV_EXTRA_MODULES_PATH` 選 opencv_contrib/modules
3. add entry 把要的 flag 加上去打勾, 我只用到 `face` 這個 module
```
BUILD_opencv_legacy=OFF
BUILD_opencv_face=ON

```
- 編的時候有遇到錯的 module, 但我忘記是哪一個, 直接加上他的 flag 然後 disable 就編的過了
```
BUILD_opencv_cvv=OFF
```
- GLES3/gl3.h : No such file or directory
    - Undefine GLES3/gl3.h

## dnn build

1. protobuf issue
    - rm /usr/bin/protoc
    - cp newer version protoc to /usr/bin

2. cmake enable protobuf and dnn

## 112_Project usage

```shell
# real-time
LD_LIBRARY_PATH=. ./demo 1280 720 1 config/yolov3-tiny.cfg config/yolov3-tiny.weights 160 0.1 0.1 0.1

# 12 items ~= 2m30s
time LD_LIBRARY_PATH=. ./performance 0.1 0.1 0.1 config/yolov3.cfg config/yolov3.weights 0 416

# class wide performance ~= 15 second
time LD_LIBRARY_PATH=. ./performance 0.1 0.1 0.1 config/yolov3.cfg config/yolov3.weights 0 128

```