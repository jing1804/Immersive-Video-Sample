# Immersive Video OMAF Sample

## Introduction
   The Immersive Video Delivery OMAF Sample provides a quick trial to setup E2E OMAF-Compliant 360 video streaming. OMAF 360 Video streaming sample can support both VOD and Live streaming for 4K and 8K contents. 

## Software Requirements

 - Server OS : CentOS Linux release 7.6.1810 (Core)
 - Client OS : Ubuntu 18.04 LTS
 - Docker version : 1.13.1

## Hardware Tested
| Platform | Server | Client |
|:----:|:----:|:----:|
| CPU SKU | Intel® Xeon® Platinum<br>8280M CPU @ 2.70GHz | Intel® Core™ i7-6770HQ<br>CPU @ 2.60GHz x 8 |
| Memory | 128G | 16G |

## Installation

- [Install docker engine in server](https://docs.docker.com/install)

- Server :
```bash
    cd OMAF-Sample/server && ./deploy.sh <proxy>  # Proxy config is optional.
    docker image ls                               # Created an image. [REPOSITORY:immersive_server, TAG:v1]
```

- Client :
```bash
    cd OMAF-Sample/client && ./deploy.sh
```

## How To Run (HTTPS)

- Server :
```bash
    docker run --privileged -p 5000:443 -p 5001:8080 -it immersive_server:v1 bash  # Map the port.
    cd /usr/local/nginx/conf/
    ./configure.sh CN Shanghai A B C D E@F.com                                     # './configure.sh -h' for details.
    /usr/local/nginx/sbin/nginx                                                    # Start nginx.
    cd /home/immersive/Sample-Videos && ./run.sh <RES> <TYPE>                      # <RES>:[4K,8K] <TYPE>:[LIVE,VOD]
                                                                                   # Press 'q' button to quit.
```

For details in FFmpeg plugins' parameters, refer to the [FFmpeg usage doc](../src/doc/Immersive_Video_Delivery_FFmpeg_usage.md).

- Client :
```bash
    sudo su
    cd Immersive-Video-Sample/src/build/client/player
    export LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH
    vim config.xml  # Set up configuration, details in following table.
    ./render
```
**Config.xml**

   | Parameters | Descriptions | Examples |
| :----:| :----: | :----: |
| windowWidth  | The width of render window | 960 for 4k, 1920 for 8k |
| windowHeight | The height of render window  | 960 for 4k, 1920 for 8k  |
| url | The resource URL path | Remote URL |
| sourceType | Source type | 0 is for Dash Source |
| decoderType | FFmpeg Decoder type | 0 is for software decoder |
| contextType | OpenGL context type | 0 is for glfw |
| useDMABuffer | DMA Buffer flag | 0 means no DMA buffer |
| enableExtractor | extractor track path or later binding path | 1 is for extractor track and 0 is for later binding |
| viewportHFOV | Viewport horizon FOV degree | 80 |
| viewportVFOV | Viewport vertical FOV degree | 80 |
| viewportWidth | Viewport width | 960 for 4k, 1920 for 8k |
| viewportHeight | Viewport height | 960 for 4k, 1920 for 8k |
| cachePath | Cache path | /home/media/cache |
| predict | viewport prediction plugin | 0 is disable and 1 is enable |

   - **Note** : So far, some parameters settings are limited. URL need to be a remote dash source URL, choose `./run.sh 8K LIVE` for example : `https://xxx.xxx.xxx.xxx:5000/LIVE8K/Test.mpd`. The parameter sourceType must set to 0, which represents dash source. The parameter decoderType must set to 0, which stands for FFmpeg software decoder. The parameter contextType need to be 0, which represents glfw context. And useDMABuffer flag should be set to 0. 
