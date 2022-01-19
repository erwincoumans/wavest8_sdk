// Copyright 2021 Erwin Coumans All Rights Reserved.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This example shows how to parse a Wavestate file (.wsperf etc)

#include <iostream>
#include "Bank/Performance.pb.h"
#include "Storage/FileInfo.pb.h"
#include "Storage/ExtendedFileInfo.pb.h"
#include "command_line_args.h"
#include "VoiceModels/Wavest8/Wavest8Model.pb.h"

using namespace std;

const char* parse_korg_proto(const char* orgbuf, int size, const string& type)
{
    cout << "Parsing: " << type << endl;
    const char* buf = orgbuf;
    orgbuf += size;

    if (type == "ExtendedFileInfo")
    {
        ::KorgProtocolMessage::ExtendedFileInfo ext_file_info;
        ext_file_info.ParseFromString(string(buf, size));
        cout << ext_file_info.DebugString() << endl;
    }
    if (type == "Item" || type == "Items")
    {
        buf += 4;
        int sz = *(int*)buf;
        buf += 4;

        ::KorgProtocolMessage::FileInfo file_info2;
        file_info2.ParseFromString(string(buf, sz));
        buf += sz;
        cout << file_info2.DebugString() << endl;

        for (int j = 0; j < file_info2.payload_info_size(); j++)
        {
            auto payload_item = file_info2.payload_info()[j];
            cout << "type:" << payload_item.type() << endl;
            int num_bytes = *(int*)buf;
            buf += 4;
            buf = parse_korg_proto(buf, num_bytes, payload_item.type());

        }
        orgbuf = buf;
    }
    if (type == "Performance")
    {
        ::KorgProtocolMessage::Performance perf;
        perf.ParseFromString(string(buf,size));
        //perf.set_name("cute");
        cout << perf.DebugString() << endl;
        int track_index = 0;
        for (int track_index = 0; track_index < perf.track_size(); track_index++)
        {
            assert(track_index < 4);
            auto track = perf.track()[track_index];
            if (track.has_instrument_track())
            {
                if (track.instrument_track().has_common())
                {

                }
                if (track.instrument_track().has_program())
                {
                    for (auto patch : track.instrument_track().program().patch())
                    {
                        ::KorgProtocolMessage::Wavest8Model model;
                        if (model.ParseFromString(patch.data()))
                        {
                            cout << model.DebugString() << endl;
                        }
                    }
                }
            }
        }
    }

    if (type == "ObjectInfo")
    {
        ::KorgProtocolMessage::ObjectInfo ob_info;
        ob_info.ParseFromString(string(buf, size));
        cout << ob_info.DebugString() << endl;
    }
    return orgbuf;
}

int main(int argc, char* argv[]) {
    //////////////////////////////////////////////////////////////
    // Initializer connection, exit
    //////////////////////////////////////////////////////////////
    string file_name = "APeacefulDay.wsperf";
    CommandLineArgs cargs(argc, argv);
    cargs.GetCmdLineArgument("file_name", file_name);
    
    FILE* f = fopen(file_name.c_str(), "rb");
    if (f)
    {
        fseek(f, 0L, SEEK_END);
        int szf = ftell(f);
        fseek(f, 0L, SEEK_SET);
        char* orgbuf = new char[szf];
        const char* buf = orgbuf;
        fread(orgbuf, 1, szf, f);
        buf += 4;
        int sz1 = *(int*)buf;
        buf += 4;
        ::KorgProtocolMessage::FileInfo file_info;
        file_info.ParseFromString(string(buf, sz1));
        cout << file_info.DebugString() << endl;
        buf += sz1;
        for (int i = 0; i < file_info.payload_info_size(); i++)
        {
            const ::KorgProtocolMessage::PayloadInfo& info = file_info.payload_info()[i];
            cout << "type:" << info.type() << endl;
            int count = info.num_items() > 0 ? info.num_items() : 1;

            for (int j = 0; j < count; j++)
            {
                int num_bytes = *(int*)buf;
                buf += 4;
                buf = parse_korg_proto(buf, num_bytes, info.type());

            }
        }
    }
    else
    {
        cout << "cannot open file: " << file_name << endl;
    }
}