# BLT XAudioAPI

XAudio API (named before I found out that's what DirectX's audio system is called), or
the eXtra Audio API, is an OpenAL-based API to allow mods to use positional audio. In the future
it will also likely support HRTF, allowing for more accurate 3D sound.

# Units
In some parts of this API, units probably won't matter if they're consistent.

However, in other parts they will. Use meters as your standard unit.

# API:
`blt.xaudio`:
- `setup()`: Run this to start XAudio. Without it, IDK exactly what will
happen, but the game will very likely crash. Calling this multiple times is not an issue.
- `loadbuffer(filename...)`: Create (or load, if cached) buffers containing the contents of
the specified OGG files.
- `newsource([num])`: Creates an audio source. By default this does not have any sounds connected. You
may optionally specify a number of sources to create, and they will be returned as individual arguments.

Buffer:
- `close()`: Closes this buffer. Probably a bit buggy, I haven't yet tested it. If you don't close it, the
buffer will be reused if you later request the same file and will automatically be closed when the game exits.

Audio Source:
- `close()`: Closes this audio source. Probably a bit buggy, I haven't yet tested it. If you don't close it, it will
automatically be destroyed when the game exits.
- `setbuffer(buffer)`: Sets this source to play whatever audio is contained by the specified buffer.
- `play()`: Play this audio source.
- `setposition(x,y,z)`: Sets the position of this audio source in 3D space.
- `setvelocity(x,y,z)`: Sets the velocity of this audio source. This is solely used for applying the doppler effect (stuff moving
towards and away from you have differnt pitches).
- `setdirection(x,y,z)`: Sets the direction vector of this sound. Currently doesn't (seem to) do anything.

`blt.xaudio.listener`:
- `setposition(x,y,z)`: Sets the position in 3D space of the player.
- `setvelocity(x,y,z)`: Sets the velocity of the player. As per audio sources, this is used for the doppler effect.
- `setorientation(x,y,z, x,y,z)`: Sets the orientation of the player, with a forward-up vector pair.

# Sample

```
blt.xaudio.setup()

local buff = blt.xaudio.loadbuffer(ModPath .. "test.ogg")

local src = blt.xaudio.newsource()
src:setbuffer(buff)
src:setposition(-1, 0, 0)
src:play()

blt.xaudio.listener:setposition(-2, 0, 0)
blt.xaudio.listener:setorientation(1, 0, 0,  0, 1, 0)
```
