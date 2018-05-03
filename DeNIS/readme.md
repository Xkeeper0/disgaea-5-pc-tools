To use this, drag `file.dat` onto it. (This also works with `.sav` files.)

You can choose to (d)ecode or (e)ncode.

A new file, `file.dat.dec`, will be created if you choose to decode.

This works on almost all of Disgaea 5 PC's `.dat` and `.sav` files -- some of the larger ones appear to be unencoded already.

--

Decoding:

* XOR every byte with `0xF0`.
* Swap the nybbles of each byte. `0x5A` becomes `0xA5`.

Encoding is the same but in reverse.
