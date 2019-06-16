# YabaSanshiro for RetroArena

## How to build:

### Get Source Code

```
$ git clone https://github.com/devmiyax/yabause.git
$ cd yabause
$ git submodule update --init --recursive
$ mkdir build
$ cd build
```

### Generate Makefile for ODROID XU4

```
$ cmake ../yabause -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DYAB_WANT_ARM7=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/xu4.cmake -DCMAKE_INSTALL_PREFIX=/opt/retroarena/emulators/yabause/
```

### Generate Makefile for ODROID N2

```
$ cmake ../yabause -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/n2.cmake -DCMAKE_INSTALL_PREFIX=/opt/retroarena/emulators/yabause/
```

### Generate Makefile for RockPro64

```
$ cmake ../yabause -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DYAB_WANT_ARM7=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/rp64.cmake -DCMAKE_INSTALL_PREFIX=/opt/retroarena/emulators/yabause/
```

### Build

```
make
```
Ater that you are ready to run ```./src/retro_arena/yabasanshiro``` .

## Commandline Usage:

|           |                               |                                              |
|-----------|-------------------------------|----------------------------------------------|
| -b STRING | --bios STRING                 | bios file                                    |
| -i STRING | --iso STRING                  | iso/cue file                                 |
| -r NUMBER | --resolution_mode NUMBER      | 0 .. Native, 1 .. 4x, 2 .. 2x, 3 .. Original |
| -a        | --keep_aspect_rate            | |
| -s NUMBER | --scps_sync_per_frame NUMBER  | |
| -nf        | --no_frame_skip              |  disable frame skip |
| -v        | --version                     | | 
| -h        |                               | Show help information |

## KeyConfig file:

Key config file is sotred as ~/.yabasanshiro/keymap.json .

Sample file format

```json
{
  "player1": {
    "DeviceID": 0,
    "deviceGUID": "03000000550900001072000011010000",
    "deviceName": "NVIDIA Controller v01.03",
    "pdmode": 0, 
    "up": { "id": 0,"type": "hat","value": 1 },    
    "down": { "id": 0, "type": "hat", "value": 4 },
    "left": { "id": 0, "type": "hat", "value": 8 },
    "right": { "id": 0,"type": "hat","value": 2 },
    "select": {"id": 14,"type": "button","value": 1 },
    "start": {"id": 7,"type": "button","value": 1 },
    "a": { "id": 0, "type": "button", "value": 1 },
    "b": { "id": 1, "type": "button", "value": 1 },
    "c": { "id": 5, "type": "button", "value": 1 },
    "x": {"id": 2,"type": "button", "value": 1 },
    "y": { "id": 3,"type": "button", "value": 1 },
    "z": { "id": 4,"type": "button", "value": 1  },
    "l": { "id": 5, "type": "axis", "value": 1  },
    "r": { "id": 5,"type": "axis","value": 1 },
    "analogleft": { "id": 4, "type": "axis", "value": 0 },
    "analogright": { "id": 5, "type": "axis", "value": 0 },
    "analogx": { "id": 0, "type": "axis", "value": 0  },
    "analogy": { "id": 1, "type": "axis", "value": 0  }
  }
}
```

## Special functions for Retropie:

* Synchronize input settings with ~/.emulationstation/es_temporaryinput.cfg, if you don't have ~/.yabasanshiro/keymap.json .

| es_temporaryinput.cfg | YabaSanshiro                  |
|-----------------------|-------------------------------|
| up                    | up |
| down                  | down |
| left                  | left |
| right                 | right |
| a                     | a |
| b                     | b |
| rightshoulder         | c |
| a                     | x |
| b                     | y |
| leftshoulder          | z |
| lefttrigger           | l |
| righttrigger          | r |
| start                 | start |

'select' is force to mapped as 'Show/Hide Menu' button
