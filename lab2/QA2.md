

## QA

1. what are cmake and make for ? what is the relation between them ? 
    - cmake is a cross-platform build tools, making project can be build in different compiler or system
    - CMakeLists.txt can used to defined structure and flags, cmake can generate different building files including Makefile
    - Make is a tool used in Unix-like system
    - cmake set and build the Makefile, and Makefile 

2. Why there are so many argument ?
    - we need to get right headers and library
    - -I, specifies included header files
    - -L, specify library files
    - -Wl,-rpath-link, specify the runtime library search paths
    - -lpthread, compiler link POSIX thread library
    - -lopencv_world, compiler link OPENCV library

3. What is libopencv_world.so.3.4.7 for ? Why do we need to use LD_LIBRARY_PATH. ./demo to run the executable ? What would happen if we just run with ./demo ? Why?
    - The libopencv_world.so.3.4.7 library is a part of the OpenCV library, which is used for computer vision and image processing tasks in C++ and other languages. This specific library file corresponds to version 3.4.7 of OpenCV. libopencv_world is a single, large shared library that includes many OpenCV modules, making it easier to use OpenCV in projects without worrying about linking multiple smaller libraries.
    - LD_LIBRARY_PATH is an environment variable that tells the system where to look for shared libraries (such as libopencv_world.so.3.4.7) when running an executable. 
    - If you run ./demo without setting LD_LIBRARY_PATH, the loader will only search for shared libraries in the default system library paths. If libopencv_world.so.3.4.7 is not located in one of those paths, you may encounter an error similar to "shared library not found" or "undefined symbol" when running ./demo. This is because the loader cannot locate the required libopencv_world.so.3.4.7 library.

4. It is so complex and difficult to show a picture by using the framebuffer. Why don't we just use cv::imshow() to do this work ?
    - (not sure) System Resources: In some embedded or resource-constrained environments, using a lightweight framebuffer approach can be more efficient in terms of memory and processing power compared to running a full-featured graphical library like OpenCV.


5. What is framebuffer ?
    - https://ecomputernotes.com/computer-graphics/basic-of-computer-graphics/what-is-frame-buffer
    - A frame buffer is a large, contiguous piece of computer memory. At a minimum  there is one memory bit for each pixel in the rater; this amount of memory is called a bit  plane. The picture is built up in the frame buffer one bit at a time.