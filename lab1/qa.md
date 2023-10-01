https://github.com/hcgcarry/embeddedSystem_nctu


https://github.com/skchen1993/Embedded-System-Design/tree/master
https://github.com/kirito1219/Embedded-system/tree/main
## E9v3

- CPU
    - NXP i.MX6Q

Cortex_A9
https://developer.arm.com/Processors/Cortex-A9

- Network card
    - AR8035
    - https://www.digikey.tw/zh/htmldatasheets/production/2081963/0/0/1/ar8035-al1a
- Sound card
    - SGTL5000

## 5. Questions

Host CPU : x86

1. Explain what is *arm-linux-gnueabihf-gcc* ? Why don't we just compile with gcc ?
    - e9v3 is arm processor, if we use gcc, default compiler will be compatible with our host computer which may not be arm
    - we need cross compiler to generate the arm executable file to e9v3
    - arm-linux-gnueabihf will make sure the compiler will find the correct library and headers.

2. Can executable *hello_world* run on the host computer ? Why or why not ?
    - If our host computer is same arm architecture, then the hello_world can run. Otherwise it can't

## 6. Advanced

1. simply install minicom and sz rz to SD card XD.

2. run performance.c or other tools like perf.

## Transfer

:::warning
We need to encode file before transferring !!
:::


1. e9v3 # ./recv {filename}
2. host # cat hello_world | base64 > b64hello_world
3. check send.py filename = "b64hello_world"
4. host # python send.py
5. e9v3 # base64 -d {filename} > hello_world
6. e9v3 # ./hello_world