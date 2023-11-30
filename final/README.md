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
BUILD_opencv_face
```
- 編的時候有遇到錯的 module, 但我忘記是哪一個, 直接加上他的 flag 然後 disable 就編的過了
