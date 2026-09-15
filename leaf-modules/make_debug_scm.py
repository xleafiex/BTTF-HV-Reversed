"""Minimal Miami SCM using this checkout's command numbering and float encoding."""
import re
import struct
from pathlib import Path

def build(repo):
    names = re.findall(r'^\s*(COMMAND_\w+)', (repo/'src/control/ScriptCommands.h').read_text(), re.M)
    op = {name: i for i, name in enumerate(names)}
    def integer(n): return b'\x01'+struct.pack('<i', n)
    def real(n): return b'\x06'+struct.pack('<f', n)
    def cmd(name, *args): return struct.pack('<H', op['COMMAND_'+name])+b''.join(args)
    data = bytearray(160)
    data[:7] = cmd('GOTO', integer(128))
    data[128:135] = cmd('GOTO', integer(140))
    data[140:147] = cmd('GOTO', integer(160))
    # Zero object/mission counts; global variables occupy bytes 8..127.
    pos = tuple(real(n) for n in (-378.0, -539.0, 10.0))
    data += cmd('CREATE_PLAYER', integer(0), *pos, b'\x02\x08\x00')
    data += cmd('LOAD_SCENE', *pos)
    data += cmd('REQUEST_COLLISION', real(-378.0), real(-539.0))
    data += cmd('LOAD_ALL_MODELS_NOW')
    # Resolve the ground only after the destination's collision is loaded.
    data += cmd('SET_PLAYER_COORDINATES', integer(0), real(-378.0), real(-539.0), real(-100.0))
    data += cmd('SET_TIME_OF_DAY', integer(12), integer(0))
    data += cmd('SET_PLAYER_CONTROL', integer(0), integer(1))
    data += cmd('SWITCH_WIDESCREEN', integer(0))
    data += cmd('RESTORE_CAMERA_JUMPCUT')
    data += cmd('SET_CAMERA_BEHIND_PLAYER')
    data += cmd('DO_FADE', integer(500), integer(1))
    loop = len(data)
    data += cmd('WAIT', integer(1000))
    data += cmd('GOTO', integer(loop))
    struct.pack_into('<I', data, 148, len(data))
    return bytes(data)
