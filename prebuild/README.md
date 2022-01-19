Creating an 'SDK' for the Wavestate:

install https://github.com/marin-m/pbtk

Run pbtk on the Wavestate Editor/Librarian and/or the Wavest8 executable (on the Wavestate Raspberry Pi compute module).
Use one or both sets of proto files, generate Python and/or C++ bindings. You can use those to read/modify/write files.
You can also generate rpcz bindings (together with zeromq) to connect to Wavestate live over networked USB (RNDIS and NCM)

How to access /Korg/Wavest8 executable?
Buy a RPI3 compute module or 'root' the Wavestate using the unofficial wavest8 editor:

wavest8_editor_2021april11.exe --root_wavestate
then connect to the Wavestate and press the ROOT button in the upper right corner
From then you can ssh into your wavest8 using
ssh root@Korgwavestate1.local


Another example of using such SDK:
wavestate_parse_file.exe --file_name="erwin_organ_performance.wsperf" > output.txt

It will extract all data from a wsperf file (only a few structs haven't been mapped yet)
