#
# Copyright 2021 Erwin Coumans LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
 
#import sys
import os, inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(os.path.dirname(currentdir))
os.sys.path.insert(0, currentdir)
os.sys.path.insert(0, parentdir)

import argparse
import os
import io
import copy
import Storage.FileInfo_pb2
import Storage.ExtendedFileInfo_pb2
import Bank.Performance_pb2
import Bank.Program_pb2
import SubRate.Components.RhythmicStepGenerator.RhythmicStepGenerator_pb2 as RhythmicStepGenerator_pb2
import SubRate.VoiceModels.VoiceModelPatch_pb2 as VoiceModelPatch_pb2
import VoiceModels.Wavest8.Wavest8Model_pb2 as Wavest8Model_pb2
import Effects.Overb.OverbEffect_pb2 as OverbEffect_pb2
import Effects.Compressor.CompressorEffect_pb2 as CompressorEffect_pb2
import Effects.Chorus.ChorusEffect_pb2 as ChorusEffect_pb2
import Effects.ParametricEQ.ParametricEQEffect_pb2 as ParametricEQEffect_pb2
import Effects.LCRDelay.LCRDelayEffect_pb2 as LCRDelayEffect_pb2

import Panel.Wavest8PerformancePanelState_pb2 as Wavest8PerformancePanelState_pb2
import Storage.ObjectInfo_pb2 as ObjectInfo_pb2

parser = argparse.ArgumentParser(description='Read and Write Korg Wavestate files')
parser.add_argument('--file_name', type=str, default="prebuild/erwin_organ_performance.wsperf")
args = parser.parse_args()

file_length_in_bytes = os.path.getsize(args.file_name)

expected_header="Korg"
name_id = 1

def read_and_check_header(header):
  if (header==expected_header):
    print("Header OK:", header)
  else:
    print("Error: expected header", expected_header, "instead of ", header)
    exit(0)
      
def parse_proto(proto_type, num_bytes, fd):
  print("+++++++++++++++")
  global name_id
  data = fd.read(num_bytes)
  #print(proto_type, "data=",data)
  if (proto_type=="ObjectInfo"):
    obj_info = Storage.ObjectInfo_pb2.ObjectInfo()
    obj_info.ParseFromString(data)
    print("MyObjectInfo=",obj_info)
    
  if (proto_type=="ExtendedFileInfo"):
    ext_info = Storage.ExtendedFileInfo_pb2.ExtendedFileInfo()
    ext_info.ParseFromString(data)
    print("ExtendedFileInfo=",ext_info)
  print("proto_type=",proto_type)
  if (proto_type=="Item") or (proto_type=="Items"):
    #expect another "Korg" header
    fd_mem = io.BytesIO(data)
    header = (fd_mem.read(4)).decode("utf-8")
    read_and_check_header(header)
    sz = int.from_bytes(fd_mem.read(4), byteorder='little')
    print("Item sz=",sz)
    item_file_info = Storage.FileInfo_pb2.FileInfo()
    item_file_info.ParseFromString(fd_mem.read(sz))
    for info in item_file_info.payload_info:
      print("---------------")
      print("item info=",info)
      print("item info.type=",info.type)
      print("item info.version=",info.version)
      print("item info.num_items=",info.num_items)
      num_bytes = int.from_bytes(fd_mem.read(4), byteorder='little')
      print("item num_bytes=",num_bytes)
      
      parse_proto(info.type, num_bytes, fd_mem)
      
  if (proto_type=="Performance"):
    perf = Bank.Performance_pb2.Performance()
    perf.ParseFromString(data)
    
    #prog.track (repeated)
    # each track 
    #   name
    #   has oneof 'track_type
    #     instrument_track
    #     external_instrument_track
    #     audio_track
    #     aux_track

    
    #note that Wavest8Model data is stored as 'bytes' which is not human readable
    #so we make a copy, clear the field, print that, and then manually print the 
    #Wavest8Model
    perf_copy = copy.deepcopy(perf)
    #print(dir(perf_copy))
    for track in perf_copy.track:
      print("track.name=",track.name)
      if (hasattr(track,"instrument_track")):
        #print("track.instrument_track=",track.instrument_track)
        #print("track.instrument_track.Program", track.instrument_track.Program)
        prog = track.instrument_track.Program
        
        for effect in prog.effect_rack.effect:
          print("effect.name=", effect.name)
          if effect.type_uuid==b"Korg\000\000\000\000\000\010\000\000\016\000\000\000":
            print("MyCompressor:")
            compress = CompressorEffect_pb2.CompressorEffect()
            fd_compress = io.BytesIO(effect.data)
            compress.ParseFromString(fd_compress.read())
            human_tag = "compress["+str(name_id)+"]"
            print(human_tag, compress)
            effect.data = str.encode(human_tag)
            name_id=name_id+1
          if effect.type_uuid==b"Korg\000\000\000\000\000\010\000\000\r\000\000\000":
            print("MyStereoChorus:")
            chorus = ChorusEffect_pb2.ChorusEffect()
            fd_chorus = io.BytesIO(effect.data)
            chorus.ParseFromString(fd_chorus.read())
            human_tag = "chorus["+str(name_id)+"]"
            print(human_tag, chorus)
            effect.data = str.encode(human_tag)
            name_id=name_id+1
          if effect.type_uuid==b"Korg\000\000\000\000\000\010\000\000&\000\000\000":
            print("LCRDelayEffect:")
            lcr = LCRDelayEffect_pb2.LCRDelayEffect()
            fd_lcr = io.BytesIO(effect.data)
            lcr.ParseFromString(fd_lcr.read())
            human_tag = "LCRDelay["+str(name_id)+"]"
            print(human_tag,lcr)
            effect.data = str.encode(human_tag)
            name_id=name_id+1
          #todo, map the other type_uuid to matching effects, in the Effects folder
              
          
        for patch in prog.patch:
          print("patch.name=", patch.name)
          print("patch.type_uuid=",patch.type_uuid)
          #with open("type_uuid_"+str(name_id), "wb") as fd:
          #  fd.write(patch.type_uuid)
          
          if len(patch.data):
            #print("patch.data=", patch.data)
            #print("len(patch.data)=", len(patch.data))
            my_file_info = Wavest8Model_pb2.Wavest8Model()
            fd_mem2 = io.BytesIO(patch.data)
            my_file_info.ParseFromString(fd_mem2.read())
            human_tag = "Wavest8_data["+str(name_id)+"]"
            print(human_tag, my_file_info)
            patch.data = str.encode(human_tag)
            #with open("patch_"+str(name_id), "wb") as fd:
            #  fd.write(patch.data)
            name_id=name_id+1
    
    #interpret reverb effect. todo, map other effect.type_uuid to one of the proto's in Effects
    for chan in perf_copy.audio_output_channel:
      fx = chan.pre_fader_effect_rack.effect
      print("Channel has ", len(fx), "effects")
      for effect in fx:
        print("effect.type_uuid=",effect.type_uuid)
          
        if effect.type_uuid==b"Korg\000\000\000\000\000\010\000\0004\000\000\000":
          #print("Overb type!")
          overb = OverbEffect_pb2.OverbEffect()
          fd_overb = io.BytesIO(effect.data)
          overb.ParseFromString(fd_overb.read())
          human_tag = "overb["+str(name_id)+"]"
          print(human_tag,overb)
          effect.data = str.encode(human_tag)
          name_id=name_id+1
          
        if effect.type_uuid==b"Korg\000\000\000\000\000\010\000\0006\000\000\000":
          #print("ParametricEQEffect!")
          eq = ParametricEQEffect_pb2.ParametricEQEffect()
          fd_eq = io.BytesIO(effect.data)
          eq.ParseFromString(fd_eq.read())
          human_tag = "MasterEQ["+str(name_id)+"]"
          print(human_tag,eq)
          effect.data = str.encode(human_tag)
          name_id=name_id+1
        print("effect=", effect)
      #print(dir(chan))
    
    panel = Wavest8PerformancePanelState_pb2.WavestatePerformancePanelState()
    fd_panel = io.BytesIO(perf_copy.panel_state.state_data)
    panel.ParseFromString(fd_panel.read())
    human_tag = "panel_state["+str(name_id)+"]"
    print(human_tag,panel)
    perf_copy.panel_state.state_data = str.encode(human_tag)
    name_id=name_id+1
    
    #perf_copy has the binary blobs substituted with protobuf data
    print("Performance:", perf_copy)  
    

    
  if (proto_type=="Program"):
    prog = Bank.Program_pb2.Program()
    prog.ParseFromString(data)
    print("Program:", prog)
    
      
  if (proto_type=="TimingLane"):
    timing_lane = RhythmicStepGenerator_pb2.TimingLane()
    timing_lane.ParseFromString(data)
    print("timing_lane=",timing_lane)

    

with open(args.file_name, "rb") as fd:
    fd.seek(0, 0)
    header = (fd.read(4)).decode("utf-8")
    read_and_check_header(header)
    
    sz = int.from_bytes(fd.read(4), byteorder='little')
    my_file_info = Storage.FileInfo_pb2.FileInfo()
    my_file_info.ParseFromString(fd.read(sz))
    for info in my_file_info.payload_info:
      print("---------------")
      print("info=",info)
      print("info.type=",info.type)
      print("info.version=",info.version)
      print("info.num_items=",info.num_items)
      #check, if num_items == 0, we still need to parse ExtendedFileInfo etc.
      if info.num_items>0:
        for item in range (info.num_items):
          num_bytes = int.from_bytes(fd.read(4), byteorder='little')
          parse_proto(info.type, num_bytes, fd)
      else:
        num_bytes = int.from_bytes(fd.read(4), byteorder='little')
        parse_proto(info.type, num_bytes, fd)
      
    
    
