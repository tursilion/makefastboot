Source code for a new encrypted boot, intended to be embedded within makefastboot.

To create it:

-if you modify the code, be aware of the areas that the CD boot modifies. Most of it is currently outside of the code area. If you put code in those areas your cartridge will probably crash if it's started from a JagCD.
-build the source code with the enclosed makefile
-the bootcode.o contains the code with a 32-byte header, strip that header. The code is exactly 132 bytes long (unless you extended it), so trim all data after that point. Even though the actual code is 130 bytes, it appears the jag needs it rounded up to work right. Save the trimmed as bootcode.rawbin
-Run jagcrypt.exe -x -tursi bootcode.rawbin -- this will encrypt the header without needing a cart or patching the various bits normally modified.
-open bootcode.XXX and trim to exactly 132 bytes long
-Convert bootcode.xxx to C-style data with bin2inc or a similar tool, and paste it into makefastboot. Alternately, you can paste the first 132 bytes of bootcode.XXX on top of any cartridge, HOWEVER, you must set the checksum information or it will not boot.

The checksum information lives at offset $0410 in the cart (after the boot information). It consists of 3 32-bit words:

-start address to checksum
-number of 32-bit words to checksum
-checksum value + 0x03d0dead (Jag's magic boot value)

The checksum range is subtracted from the provided checksum with the intent that if there are no bad contacts it will end up with 0x03d0dead, which the Jaguar needs to boot. Any bad reads will then cause the traditional red screen.

If you don't want a checksum, you can set a single value (you can't have zero dwords checked). If you choose an address that contains all zeros or all FFs, it's very easy. For instance, you could select $00800420 (right after the checksum header), which is normally all FF. $FFFFFFFF is negative one, so this at $410 would pass and boot (without verifying the cart):

00 80 04 20 00 00 00 01 03 D0 DE AC
----------- ----------- -----------
|           |           |
start adr   |           |
      number of dwords  |
                   starting checksum

It will subtract negative one (which adds 1) from $03d0deac and get the correct value needed to boot. But the checksum is quick and makefastboot will calculate it for you, so I don't recommend skipping it. You'll have fewer crashes caused by dirty contacts. ;)
