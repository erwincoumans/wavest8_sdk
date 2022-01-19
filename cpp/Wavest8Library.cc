#include "Wavest8Library.h"

#include <iostream>
#include "rpcz/rpcz.hpp"
#include "API/Message/ClientMessages.rpcz.h"
#include "API/Message/SoundLibraryMessages.rpcz.h"
#include "API/Message/ObjectMessages.rpcz.h"
#include "API/Message/ParameterDescriptorMessages.rpcz.h"
#include "API/Message/FileSystemMessages.rpcz.h"
#include "API/Message/EventMessages.rpcz.h"
#include "API/Message/LicenseManagerMessage.rpcz.h"
#include "API/Message/SoundLibraryMessages.rpcz.h"
#include "Bank/Performance.pb.h"
#include "Storage/FileInfo.pb.h"
#include "Storage/ExtendedFileInfo.pb.h"
#include "ProductMessages.rpcz.h"
#include "command_line_args.h"
#include "VoiceModels/Wavest8/Wavest8Model.pb.h"
#include "modulation_enums.h"
#include "InitWaveSeq.h"
#include "multisample.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>

#endif

#undef GetMessage
#undef GetMessageA
//don't add this line before including windows.h when using C++17 or above
//see https://developercommunity.visualstudio.com/content/problem/93889/error-c2872-byte-ambiguous-symbol.html
using namespace std;

  
struct Wavest8LibraryDetails
{
    rpcz::application m_application;
    rpcz::rpc_channel* m_chan1;
    bool m_is_connected;
    ::KorgProtocolMessage::ClientService_Stub* m_client_stub;
    ::KorgProtocolMessage::EventService_Stub* m_events_stub;
    ::KorgProtocolMessage::FileSystemService_Stub* m_file_system_stub;
    ::KorgProtocolMessage::SoundLibraryService_Stub* m_sound_library_stub;
    ::KorgProtocolMessage::ObjectService_Stub* m_object_stub;
    ::KorgProtocolMessage::Wavest8::ProductService_Stub* m_product_mes_stub;
    ::KorgProtocolMessage::ParameterDescriptorService_Stub* m_parameter_descriptor_stub;
    ::KorgProtocolMessage::FileInfo file_info;
    ::KorgProtocolMessage::FileInfo file_info3;
    ::KorgProtocolMessage::ExtendedFileInfo ext_file_info;
    ::KorgProtocolMessage::Performance m_perf;
    ::KorgProtocolMessage::ObjectInfo m_ob_info;

    ::google::protobuf::int64 m_view_id;
    
    std::vector<::KorgProtocolMessage::SoundDatabase::Record> m_wave_sequences;
    std::vector<std::string> m_wave_sequence_names;
    std::vector<std::string> m_wave_sequence_uuids;

    std::vector<::KorgProtocolMessage::SoundDatabase::Record> m_programs;
    std::vector<std::string> m_program_names;
    std::vector<std::string> m_program_uuids;
    std::vector <int> m_program_row_ids;

    ::google::protobuf::int64 m_row_count;
    ::google::protobuf::int64 m_actual_row_count;
    std::vector<std::string> m_performance_names;
    
    
    std::vector <int> m_performance_row_ids;
    
    bool m_is_perf_valid;
    bool m_is_ob_info_valid;

    int m_deadline_ms;
    int m_client_id;

    Wavest8LibraryDetails()
        :m_is_connected(false),
        m_is_perf_valid(false),
        m_is_ob_info_valid(false)
    {

    }
};
  
 

Wavest8Library::Wavest8Library(int deadline_ms)
{
    m_details = new Wavest8LibraryDetails();
    m_details->m_deadline_ms = deadline_ms;
    
    
}

Wavest8Library::~Wavest8Library()
{
    disconnect();
    delete m_details;
}

void Wavest8Library::connect()
{
    if (!m_details->m_is_connected)
    {
        //cout << "create_rpc_channel" << endl;

        m_details->m_chan1 = m_details->m_application.create_rpc_channel(
            "tcp://korgwavestate1.local:50000");
        if (m_details->m_chan1)
        {
            //cout << "chan ok";
        }
        else
        {
            //cout << "chan failure";
        }
        m_details->m_client_stub = new ::KorgProtocolMessage::ClientService_Stub(m_details->m_chan1, false);
        m_details->m_events_stub = new ::KorgProtocolMessage::EventService_Stub(m_details->m_chan1, false);
        m_details->m_file_system_stub = new ::KorgProtocolMessage::FileSystemService_Stub(m_details->m_chan1, false);
        m_details->m_sound_library_stub = new ::KorgProtocolMessage::SoundLibraryService_Stub(m_details->m_chan1, false);
        m_details->m_object_stub = new ::KorgProtocolMessage::ObjectService_Stub(m_details->m_chan1, false);
        m_details->m_product_mes_stub = new ::KorgProtocolMessage::Wavest8::ProductService_Stub(m_details->m_chan1, false);
        m_details->m_parameter_descriptor_stub = new ::KorgProtocolMessage::ParameterDescriptorService_Stub(m_details->m_chan1, false);

        {
            ::KorgProtocolMessage::ClientIdentifyServerRequest request;
            ::KorgProtocolMessage::ClientIdentifyServerResponse response;

            //cout << "Sending request." << endl;
            try {
                m_details->m_client_stub->IdentifyServer(request, &response, m_details->m_deadline_ms);

                //cout << response.DebugString() << endl;
                //cout << response.api_name() << endl;
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                return;
            }
        }

        {
            ::KorgProtocolMessage::ClientAddRequest add_request;
            add_request.set_name("wavestate Sound Librarian");
            ::KorgProtocolMessage::ClientAddResponse add_response;
            try {
                m_details->m_client_stub->Add(add_request, &add_response, m_details->m_deadline_ms);
                //cout << add_response.DebugString() << endl;
                //cout << add_response.client_id() << endl;
                m_details->m_client_id = add_response.client_id();
                
                m_details->m_is_connected = true;
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                return;
            }
        }
        create_view();

        ::KorgProtocolMessage::GetTotalWaveMotionDataSizeRequest request;
        ::KorgProtocolMessage::GetTotalWaveMotionDataSizeResponse response;
        try
        {
            m_details->m_sound_library_stub->GetTotalWaveMotionDataSize(request,&response,m_details->m_deadline_ms);
            ::google::protobuf::uint64 sz = response.size_bytes();
            //cout << response.DebugString() <<endl;
        }
        catch(rpcz::rpc_error& e) {
            cout << "Error: " << e.what() << endl;;
            return;
        }


    }
}

bool Wavest8Library::get_current_performance(std::string& performance,std::uint64_t& object_id)
{
    std::string object_path = "|Current Performance|Select";
    get_object_path(object_path,performance,object_id);
    //cout << performance << endl;
    return true;
}

bool Wavest8Library::select_performance(std::int64_t row_id)
{

    bool result = false;
    ::google::protobuf::uint64 index_min = -1;
    ::google::protobuf::uint64 index_max = -1;

    ::google::protobuf::int64 value_descriptor_id = -1;
    ::KorgProtocolMessage::Value value;
    ::google::protobuf::uint64 object_id = -1;
    ::KorgProtocolMessage::GetRowIDOfIndexResponse row_id_response;

    if(1)
    {
        {
            ::KorgProtocolMessage::GetDigestRequest digest_request;
            digest_request.mutable_object_path()->set_path("|Current Performance|Select");// | wavestate | Set List Select");

            ::KorgProtocolMessage::GetDigestResponse digest_response;
            try {
                m_details->m_object_stub->GetDigest(digest_request,&digest_response,m_details->m_deadline_ms);
                //cout << digest_response.DebugString() << endl;
                value_descriptor_id = digest_response.value_descriptor_id();
                value = digest_response.value();

                object_id = digest_response.object_id();
            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                return false;
            }
        }

    }

    {
        ::KorgProtocolMessage::SetValueRequest  value_request;
        ::KorgProtocolMessage::SetValueResponse value_response;

        value_request.mutable_object_path()->set_id(object_id);
        value_request.mutable_value()->set_int_64_value(row_id);

        try
        {
            m_details->m_object_stub->SetValue(value_request,&value_response,m_details->m_deadline_ms);
            cout << value_response.DebugString() << endl;
            result = true;
        }
        catch(rpcz::rpc_error& e) {
            cout << "Error: " << e.what() << endl;;
            return false;
        }
    }


#if 0
    //this doesn't seem to work
    std::string object_path = "|wavestate|Set List Slot";
    std::string output_name;
    std::uint64_t object_id;

    //get_object_path(object_path,output_name,object_id);


    ::google::protobuf::uint64 index_min = -1;
    ::google::protobuf::uint64 index_max = -1;

    ::google::protobuf::int64 value_descriptor_id = -1;
    ::KorgProtocolMessage::Value value;
    
    ::KorgProtocolMessage::GetRowIDOfIndexResponse row_id_response;

    if(1)
    {
        {
            ::KorgProtocolMessage::GetDigestRequest digest_request;
            digest_request.mutable_object_path()->set_path("|wavestate|Set List Slot");

            ::KorgProtocolMessage::GetDigestResponse digest_response;
            try {
                m_details->m_object_stub->GetDigest(digest_request,&digest_response,m_details->m_deadline_ms);
                //cout << digest_response.DebugString() << endl;
                value_descriptor_id = digest_response.value_descriptor_id();
                value = digest_response.value();

                object_id = digest_response.object_id();
            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }

    }





    cout << output_name << endl;
    {
        ::KorgProtocolMessage::SetValueRequest  value_request;
        ::KorgProtocolMessage::SetValueResponse value_response;

        value_request.mutable_object_path()->set_id(object_id);
        value_request.mutable_value()->set_int_64_value(row_id);

        try
        {
            m_details->m_object_stub->SetValue(value_request,&value_response,m_details->m_deadline_ms);
            //cout << value_response.DebugString() << endl;
            result = true;
        }
        catch(rpcz::rpc_error& e) {
            cout << "Error: " << e.what() << endl;;

        }
    }
#endif
    return result;
    
}

void Wavest8Library::disconnect()
{
    if (m_details->m_is_connected)
    {
        //////////////////////////////////////////////////////////////
    // Terminate connection, exit
    //////////////////////////////////////////////////////////////
        {
            try {
                ::KorgProtocolMessage::ClientRemoveRequest remove_request;
                remove_request.set_client_id(m_details->m_client_id);
                ::KorgProtocolMessage::ClientRemoveResponse remove_response;
                m_details->m_client_stub->Remove(remove_request, &remove_response, m_details->m_deadline_ms);
                //cout << remove_response.DebugString() << endl;
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;;
                
            }
        }

        m_details->m_application.terminate();
        m_details->m_is_connected = false;

    }
}
bool Wavest8Library::is_connected()
{
    return m_details->m_is_connected;
}

void Wavest8Library::send_raw_midi_message(std::string msg)
{
    if(!is_connected())
        return;

    ::KorgProtocolMessage::RawMidiMessageRequest midi_req;
    midi_req.set_midi_message(msg);

    ::KorgProtocolMessage::RawMidiMessageResponse midi_response;
    try {
        m_details->m_events_stub->RawMidiMessage(midi_req,&midi_response,m_details->m_deadline_ms);
        //cout << note_on_response.DebugString() << endl;
    }
    catch(rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;

    }
    
}

void Wavest8Library::note_on_off(int note_val, int channel, int velocity, bool on)
{
    if (!is_connected())
        return;

    if (on)
    {
        ::KorgProtocolMessage::NoteOnRequest note_on_request;

        note_on_request.set_note(note_val);
        note_on_request.set_channel(channel);
        note_on_request.set_velocity(velocity);

        ::KorgProtocolMessage::NoteOnResponse note_on_response;
        try {
            m_details->m_events_stub->NoteOn(note_on_request, &note_on_response, m_details->m_deadline_ms);
            //cout << note_on_response.DebugString() << endl;
        }
        catch (rpcz::rpc_error& e) {
            cout << "Error: " << e.what() << endl;
            
        }
    }
    else
    {

        ::KorgProtocolMessage::NoteOffRequest note_off_request;
        note_off_request.set_note(note_val);
        note_off_request.set_channel(channel);
        note_off_request.set_velocity(velocity);

        ::KorgProtocolMessage::NoteOffResponse note_off_response;
        try {
            m_details->m_events_stub->NoteOff(note_off_request, &note_off_response, m_details->m_deadline_ms);
            //cout << note_off_response.DebugString() << endl;
        }
        catch (rpcz::rpc_error& e) {
            cout << "Error: " << e.what() << endl;
            
        }
    }
}



bool Wavest8Library::get_object_path(const std::string& object_path, std::string& output_name,std::uint64_t& object_id)
{
    bool result = true;
    prevent_activity(true);
    ::google::protobuf::int64 value_descriptor_id = -1;
    ::KorgProtocolMessage::Value value;
    //::google::protobuf::uint64 object_id = -1;
    ::google::protobuf::uint64 view_id = -1;
    ::google::protobuf::uint64 row_count = -1;
    ::google::protobuf::uint64 actual_row_count = -1;


    ::KorgProtocolMessage::GetDigestRequest digest_request;
    digest_request.mutable_object_path()->set_path(object_path);// | wavestate | Set List Select);

    ::KorgProtocolMessage::GetDigestResponse digest_response;
    try {
        m_details->m_object_stub->GetDigest(digest_request, &digest_response, m_details->m_deadline_ms);
        //cout << digest_response.DebugString() << endl;
        value_descriptor_id = digest_response.value_descriptor_id();
        value = digest_response.value();
        object_id = digest_response.object_id();
    }
    catch (rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        result = false;
        prevent_activity(false);
        return false;
    }






    {
        {
            ::KorgProtocolMessage::GetParameterDescriptorDigestRequest param_request;
            param_request.set_value_descriptor_id(value_descriptor_id);
            ::KorgProtocolMessage::GetParameterDescriptorDigestResponse param_response;
            try {
                m_details->m_parameter_descriptor_stub->GetDigest(param_request, &param_response, m_details->m_deadline_ms);
                //cout << param_response.DebugString() << endl;

            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                result = false;
                prevent_activity(false);
                return false;
            }
        }

        {
            ::KorgProtocolMessage::ConvertValueToTextRequest convert_request;
            convert_request.set_value_descriptor_id(value_descriptor_id);
            convert_request.mutable_value()->CopyFrom(value);
            ::KorgProtocolMessage::ConvertValueToTextResponse convert_response;
            try {
                m_details->m_parameter_descriptor_stub->ConvertValueToText(convert_request, &convert_response, m_details->m_deadline_ms);
                
                //cout << convert_response.DebugString() << endl;
                //cout << "Current Selected Performance:" << convert_response.text() << endl;
                output_name = convert_response.text();
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                result = false;
                prevent_activity(false);
                return false;
            }

            if (0)
            {
                ::KorgProtocolMessage::WriteToBufferRequest write_to_buffer_request;
                write_to_buffer_request.mutable_object_path()->set_id(object_id);
                ::KorgProtocolMessage::WriteToBufferResponse write_to_buffer_response;

                try {
                    m_details->m_object_stub->WriteToBuffer(write_to_buffer_request, &write_to_buffer_response, m_details->m_deadline_ms);
                    //cout << write_to_buffer_response.DebugString() << endl;
                }
                catch (rpcz::rpc_error& e) {
                    cout << "Error: " << e.what() << endl;
                    result = false;
                }
            }
        }
    }
    prevent_activity(false);
    return result;

}
void Wavest8Library::export_all()
{
    if (!is_connected())
        return;

    ::KorgProtocolMessage::ExportAllRequest export_all_request;
    //export_all_request.mutable_folder_path()->assign("/Korg/")
    //by default, files end up in /Korg/FileSystem/External/Export

    ::KorgProtocolMessage::ExportAllResponse export_all_response;

    try {
        m_details->m_sound_library_stub->ExportAll(export_all_request, &export_all_response, 32 * 1000 * 1000);
        //cout << export_all_response.DebugString() << endl;

    }
    catch (rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        
    }
}

void Wavest8Library::get_all_collection_info()
{
    if (!is_connected())
        return;

    ::KorgProtocolMessage::GetAllCollectionInfoRequest  collection_request;
    ::KorgProtocolMessage::GetAllCollectionInfoResponse collection_response;
    try {
        m_details->m_sound_library_stub->GetAllCollectionInfo(collection_request, &collection_response, m_details->m_deadline_ms);
        //cout << collection_response.DebugString() << endl;

        //collection_response.collection_info()
    }
    catch (rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        
    }
}

void Wavest8Library::get_instruments(std::vector<std::string>& samples)
{
    if (!is_connected())
        return;

    ::KorgProtocolMessage::GetInstrumentsRequest instrument_request;
    instrument_request.set_data_type(::KorgProtocolMessage::SoundDatabase::DATA_TYPE_MULTISAMPLE);
    ::KorgProtocolMessage::GetInstrumentsResponse instrument_response;

    try {
        
        m_details->m_sound_library_stub->GetInstruments(instrument_request, &instrument_response, m_details->m_deadline_ms);
        //cout << instrument_response.DebugString() << endl;
        int num_instruments = instrument_response.num_presets();
        //cout << "num_instruments" << num_instruments << endl;
        for (int i = 0; i < instrument_response.main_category_size(); i++)
        {
            std::string cat = instrument_response.main_category(i);
            //cout << "instrument" << std::to_string(i) << cat << endl;
        }


    }
    catch (rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        
    }
}


void Wavest8Library::get_multi_sample_list(std::vector<std::string>& samples, std::vector<std::string>& uuids)
{
    ::KorgProtocolMessage::GetCollectionMultiSampleListResponse multisample_response;

    if(!is_connected())
    {
#ifdef READ_FROM_FILE
        //try loading from bin file
        const char* prefix[] ={"./", "../", "../../", "../../../", "../../../../"};
        int numprefix = sizeof(prefix) / sizeof(const char*);
        FILE* f=0;
        for(int i = 0; f==0 && i < numprefix; i++)
        {
            char relativeFileName[1024];
            sprintf(relativeFileName,"%s%s",prefix[i],"data/multisamplelist.bin");
            f = fopen(relativeFileName,"rb");
        }
        if(f)
        {
            fseek(f,0L,SEEK_END);
            int szf = ftell(f);
            if(szf)
            {
                fseek(f,0L,SEEK_SET);
                char* orgbuf = new char[szf];
                const char* buf = orgbuf;
                fread(orgbuf,1,szf,f);
#else
        {
            {

                auto& samplefile = bin2cpp::getMultisampleFile();
                const char* buf = samplefile.getBuffer();
                int szf = samplefile.getSize();

#endif
                std::string data(buf,szf);
                if(multisample_response.ParseFromString(data))
                {
                    for(int i = 0; i < multisample_response.uuid_name_size(); i++)
                    {
                        //std::string new_name = std::to_string(i)+std::string(": ")+multisample_response.uuid_name()[i].name();
                        samples.push_back(multisample_response.uuid_name()[i].name());
                        auto uuid = multisample_response.uuid_name()[i].uuid();
                        int len = uuid.length();
                        uuids.push_back(uuid);
                    }
                }
#ifdef READ_FROM_FILE
                delete[] orgbuf;
#endif
            }
#ifdef READ_FROM_FILE
            fclose(f);
            f=0;
#endif
        }
        return;
    }

    ::KorgProtocolMessage::GetCollectionMultiSampleListRequest multisample_request;
   

    //cout << "GetCollectionMultiSampleList" << endl;

    try {
        m_details->m_sound_library_stub->GetCollectionMultiSampleList(multisample_request, &multisample_response, m_details->m_deadline_ms);
        //cout << multisample_response.DebugString() << endl;
        std::string mem_buf;
        if(multisample_response.SerializeToString(&mem_buf))
        {
            int sz = mem_buf.size();
            FILE* f = fopen("data/multisamplelist.bin","wb");
            if(f)
            {
                fwrite(mem_buf.c_str(),mem_buf.length(),1,f);
                fclose(f);
            }
        }
        for (int i = 0; i < multisample_response.uuid_name_size(); i++)
        {
            //std::string new_name = std::to_string(i)+std::string(": ")+multisample_response.uuid_name()[i].name();
            samples.push_back(multisample_response.uuid_name()[i].name());//multisample_response.uuid_name()[i].name());
            auto uuid = multisample_response.uuid_name()[i].uuid();
            int len = uuid.length();
            uuids.push_back(uuid);
        }
        
    }
    catch (rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        
    }
}

void Wavest8Library::show_popup_message(const char* msg, int time_ms)
{
    if (!is_connected())
        return;

    ::KorgProtocolMessage::Wavest8::ShowPopupMessageRequest product_request;
    ::KorgProtocolMessage::Wavest8::ShowPopupMessageResponse product_response;
    product_request.set_text(msg);
    product_request.set_time_ms(time_ms);
    try {
        m_details->m_product_mes_stub->ShowPopupMessage(product_request, &product_response, m_details->m_deadline_ms);
        //cout << product_response.DebugString() << endl;
    }
    catch (rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        
    }
}

void Wavest8Library::prevent_activity(bool prevent)
    //block activity, during database update operations
{
    if (!is_connected())
        return;

    ::KorgProtocolMessage::Wavest8::PreventActivityRequest prevent_request;
    prevent_request.set_prevent(prevent);
    ::KorgProtocolMessage::Wavest8::PreventActivityResponse prevent_response;
    try {
        m_details->m_product_mes_stub->PreventActivity(prevent_request, &prevent_response, m_details->m_deadline_ms);
        //cout << prevent_response.DebugString() << endl;
    }
    catch (rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        
    }
}



const char* Wavest8Library::parse_korg_proto(const char* orgbuf, int size, const string& type)
{
    //cout << "Parsing: " << type << endl;
    const char* buf = orgbuf;
    orgbuf += size;

    if (type == "ExtendedFileInfo")
    {
        
        m_details->ext_file_info.ParseFromString(string(buf, size));
        //cout << m_details->ext_file_info.DebugString() << endl;
    }
    if (type == "Item" || type == "Items")
    {
        buf += 4;
        int sz = *(int*)buf;
        buf += 4;

        m_details->file_info3.ParseFromString(string(buf, sz));
        buf += sz;
        //cout << m_details->file_info3.DebugString() << endl;

        for (int j = 0; j < m_details->file_info3.payload_info_size(); j++)
        {
            auto payload_item = m_details->file_info3.payload_info()[j];
            //cout << "type:" << payload_item.type() << endl;
            int num_bytes = *(int*)buf;
            buf += 4;
            buf = parse_korg_proto(buf, num_bytes, payload_item.type());

        }
        orgbuf = buf;
    }
    if (type == "Performance")
    {
        m_details->m_perf = ::KorgProtocolMessage::Performance();
        if (m_details->m_perf.ParseFromString(string(buf, size)))
        {
            //cout << m_details->m_perf.DebugString() << endl;
            m_details->m_is_perf_valid = true;
        } else
        {
            m_details->m_is_perf_valid = false;
        }
        
    }

    if (type == "ObjectInfo")
    {
        

        m_details->m_ob_info = ::KorgProtocolMessage::ObjectInfo();
        if (m_details->m_ob_info.ParseFromString(string(buf, size)))
        {
            //cout << m_details->m_ob_info.DebugString() << endl;
            m_details->m_is_ob_info_valid = true;
        }
        else
        {
            //cout << "Cannot parse ObjectInfo" << endl;
            m_details->m_is_ob_info_valid = false;
        }
        
    }
    return orgbuf;
}

bool Wavest8Library::import_file(const std::string& file_name)
{
    FILE* f = fopen(file_name.c_str(), "rb");
    if (f)
    {
        fseek(f, 0L, SEEK_END);
        int szf = ftell(f);
        fseek(f, 0L, SEEK_SET);
        char* orgbuf = new char[szf];
        const char* buf = orgbuf;
        fread(orgbuf, 1, szf, f);
        if (strncmp("Korg",orgbuf,4))
        {
            cout << "Not a wsperf file" << endl;
        }
        else
        {
            buf += 4;
            int sz1 = *(int*)buf;
            buf += 4;
            
            m_details->file_info.ParseFromString(string(buf, sz1));
            //cout << m_details->file_info.DebugString() << endl;
            buf += sz1;
            for (int i = 0; i < m_details->file_info.payload_info_size(); i++)
            {
                const ::KorgProtocolMessage::PayloadInfo& info = m_details->file_info.payload_info()[i];
                //cout << "type:" << info.type() << endl;
                int count = info.num_items() > 0 ? info.num_items() : 1;

                for (int j = 0; j < count; j++)
                {
                    int num_bytes = *(int*)buf;
                    buf += 4;
                    buf = parse_korg_proto(buf, num_bytes, info.type());
                }
            }
        }
        if(orgbuf)
            delete[] orgbuf;
        fclose(f);
    }
    else
    {
        cout << "cannot open file: " << file_name << endl;
        return false;
    }

    return true;
}

bool Wavest8Library::get_object_info(Wavest8ObjectInfo& ob_info)
{
    if (m_details->m_is_ob_info_valid)
    {
        ob_info.m_name = m_details->m_ob_info.name();
        return true;
    }
    return false;
}
bool Wavest8Library::set_object_info(const Wavest8ObjectInfo& ob_info)
{
    if (m_details->m_is_ob_info_valid)
    {
        m_details->m_ob_info.mutable_name()->assign(ob_info.m_name);
    }
    return false;
}


void dump_protobuf_message(::google::protobuf::Message& message, int spacing, std::vector<std::string>& type_str)
{
    bool verbose=true;
    const ::google::protobuf::Descriptor *desc       = message.GetDescriptor();
    const ::google::protobuf::Reflection *refl       = message.GetReflection();
    int fieldCount= desc->field_count();
    if(verbose)
    {
        for(int i=0;i<spacing;i++)
        {
            fprintf(stderr," ");
        }
        fprintf(stderr,"The fullname of the message is %s \n",desc->full_name().c_str());
    }
    for(int i=0;i<fieldCount;i++)
    {
        const ::google::protobuf::FieldDescriptor *field = desc->field(i);
        if(verbose)
        {
            for(int i=0;i<spacing;i++)
            {
                fprintf(stderr," ");
            }
            fprintf(stderr,"The name of the %i th element is %s and the type is  %s \n",i,field->name().c_str(),field->type_name());
            if(field->name()=="effect")
            {
                cout << "test" << endl;
            }
            if(!field->is_repeated())
            {
                switch(field->cpp_type())
                {
                case ::google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                {
                    float val = refl->GetFloat(message,field);
                    cout << to_string(val) << endl;
                    break;
                }
                case ::google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                {
                    float val = refl->GetInt32(message,field);
                    cout << to_string(val) << endl;
                    break;
                }
                case ::google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                {
                    std::string str = refl->GetString(message,field);
                    cout << str << endl;
                    break;
                }

                default:
                {
                    cout << "unknown type:" << to_string(field->cpp_type()) << endl;
                }
                };
            }
            else
            {

            }
        }
        if(field->name()=="modulation_list")
        {
            for(int f=0;f<type_str.size();f++)
            {
                fprintf(stderr,type_str[f].c_str());
            }
            fprintf(stderr,"\n");
        }

        if(field->type()==::google::protobuf::FieldDescriptor::TYPE_MESSAGE)
        {
            std::vector<std::string> type_str_copy = type_str;
            type_str_copy.push_back(field->name()+std::string("::"));
            if(!field->is_repeated())
            {
                const ::google::protobuf::Message &mfield = refl->GetMessage(message,field);
                
                ::google::protobuf::Message *mcopy = mfield.New();
                mcopy->CopyFrom(mfield);
                //void *ptr = new std::shared_ptr<::google::protobuf::Message>(mcopy);
                //std::shared_ptr<::google::protobuf::Message> *m =
                //    static_cast<std::shared_ptr<::google::protobuf::Message> *>(ptr);
                

                dump_protobuf_message(*mcopy, spacing+2,type_str_copy);
            }
            else
            {
                for(int j = 0; j < refl->FieldSize(message,field); j++) {
                    dump_protobuf_message(*refl->MutableRepeatedMessage(&message,field,j), spacing+2,type_str_copy);
                }
            }
        }
    }
}


bool Wavest8Library::get_performance_info(Wavest8PerformanceInfo& perf_info)
{
    int pitch_tune_counts[4]={0};
    int pitch_octave_counts[4]={0};
    int amp_attack_counts[4] ={0};
    int amp_decay_counts[4] ={0};
    int filter_polysix_frequency[4] ={0};

    if (m_details->m_is_perf_valid)
    {
        perf_info.m_write_protected = m_details->m_ob_info.write_protected();

        //std::string txt= m_details->m_perf.DebugString();

#if 0

        cout << "start dumping message field types" << endl;
        cout << "---------------------------------" << endl;

        {
            std::vector<std::string> type_str;
            dump_protobuf_message(m_details->m_perf, 0,type_str);
        }
        cout << "---------------------------------" << endl;
        cout << "end dumping message field types" << endl;

#endif


        
        perf_info.m_name = m_details->m_perf.name();
        perf_info.m_tempo = m_details->m_perf.tempo().value();
        
#if 0
        for(int a=0;a<m_details->m_perf.audio_output_channel_size();a++)
        {
            auto audio_track = m_details->m_perf.audio_output_channel(a);
            if(audio_track.has_pre_fader_effect_rack())
            {
                for(int s=0;s<audio_track.mutable_pre_fader_effect_rack()->effect_size();s++)
                {
                    auto effect = audio_track.mutable_pre_fader_effect_rack()->effect(s);
                    cout << effect.DebugString() <<endl;
                }
            }
            if(audio_track.has_post_fader_effect_rack())
            {
                for(int s=0;s<audio_track.mutable_post_fader_effect_rack()->effect_size();s++)
                {
                    auto effect = audio_track.mutable_post_fader_effect_rack()->effect(s);
                    cout << effect.DebugString() <<endl;
                }

            }
        }
#endif   


        for (int track_index = 0; track_index < m_details->m_perf.track_size();track_index++)
        {
            
            
            //clear the track of old data
            perf_info.m_tracks[track_index] = Wavest8PerformanceTrackInfo();
            assert(track_index < 4);
            auto track = m_details->m_perf.track()[track_index];
#if 0
            if(track.has_audio_track())
            {
                auto audio_track = track.mutable_audio_track();
                if(audio_track->has_effect_rack())
                {
                    auto effect_rack = audio_track->mutable_effect_rack();
                    for(int e=0;e<effect_rack->effect_size();e++)
                    {
                        auto effect = effect_rack->mutable_effect(e);
                        cout << effect->type_uuid() << endl;
                        cout << effect->DebugString() << endl;
                    }
                }
            }
#endif
            
            


            if (track.has_instrument_track())
            {
                
                if (track.instrument_track().has_common())
                {
                    perf_info.m_tracks[track_index].m_enabled_track = (track.instrument_track().common().enable_track().value())? 1 : 0;
                }
                else
                {
                    cout << "no instrument track common, does this happen?\n" << endl;
                }
                if (track.instrument_track().has_program())
                {
                    perf_info.m_tracks[track_index].m_program_name = track.instrument_track().program().name();
                    perf_info.m_tracks[track_index].m_program_copied_from_uuid = track.instrument_track().program().copied_from_uuid();

                    //track.instrument_track().program().effect_rack().effect(1).data();

                    perf_info.m_tracks[track_index].m_hold_note.value = track.instrument_track().program().voice_assignment_settings().hold().value()? 1 : 0;
#if 0
                    for(int m=0;m<track.instrument_track().program().voice_assignment_settings().hold().modulation_list().modulation_size();m++)
                    {
                        IntModulation mod(
                        mod.m_modulation_source = track.instrument_track().program().voice_assignment_settings().hold().modulation_list().modulation(m).source().type();
                        mod.m_intensity = 1;
                        mod.m_intensity_mod_source = 0;
                        perf_info.m_tracks[track_index].m_hold_note.m_modulations.push_back(mod);
                    }
#endif

                    int num_patches = track.instrument_track().program().patch_size();
                    if (num_patches>0)
                    {
                        auto patch = track.instrument_track().program().patch()[0];
                        
                        ::KorgProtocolMessage::Wavest8Model model;
                        if (model.ParseFromString(patch.data()))
                        {
                            
                            if(model.has_filter())
                            {
                                perf_info.m_tracks[track_index].m_filter.m_filter_type = model.filter().filter_type_case();
                                const ::KorgProtocolMessage::ModulationList* mod_list_frequency_cutoff = 0;
                                switch(model.filter().filter_type_case())
                                    {
                                    case ::KorgProtocolMessage::Filter::kBypassFilter:
                                    {
                                        if (model.filter().has_bypass_filter())
                                        {
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kTwoPoleLowPassFilter:
                                    {
                                        if(model.filter().has_two_pole_low_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().two_pole_low_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value = model.filter().two_pole_low_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kFourPoleLowPassFilter:
                                    {
                                        if(model.filter().has_four_pole_low_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().four_pole_low_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().four_pole_low_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kTwoPoleResonantLowPassFilter:
                                    {
                                        if(model.filter().has_two_pole_resonant_low_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().two_pole_resonant_low_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().two_pole_resonant_low_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kTwoPoleResonantHighPassFilter:
                                    {
                                        if(model.filter().has_two_pole_resonant_high_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().two_pole_resonant_high_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().two_pole_resonant_high_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kTwoPoleResonantBandPassFilter:
                                    {
                                        if(model.filter().has_two_pole_resonant_band_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().two_pole_resonant_band_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().two_pole_resonant_band_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kTwoPoleResonantBandRejectFilter:
                                    {
                                        if(model.filter().has_two_pole_resonant_band_reject_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().two_pole_resonant_band_reject_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().two_pole_resonant_band_reject_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kFourPoleResonantLowPassFilter:
                                    {
                                        if(model.filter().has_four_pole_resonant_low_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().four_pole_resonant_low_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().four_pole_resonant_low_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kFourPoleResonantHighPassFilter:
                                    {
                                        if(model.filter().has_four_pole_resonant_high_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().four_pole_resonant_high_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().four_pole_resonant_high_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kFourPoleResonantBandPassFilter:
                                    {
                                        if(model.filter().has_four_pole_resonant_band_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().four_pole_resonant_band_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().four_pole_resonant_band_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kFourPoleResonantBandRejectFilter:
                                    {
                                        if(model.filter().has_four_pole_resonant_band_reject_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().four_pole_resonant_band_reject_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().four_pole_resonant_band_reject_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kTwoPoleMultiFilter:
                                    {
                                        if(model.filter().has_two_pole_multi_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().two_pole_multi_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().two_pole_multi_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kMs20HighPassFilter:
                                    {
                                        if(model.filter().has_ms20_high_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().ms20_high_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().ms20_high_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kMs20LowPassFilter:
                                    {
                                        if(model.filter().has_ms20_low_pass_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().ms20_low_pass_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                                model.filter().ms20_low_pass_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kPolysixFilter:
                                    {
                                        if(model.filter().has_polysix_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().polysix_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value = 
                                                model.filter().polysix_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    case ::KorgProtocolMessage::Filter::kOdysseyRev3Filter:
                                    {
                                        if(model.filter().has_odyssey_rev_3_filter())
                                        {
                                            mod_list_frequency_cutoff = &model.filter().odyssey_rev_3_filter().common().frequency_semitones().modulation_list();
                                            perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value = 
                                                model.filter().odyssey_rev_3_filter().common().frequency_semitones().value();
                                        }
                                        break;
                                    }
                                    default:
                                    {
                                    }
                                    }
                                if(mod_list_frequency_cutoff)
                                {
                                    for(int m=0;m<mod_list_frequency_cutoff->modulation_size();m++)
                                    {
                                        auto org_mod = mod_list_frequency_cutoff->modulation()[m];
                                        int uid = track_and_id_to_uid(track_index,MOD_TARGET_FILTER_CUTOFF_FREQUENCY);
                                        uid+= filter_polysix_frequency[track_index]++;
                                        FloatModulation mod = CreateFilterCutoffFrequencyModulation(uid);
                                        mod.m_intensity = org_mod.intensity().float_value()/mod.m_scaling_factor;
                                        mod.m_modulation_source = org_mod.source().type();
                                        mod.m_intensity_mod_source = org_mod.intensity_mod_source().type();
                                        perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.m_modulations.push_back(mod);
                                    }
                                }
                            }

                            perf_info.m_tracks[track_index].m_pitch.m_tune.value = model.mutable_pitch()->mutable_tune_cents()->value();
                            for(int m=0;m<model.mutable_pitch()->mutable_tune_cents()->modulation_list().modulation_size();m++)
                            {
                                auto org_mod = model.mutable_pitch()->mutable_tune_cents()->modulation_list().modulation()[m];
                                int uid = track_and_id_to_uid(track_index,MOD_TARGET_PITCH_TUNE);
                                uid+= pitch_tune_counts[track_index]++;
                                FloatModulation mod = CreatePitchTuneModulation(uid);
                                mod.m_intensity = org_mod.intensity().float_value()/mod.m_scaling_factor;
                                mod.m_modulation_source = org_mod.source().type();
                                mod.m_intensity_mod_source = org_mod.intensity_mod_source().type();
                                perf_info.m_tracks[track_index].m_pitch.m_tune.m_modulations.push_back(mod);
                            }

                            perf_info.m_tracks[track_index].m_pitch.m_transpose_octaves.value = model.mutable_pitch()->mutable_transpose_octaves()->value();
                            for(int m=0;m<model.mutable_pitch()->mutable_transpose_octaves()->modulation_list().modulation_size();m++)
                            {
                                auto org_mod = model.mutable_pitch()->mutable_transpose_octaves()->modulation_list().modulation()[m];
                                int uid = track_and_id_to_uid(track_index,MOD_TARGET_PITCH_OCTAVE);
                                uid+= pitch_octave_counts[track_index]++;
                                IntModulation mod = CreatePitchOctaveModulation(uid);
                                mod.m_intensity = org_mod.intensity().int_value();
                                mod.m_modulation_source = org_mod.source().type();
                                mod.m_intensity_mod_source = org_mod.intensity_mod_source().type();
                                perf_info.m_tracks[track_index].m_pitch.m_transpose_octaves.m_modulations.push_back(mod);
                            }


                            perf_info.m_tracks[track_index].m_adrs_attack.value = model.mutable_amp_eg()->mutable_attack_time_secs()->value();
                            for(int i=0;i<model.mutable_amp_eg()->mutable_attack_time_secs()->modulation_list().modulation_size();i++)
                            {
                                int uid = track_and_id_to_uid(track_index,MOD_TARGET_AMP_ATTACK);
                                uid+= amp_attack_counts[track_index]++;
                                FloatModulation mod = CreateAMPAttackModulation(uid);
                                mod.m_modulation_source = model.mutable_amp_eg()->mutable_attack_time_secs()->modulation_list().modulation(i).source().type();
                                mod.m_intensity = model.mutable_amp_eg()->mutable_attack_time_secs()->modulation_list().modulation(i).intensity().float_value();
                                mod.m_intensity_mod_source = model.mutable_amp_eg()->mutable_attack_time_secs()->modulation_list().modulation(i).intensity_mod_source().type();
                                perf_info.m_tracks[track_index].m_adrs_attack.m_modulations.push_back(mod);
                            }
                                                       
                            perf_info.m_tracks[track_index].m_adrs_decay.value = model.mutable_amp_eg()->mutable_decay_time_secs()->value();
                            for(int i=0;i<model.mutable_amp_eg()->mutable_decay_time_secs()->modulation_list().modulation_size();i++)
                            {
                                int uid = track_and_id_to_uid(track_index,MOD_TARGET_AMP_DECAY);
                                uid+= amp_decay_counts[track_index]++;
                                FloatModulation mod = CreateAMPDecayModulation(uid);
                                mod.m_modulation_source = model.mutable_amp_eg()->mutable_decay_time_secs()->modulation_list().modulation(i).source().type();
                                mod.m_intensity = model.mutable_amp_eg()->mutable_decay_time_secs()->modulation_list().modulation(i).intensity().float_value();
                                mod.m_intensity_mod_source = model.mutable_amp_eg()->mutable_decay_time_secs()->modulation_list().modulation(i).intensity_mod_source().type();
                                perf_info.m_tracks[track_index].m_adrs_decay.m_modulations.push_back(mod);
                            }


                            perf_info.m_tracks[track_index].m_adrs_sustain = model.mutable_amp_eg()->mutable_sustain_level_percent()->value();
                            perf_info.m_tracks[track_index].m_adrs_release = model.mutable_amp_eg()->mutable_release_time_secs()->value();
                            perf_info.m_tracks[track_index].m_trigger_at_note_on = model.mutable_common()->mutable_trigger_on_key_off()->value();

                            //cout << model.DebugString() << endl;
                            if (model.osc().has_multi_sample_set())
                            {
                                
                                for (auto zone : model.osc().multi_sample_set().velocity_zone())
                                {
                                    
                                    if (zone.has_multisample_reference())
                                    {
                                        
                                        //cout << "multisample" << endl;
                                        std::string uuid = *zone.mutable_multisample_reference()->mutable_uuid();
                                        perf_info.m_tracks[track_index].m_multi_sample_instrument_uuid = uuid;
                                        perf_info.m_tracks[track_index].m_has_wave_sequence = 0;
                                        
                                        //cout << uuid << endl;
                                    }
                                    if (zone.has_wave_sequencer())
                                    {
                                        perf_info.m_tracks[track_index].m_has_wave_sequence = 1;
                                        std::string wavesequuid = zone.wave_sequencer().copied_from_uuid();
                                        perf_info.m_tracks[track_index].use_master_lane = zone.wave_sequencer().rhythmic_step_generator().use_master().value();
                                        //see https://github.com/erwincoumans/wavest8sdk/blob/ff091a07354747a3263c3ce56c013fbb824143c5/SubRate/Components/RhythmicStepGenerator/RhythmicStepGenerator.proto
                                        auto step_gen = zone.wave_sequencer().rhythmic_step_generator();
                                        
                                        //cout <<"---------------------" <<endl;
                                        //cout << step_gen.DebugString() << endl;
                                        //cout <<"---------------------" <<endl;
                                        perf_info.m_tracks[track_index].m_num_waveseq_timing_lane_steps = step_gen.timing_lane().steps_size();
                                        int timing_mode = step_gen.timing_lane().mode().value();
                                        perf_info.m_tracks[track_index].m_timing_lane_mode_use_tempo = timing_mode;
                                        
                                        perf_info.m_tracks[track_index].m_timing_lane_control_params.start_step = step_gen.timing_lane().sequence_control().start_step().value()+1;
                                        perf_info.m_tracks[track_index].m_timing_lane_control_params.end_step = step_gen.timing_lane().sequence_control().end_step().value()+1;
                                        perf_info.m_tracks[track_index].m_timing_lane_control_params.loop_start_step= step_gen.timing_lane().sequence_control().loop_start_step().value()+1;
                                        perf_info.m_tracks[track_index].m_timing_lane_control_params.loop_end_step= step_gen.timing_lane().sequence_control().loop_end_step().value()+1;
                                        perf_info.m_tracks[track_index].m_timing_lane_control_params.loop_repeat_count = step_gen.timing_lane().sequence_control().loop_repeat_count().value()+1;


                                        for(int ss=0;ss<step_gen.timing_lane().steps_size();ss++)
                                        {
                                            
                                            if (step_gen.mutable_timing_lane()->mutable_steps(ss)->has_probability_unsigned_percent())
                                            {
                                                float probability_percent = step_gen.mutable_timing_lane()->mutable_steps(ss)->probability_unsigned_percent().value();
                                                perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_probability =
                                                    probability_percent;
                                            }
                                            if(step_gen.timing_lane().steps(ss).has_step_tempo_duration())
                                            {
                                                int tempo_duration = step_gen.timing_lane().steps(ss).step_tempo_duration().value();
                                                perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_step_tempo_duration = tempo_duration;
                                            }
                                            if(step_gen.mutable_timing_lane()->mutable_steps(ss)->has_step_time_duration_seconds())
                                            {
                                                float duration_secs = step_gen.mutable_timing_lane()->mutable_steps(ss)->mutable_step_time_duration_seconds()->value();
                                                perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_step_time_duration_seconds = duration_secs;
                                            }
                                            if(step_gen.mutable_timing_lane()->mutable_steps(ss)->has_crossfade_time_duration_seconds())
                                            {
                                                perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_crossfade_secs =
                                                    step_gen.mutable_timing_lane()->mutable_steps(ss)->crossfade_time_duration_seconds().value();
                                            }
                                            perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_step_type =
                                                step_gen.mutable_timing_lane()->mutable_steps(ss)->step_type();

                                        }
                                        int num_value_lanes = step_gen.value_lanes_size();
                                        for(int v=0;v<num_value_lanes;v++)
                                        {
                                            auto value_lane = step_gen.value_lanes(v);
                                            value_lane.copied_from_uuid();
                                            //cout << value_lane.name() << endl;
                                            int lane_type = -1;
                                            if(value_lane.has_shape())
                                            {
                                                perf_info.m_tracks[track_index].m_shape_lane_control_params.start_step = value_lane.sequence_control().start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_shape_lane_control_params.end_step = value_lane.sequence_control().end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_shape_lane_control_params.loop_start_step= value_lane.sequence_control().loop_start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_shape_lane_control_params.loop_end_step= value_lane.sequence_control().loop_end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_shape_lane_control_params.loop_repeat_count = value_lane.sequence_control().loop_repeat_count().value()+1;

                                                int num_steps = value_lane.shape().steps_size();
                                                if(num_steps<1) //0 or 1?
                                                    num_steps=1;
                                                if(num_steps>64)
                                                    num_steps=64;

                                                perf_info.m_tracks[track_index].m_num_waveseq_shape_lane_steps = num_steps;
                                                //cout <<  value_lane.shape().DebugString() << endl;

                                                for(int s=0;s<num_steps;s++)
                                                {
                                                    perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_probability_unsigned_percent =
                                                        value_lane.shape().steps(s).probability_unsigned_percent().value();
                                                    int shape_type = value_lane.shape().steps(s).shape().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_shape_type = shape_type;
                                                    perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_offset = value_lane.shape().steps(s).offset().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_level = value_lane.shape().steps(s).level().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_start_phase_degrees = value_lane.shape().steps(s).start_phase_degrees().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].scale_by_gate = value_lane.shape().steps(s).scale_by_gate().value();
                                                }
                                            }

                                            
                                            

                                            if(value_lane.has_pitch())
                                            {
                                                perf_info.m_tracks[track_index].m_pitch_lane_control_params.start_step = value_lane.sequence_control().start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_pitch_lane_control_params.end_step = value_lane.sequence_control().end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_pitch_lane_control_params.loop_start_step= value_lane.sequence_control().loop_start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_pitch_lane_control_params.loop_end_step= value_lane.sequence_control().loop_end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_pitch_lane_control_params.loop_repeat_count = value_lane.sequence_control().loop_repeat_count().value()+1;

                                                int num_steps = value_lane.pitch().steps_size();
                                                if(num_steps<1) //0 or 1?
                                                    num_steps=1;
                                                if(num_steps>64)
                                                    num_steps=64;

                                                perf_info.m_tracks[track_index].m_num_waveseq_pitch_lane_steps = num_steps;
                                                //cout << value_lane.pitch().DebugString() << endl;

                                                for(int s=0;s<num_steps;s++)
                                                {
                                                    perf_info.m_tracks[track_index].m_waveseq_pitch_lane_steps[s].m_probability_unsigned_percent =
                                                        value_lane.pitch().steps(s).probability_unsigned_percent().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_pitch_lane_steps[s].m_transpose = 
                                                        value_lane.pitch().steps(s).transpose().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_pitch_lane_steps[s].m_tune_cents =
                                                        value_lane.pitch().steps(s).tune_cents().value();
                                                }

                                            }
                                            if(value_lane.has_step_seq())
                                            {
                                                perf_info.m_tracks[track_index].m_num_waveseq_stepseq_lane_steps = value_lane.step_seq().steps_size();
                                                for(int s=0;s<perf_info.m_tracks[track_index].m_num_waveseq_stepseq_lane_steps;s++)
                                                {
                                                    perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_value_signed_percent =
                                                        value_lane.step_seq().steps(s).value_signed_percent().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_value_mode =
                                                        value_lane.step_seq().steps(s).value_mode();
                                                    perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_step_transition_type =
                                                        value_lane.step_seq().steps(s).step_transition_type().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_probability =
                                                        value_lane.step_seq().steps(s).probability_unsigned_percent().value();
                                                }
                                                
                                                //cout << value_lane.step_seq().DebugString() << endl;
                                            }
                                            if(value_lane.has_hd2())
                                            {

                                                perf_info.m_tracks[track_index].m_sample_lane_control_params.start_step = value_lane.sequence_control().start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_sample_lane_control_params.end_step = value_lane.sequence_control().end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_sample_lane_control_params.loop_start_step= value_lane.sequence_control().loop_start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_sample_lane_control_params.loop_end_step= value_lane.sequence_control().loop_end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_sample_lane_control_params.loop_repeat_count = value_lane.sequence_control().loop_repeat_count().value()+1;

                                                int num_steps = value_lane.hd2().steps_size();
                                                if(num_steps<1) //0 or 1?
                                                    num_steps=1;
                                                if(num_steps>64)
                                                    num_steps=64;
                                                perf_info.m_tracks[track_index].m_num_waveseq_sample_lane_steps = num_steps;

                                                for(int s=0;s<num_steps;s++)
                                                {
                                                    perf_info.m_tracks[track_index].m_waveseq_sample_lane_steps[s].m_probability =
                                                        value_lane.hd2().steps(s).probability_unsigned_percent().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_sample_lane_steps[s].m_octave =
                                                        value_lane.hd2().steps(s).octave().value();
                                                    auto uuid = value_lane.hd2().steps(s).multisample().uuid();
                                                    perf_info.m_tracks[track_index].m_waveseq_sample_lane_steps[s].m_multiSampleUuid = uuid;
                                                }
                                                //cout << value_lane.hd2().DebugString() << endl;
                                            }

                                            if(value_lane.has_pulse_width())
                                            {
                                                perf_info.m_tracks[track_index].m_gate_lane_control_params.start_step = step_gen.timing_lane().sequence_control().start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_gate_lane_control_params.end_step = step_gen.timing_lane().sequence_control().end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_gate_lane_control_params.loop_start_step= step_gen.timing_lane().sequence_control().loop_start_step().value()+1;
                                                perf_info.m_tracks[track_index].m_gate_lane_control_params.loop_end_step= step_gen.timing_lane().sequence_control().loop_end_step().value()+1;
                                                perf_info.m_tracks[track_index].m_gate_lane_control_params.loop_repeat_count = step_gen.timing_lane().sequence_control().loop_repeat_count().value()+1;


                                                perf_info.m_tracks[track_index].m_num_waveseq_gate_lane_steps = value_lane.pulse_width().steps_size();
                                                for(int s=0;s<perf_info.m_tracks[track_index].m_num_waveseq_gate_lane_steps;s++)
                                                {
                                                    perf_info.m_tracks[track_index].m_waveseq_gate_lane_steps[s].m_value_unsigned_percent =
                                                        value_lane.pulse_width().steps(s).value_unsigned_percent().value();
                                                    perf_info.m_tracks[track_index].m_waveseq_gate_lane_steps[s].probability_unsigned_percent =
                                                        value_lane.pulse_width().steps(s).probability_unsigned_percent().value();
                                                }
                                                
                                                //cout << value_lane.pulse_width().DebugString() << endl;
                                            }
                                            if(value_lane.has_volume())
                                            {
                                                
                                                //cout << value_lane.volume().DebugString() << endl;
                                                //int num_volume_step = value_lane.volume().steps_size();
                                                //cout << std::to_string(num_volume_step) << endl;
                                            }
                                        }
                                        //cout << step_gen.DebugString() << endl;
                                        //cout << "!" << endl;

                                        //cout << "wave_sequencer" << endl;
                                    }
                                }
                            }
                        }
                    }
                }
            }
                
        }
        
        
        
        return true;
    }
    return false;
}

bool Wavest8Library::replace_program(int track_index,const std::string& program_uuid)
{
    bool result = false;

	if(m_details->m_is_perf_valid)
	{
		//only replace track_index, and only update that track in perf_info...

		assert(track_index < 4);
		if(track_index >=0 && track_index <4)
		{
			auto track = m_details->m_perf.mutable_track(track_index);
			if(track->has_instrument_track())
			{
				if(track->instrument_track().has_program())
				{

				}
				for(int i=0;i<m_details->m_program_uuids.size();i++)
				{
					if(m_details->m_program_uuids[i] == program_uuid)
					{
						auto row_id = m_details->m_program_row_ids[i];

						//import_performance_from_wavestate_from_row_id(row_id);

						//std::cout << "--- old program ------------------------------------" << std::endl;
						//auto old_prog = track->mutable_instrument_track()->mutable_program();
						//std::cout << old_prog->DebugString() << std::endl;
						//std::cout << "--- new program ------------------------------------" << std::endl;
						//auto new_prog = m_details->m_programs[i];
						//std::cout << new_prog.DebugString() << std::endl;
						
						//track->mutable_instrument_track()->set_allocated_program();
						//track->mutable_instrument_track()->program();// = 

						{
							show_popup_message("export program",32*1024*1023);
							prevent_activity(true);

							if(1)
							{
								::KorgProtocolMessage::SetSearchConditionRequest set_search_condition_request;

								set_search_condition_request.mutable_search_condition()->mutable_data_type()->Add(::KorgProtocolMessage::SoundDatabase::DATA_TYPE_PROGRAM);
								set_search_condition_request.mutable_search_condition()->mutable_exclude_additional_type()->Add("Volume Lane");

								set_search_condition_request.set_view_id(m_details->m_view_id);
								::KorgProtocolMessage::SetSearchConditionResponse set_search_condition_response;
								try {
									m_details->m_sound_library_stub->SetSearchCondition(set_search_condition_request,&set_search_condition_response,m_details->m_deadline_ms);
									//cout << set_search_condition_response.DebugString() << endl;
								}
								catch(rpcz::rpc_error& e) {
									cout << "Error: " << e.what() << endl;
									exit(0);
								}
							}

							{

								::KorgProtocolMessage::ExportToBufferRequest export_to_buffer_request;
								export_to_buffer_request.mutable_row_id()->Add(row_id);
								::KorgProtocolMessage::ExportToBufferResponse export_to_buffer_response;

								try {

									m_details->m_sound_library_stub->ExportToBuffer(export_to_buffer_request,&export_to_buffer_response,32 * 1000 * 1000);
									//cout << export_to_buffer_response.DebugString() << endl;

									if(export_to_buffer_response.buffer_size() == 1)
									{
                                        std::string buf_full = export_to_buffer_response.buffer()[0];
										const char* buf = export_to_buffer_response.buffer()[0].c_str();

#if 0
                                        FILE* f = fopen("program_exported.bin", "wb");
                                        if (f)
                                        {
                                            fwrite(buf_full.c_str(), buf_full.length(),1,f);
                                            fclose(f);
                                        }
#endif

										auto file_info2 = ::KorgProtocolMessage::FileInfo();

										buf += 4;
										int szfi = *(int*)(buf);
										buf += 4;
										file_info2.ParseFromString(std::string(buf,szfi));
										buf += szfi;
										//cout << m_details->file_info2.DebugString() << endl;
										for(int i = 0; i < file_info2.payload_info_size(); i++)
										{
											//cout << m_details->file_info2.payload_info()[i].DebugString() << endl;
											int szp = *(int*)(buf);
											buf += 4;
											std::string type = file_info2.payload_info()[i].type();
											if(type == "Program")
											{
                                                //auto pp = ::KorgProtocolMessage::Program();
                                                track->mutable_instrument_track()->mutable_program()->ParseFromString(std::string(buf,szp));
												std::cout << track->mutable_instrument_track()->mutable_program()->DebugString() << std::endl;
                                                std::cout << "Replacing track " << std::to_string(track_index) << " by program " << program_uuid << std::endl;
                                                //now replace track data in Performance.....
                                                result = true;
											}
											if(type == "ObjectInfo")
											{
												auto ob_info = ::KorgProtocolMessage::ObjectInfo();
												ob_info.ParseFromString(std::string(buf,szp));
                                                std::cout << ob_info.DebugString() << std::endl;
											}
											buf += szp;
										}
									}
								}
							    catch(rpcz::rpc_error& e) {
								    cout << "Error: " << e.what() << endl;

							    }
    						}
						    prevent_activity(false);
						    show_popup_message("done",2000);
						}
					}
				}
			}
		}
	}
    return result;
}

bool Wavest8Library::set_performance_info(const Wavest8PerformanceInfo& perf_info)
{
    if (m_details->m_is_perf_valid)
    {
        m_details->m_perf.mutable_name()->assign(perf_info.m_name);
        m_details->m_perf.mutable_tempo()->set_value(perf_info.m_tempo);

        for (int track_index = 0; track_index < m_details->m_perf.track_size(); track_index++)
        {
            assert(track_index < 4);
            auto track = m_details->m_perf.mutable_track(track_index);
            if (track->has_instrument_track())
            {
                if (track->instrument_track().has_common())
                {
                    track->mutable_instrument_track()->mutable_common()->mutable_enable_track()->set_value(perf_info.m_tracks[track_index].m_enabled_track);
                    //what is this? PCM/audio recording?
                    //track->mutable_instrument_track()->mutable_common()->mutable_enable_recording()->set_value(true);
                    //track->mutable_instrument_track()->mutable_common()->mutable_enable_input_monitor()->set_value(true);
                }
                if (track->instrument_track().has_program())
                {
                    track->mutable_instrument_track()->mutable_program()->mutable_voice_assignment_settings()->mutable_hold()->set_value(
                        perf_info.m_tracks[track_index].m_hold_note.value);
                    track->mutable_instrument_track()->mutable_program()->mutable_voice_assignment_settings()->mutable_hold()->mutable_modulation_list()->mutable_modulation()->Clear();

                    for(int m=0;m<perf_info.m_tracks[track_index].m_hold_note.m_modulations.size();m++)
                    {
                        auto org_mod = perf_info.m_tracks[track_index].m_hold_note.m_modulations[m];
                        auto mod_ptr = track->mutable_instrument_track()->mutable_program()->mutable_voice_assignment_settings()->mutable_hold()->mutable_modulation_list()->add_modulation();
                        mod_ptr->mutable_source()->set_type(org_mod.m_modulation_source);
                        mod_ptr->mutable_intensity()->set_int_value(1);//org_mod.m_intensity);
                        mod_ptr->mutable_intensity_mod_source()->set_type(0);
                    }

                    int num_patches = track->instrument_track().program().patch_size();
                    if (num_patches>0)
                    {
                        ::KorgProtocolMessage::Wavest8Model model;
                        std::string data = track->instrument_track().program().patch()[0].data();
                        if (model.ParseFromString(data))
                        {

                            // no change yet 
                            // perf_info.m_tracks[track_index].m_filter.m_filter_type = model.filter().filter_type_case();
                            if(model.has_filter())
                            {
                                
                                ::KorgProtocolMessage::ModulationList* mod_list_frequency_cutoff = 0;
                                ::KorgProtocolMessage::ModulationList* mod_list_resonance_percent = 0;
                                
                                switch(model.filter().filter_type_case())
                                {
                                case ::KorgProtocolMessage::Filter::kBypassFilter:
                                {
                                    if(model.filter().has_bypass_filter())
                                    {
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kTwoPoleLowPassFilter:
                                {
                                    if(model.filter().has_two_pole_low_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_two_pole_low_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_two_pole_low_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value = model.filter().two_pole_low_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kFourPoleLowPassFilter:
                                {
                                    if(model.filter().has_four_pole_low_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_four_pole_low_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_four_pole_low_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kTwoPoleResonantLowPassFilter:
                                {
                                    if(model.filter().has_two_pole_resonant_low_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_two_pole_resonant_low_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_two_pole_resonant_low_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().two_pole_resonant_low_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kTwoPoleResonantHighPassFilter:
                                {
                                    if(model.filter().has_two_pole_resonant_high_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_two_pole_resonant_high_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_two_pole_resonant_high_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().two_pole_resonant_high_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kTwoPoleResonantBandPassFilter:
                                {
                                    if(model.filter().has_two_pole_resonant_band_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_two_pole_resonant_band_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_two_pole_resonant_band_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().two_pole_resonant_band_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kTwoPoleResonantBandRejectFilter:
                                {
                                    if(model.filter().has_two_pole_resonant_band_reject_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_two_pole_resonant_band_reject_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_two_pole_resonant_band_reject_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().two_pole_resonant_band_reject_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kFourPoleResonantLowPassFilter:
                                {
                                    if(model.filter().has_four_pole_resonant_low_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_four_pole_resonant_low_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_four_pole_resonant_low_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().four_pole_resonant_low_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kFourPoleResonantHighPassFilter:
                                {
                                    if(model.filter().has_four_pole_resonant_high_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_four_pole_resonant_high_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent =model.mutable_filter()->mutable_four_pole_resonant_high_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().four_pole_resonant_high_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kFourPoleResonantBandPassFilter:
                                {
                                    if(model.filter().has_four_pole_resonant_band_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_four_pole_resonant_band_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_four_pole_resonant_band_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().four_pole_resonant_band_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kFourPoleResonantBandRejectFilter:
                                {
                                    if(model.filter().has_four_pole_resonant_band_reject_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_four_pole_resonant_band_reject_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_four_pole_resonant_band_reject_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().four_pole_resonant_band_reject_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kTwoPoleMultiFilter:
                                {
                                    if(model.filter().has_two_pole_multi_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_two_pole_multi_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent =  model.mutable_filter()->mutable_two_pole_multi_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().two_pole_multi_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kMs20HighPassFilter:
                                {
                                    if(model.filter().has_ms20_high_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_ms20_high_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_ms20_high_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().ms20_high_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kMs20LowPassFilter:
                                {
                                    if(model.filter().has_ms20_low_pass_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_ms20_low_pass_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_ms20_low_pass_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().ms20_low_pass_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kPolysixFilter:
                                {
                                    if(model.filter().has_polysix_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_polysix_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_polysix_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                        //perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.value =
                                        //    model.filter().polysix_filter().common().frequency_semitones().value();
                                    }
                                    break;
                                }
                                case ::KorgProtocolMessage::Filter::kOdysseyRev3Filter:
                                {
                                    if(model.filter().has_odyssey_rev_3_filter())
                                    {
                                        mod_list_frequency_cutoff = model.mutable_filter()->mutable_odyssey_rev_3_filter()->mutable_common()->mutable_frequency_semitones()->mutable_modulation_list();
                                        mod_list_resonance_percent = model.mutable_filter()->mutable_odyssey_rev_3_filter()->mutable_common()->mutable_resonance_percent()->mutable_modulation_list();
                                    }
                                    break;
                                }
                                default:
                                {
                                }
                                }
                                if(mod_list_frequency_cutoff)
                                {
                                    mod_list_frequency_cutoff->mutable_modulation()->Clear();
                                    for(int m=0;m<perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.m_modulations.size();m++)
                                    {

                                        auto org_mod = perf_info.m_tracks[track_index].m_filter.m_frequency_cutoff_semi_tones.m_modulations[m];
                                        ::KorgProtocolMessage::Modulation* mod_ptr = mod_list_frequency_cutoff->add_modulation();
                                        mod_ptr->mutable_intensity()->set_float_value(org_mod.m_scaling_factor*org_mod.m_intensity);
                                        mod_ptr->mutable_source()->set_type(org_mod.m_modulation_source);
                                        mod_ptr->mutable_intensity_mod_source()->set_type(org_mod.m_intensity_mod_source);
                                    }
                                }
                                if(mod_list_resonance_percent)
                                {
                                    mod_list_resonance_percent->mutable_modulation()->Clear();

                                }
                            }


                            model.mutable_pitch()->mutable_tune_cents()->set_value(perf_info.m_tracks[track_index].m_pitch.m_tune.value);

                            model.mutable_pitch()->mutable_tune_cents()->mutable_modulation_list()->mutable_modulation()->Clear();
                            int sz = model.mutable_pitch()->mutable_tune_cents()->mutable_modulation_list()->mutable_modulation()->size();
                            
                            if(1)
                            {
                                for(int m=0;m<perf_info.m_tracks[track_index].m_pitch.m_tune.m_modulations.size();m++)
                                {
                                    auto org_mod = perf_info.m_tracks[track_index].m_pitch.m_tune.m_modulations[m];
                                    ::KorgProtocolMessage::Modulation* mod_ptr = model.mutable_pitch()->mutable_tune_cents()->mutable_modulation_list()->add_modulation();
                                    mod_ptr->mutable_intensity()->set_float_value(org_mod.m_scaling_factor*org_mod.m_intensity);
                                    mod_ptr->mutable_source()->set_type(org_mod.m_modulation_source);
                                    mod_ptr->mutable_intensity_mod_source()->set_type(org_mod.m_intensity_mod_source);
                                }
                            }
                            model.mutable_pitch()->mutable_transpose_octaves()->mutable_modulation_list()->mutable_modulation()->Clear();
                            if(1)
                            {
                                for(int m=0;m<perf_info.m_tracks[track_index].m_pitch.m_transpose_octaves.m_modulations.size();m++)
                                {
                                    auto org_mod = perf_info.m_tracks[track_index].m_pitch.m_transpose_octaves.m_modulations[m];
                                    ::KorgProtocolMessage::Modulation* mod_ptr = model.mutable_pitch()->mutable_transpose_octaves()->mutable_modulation_list()->add_modulation();
                                    mod_ptr->mutable_intensity()->set_int_value(org_mod.m_intensity);
                                    mod_ptr->mutable_source()->set_type(org_mod.m_modulation_source);
                                    mod_ptr->mutable_intensity_mod_source()->set_type(org_mod.m_intensity_mod_source);
                                }
                            }

                            //modify values
                            model.mutable_amp_eg()->mutable_attack_time_secs()->set_value(perf_info.m_tracks[track_index].m_adrs_attack.value);

                            model.mutable_amp_eg()->mutable_attack_time_secs()->mutable_modulation_list()->mutable_modulation()->Clear();
                            for(int i=0;i<perf_info.m_tracks[track_index].m_adrs_attack.m_modulations.size();i++)
                            {
                                auto org_mod = perf_info.m_tracks[track_index].m_adrs_attack.m_modulations[i];
                                auto mod_ptr = model.mutable_amp_eg()->mutable_attack_time_secs()->mutable_modulation_list()->add_modulation();
                                mod_ptr->mutable_intensity()->set_float_value(org_mod.m_intensity);
                                mod_ptr->mutable_source()->set_type(org_mod.m_modulation_source);
                                mod_ptr->mutable_intensity_mod_source()->set_type(org_mod.m_intensity_mod_source);
                            }
                            model.mutable_amp_eg()->mutable_decay_time_secs()->set_value(perf_info.m_tracks[track_index].m_adrs_decay.value);
                            model.mutable_amp_eg()->mutable_decay_time_secs()->mutable_modulation_list()->mutable_modulation()->Clear();
                            for(int i=0;i<perf_info.m_tracks[track_index].m_adrs_decay.m_modulations.size();i++)
                            {
                                auto org_mod = perf_info.m_tracks[track_index].m_adrs_decay.m_modulations[i];
                                auto mod_ptr = model.mutable_amp_eg()->mutable_decay_time_secs()->mutable_modulation_list()->add_modulation();
                                mod_ptr->mutable_intensity()->set_float_value(org_mod.m_intensity);
                                mod_ptr->mutable_source()->set_type(org_mod.m_modulation_source);
                                mod_ptr->mutable_intensity_mod_source()->set_type(org_mod.m_intensity_mod_source);
                            }

                            model.mutable_amp_eg()->mutable_sustain_level_percent()->set_value(perf_info.m_tracks[track_index].m_adrs_sustain);
                            model.mutable_amp_eg()->mutable_release_time_secs()->set_value(perf_info.m_tracks[track_index].m_adrs_release);
                            model.mutable_common()->mutable_trigger_on_key_off()->set_value(perf_info.m_tracks[track_index].m_trigger_at_note_on);


                            if(model.osc().has_multi_sample_set())
                            {

                                for(int zone_index =0; zone_index< model.mutable_osc()->mutable_multi_sample_set()->velocity_zone_size(); zone_index++)
                                {

                                    
                                    auto zone = model.mutable_osc()->mutable_multi_sample_set()->mutable_velocity_zone(zone_index);
                                    
                                    if(perf_info.m_tracks[track_index].m_has_wave_sequence)
                                    {
                                        if(!model.mutable_osc()->mutable_multi_sample_set()->mutable_velocity_zone(zone_index)->has_wave_sequencer())
                                        {
                                            
                                            //create a value wave sequence from serialized data 'Init Wave Sequence'
                                            auto waveseq_ref = model.mutable_osc()->mutable_multi_sample_set()->mutable_velocity_zone(zone_index)->mutable_wave_sequencer();
                                                                                        
                                            //cout << "-----------------" << endl;
                                            //cout << waveseq_ref->DebugString() << endl;
                                            //cout << "++++++++++++++++++" << endl;
                                            //try loading from bin file
#ifdef LOAD_FROM_FILE
                                            const char* prefix[] ={"./", "../", "../../", "../../../", "../../../../"};
                                            int numprefix = sizeof(prefix) / sizeof(const char*);
                                            FILE* f=0;
                                            for(int i = 0; f==0 && i < numprefix; i++)
                                            {
                                                char relativeFileName[1024];
                                                sprintf(relativeFileName,"%s%s",prefix[i],"data/InitWaveSeq.bin");
                                                f = fopen(relativeFileName,"rb");
                                            }
                                            if(f)
                                            {
                                                fseek(f,0L,SEEK_END);
                                                int szf = ftell(f);
                                                if(szf)
                                                {
                                                    fseek(f,0L,SEEK_SET);
                                                    char* orgbuf = new char[szf];
                                                    const char* buf = orgbuf;
                                                    fread(orgbuf,1,szf,f);
                                                    std::string ws_data(buf,szf);

                                                    if(waveseq_ref->ParseFromString(ws_data))
                                                    {
                                                        //cout << "-----------------" << endl;
                                                        //cout << waveseq_ref->DebugString() << endl;
                                                        //cout << "-----------------" << endl;
                                                        //zone->mutable_wave_sequencer()->set_name("bla");
                                                        
                                                        //zone->clear_wave_sequencer()

                                                    }
                                                }
                                                fclose(f);
                                            }
#else
                                        auto& wsfile = bin2cpp::getInitWaveSeqFile();
                                        
                                        const char* buf = wsfile.getBuffer();
                                        std::string ws_data(buf,wsfile.getSize());

                                        if(waveseq_ref->ParseFromString(ws_data))
                                        {
                                            //std::cout << waveseq_ref->DebugString() << std::endl;
                                        }
#endif
                                        }
                                        else
                                        {
                                            auto* waveseq_ref = zone->mutable_wave_sequencer();
                                            //cout << "----------------------" << endl;
                                            //cout << waveseq_ref->DebugString() << endl;
                                            //cout << "++++++++++++++++++++++" << endl;
                                        }
                                        //zone->clear_multisample_reference();
                                        auto waveseq = zone->mutable_wave_sequencer();
                                        //cout << "!!!!!!!!!!!!!!!!!!!" << endl;
                                        //cout << model.DebugString() << endl;
                                        //cout << "?????????????" << endl;

                                        //set some data from our editor

                                        ///////////////////////////////////////////
                                        
                                        {

                                            waveseq->mutable_rhythmic_step_generator()->mutable_use_master()->set_value(perf_info.m_tracks[track_index].use_master_lane);
                                            //std::string wavesequuid = zone.wave_sequencer().copied_from_uuid();

                                            
                                            //see https://github.com/erwincoumans/wavest8sdk/blob/ff091a07354747a3263c3ce56c013fbb824143c5/SubRate/Components/RhythmicStepGenerator/RhythmicStepGenerator.proto


                                            auto step_gen = waveseq->mutable_rhythmic_step_generator();
                                            step_gen->mutable_timing_lane()->mutable_mode()->set_value(perf_info.m_tracks[track_index].m_timing_lane_mode_use_tempo);

                                            auto params = step_gen->mutable_timing_lane()->mutable_sequence_control();
                                            params->mutable_start_step()->set_value(perf_info.m_tracks[track_index].m_timing_lane_control_params.start_step-1);
                                            params->mutable_end_step()->set_value(perf_info.m_tracks[track_index].m_timing_lane_control_params.end_step-1);
                                            params->mutable_loop_start_step()->set_value(perf_info.m_tracks[track_index].m_timing_lane_control_params.loop_start_step-1);
                                            params->mutable_loop_end_step()->set_value(perf_info.m_tracks[track_index].m_timing_lane_control_params.loop_end_step-1);

                                            //cout <<"---------------------" <<endl;
                                            //cout << step_gen.DebugString() << endl;
                                            //cout <<"---------------------" <<endl;
                                            //resize timing lane?
                                            while(step_gen->mutable_timing_lane()->mutable_steps()->size() > perf_info.m_tracks[track_index].m_num_waveseq_timing_lane_steps)
                                            {
                                                step_gen->mutable_timing_lane()->mutable_steps()->RemoveLast();
                                            }
                                            while(step_gen->mutable_timing_lane()->mutable_steps()->size() < perf_info.m_tracks[track_index].m_num_waveseq_timing_lane_steps)
                                            {
                                                auto newstep = step_gen->mutable_timing_lane()->mutable_steps()->Add();
                                                *newstep = step_gen->mutable_timing_lane()->steps(0);

                                                newstep->mutable_crossfade_duration_multiplier()->mutable_modulation_list()->clear_modulation();
                                                newstep->mutable_crossfade_tempo_duration()->mutable_modulation_list()->clear_modulation();
                                                //newstep->mutable_crossfade_time_duration()->mutable_modulation_list()->clear_modulation();
                                                newstep->mutable_crossfade_time_duration_seconds()->mutable_modulation_list()->clear_modulation();
                                                newstep->mutable_fade_in_shape()->mutable_modulation_list()->clear_modulation();
                                                newstep->mutable_fade_out_shape()->mutable_modulation_list()->clear_modulation();
                                                newstep->mutable_probability_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                //newstep->mutable_pulse_width_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                //newstep->mutable_step_time_duration()->mutable_modulation_list()->clear_modulation();
                                                newstep->mutable_step_time_duration_seconds()->mutable_modulation_list()->clear_modulation();
                                            }
                                            
                                            if(step_gen->mutable_timing_lane()->mutable_steps()->size() == perf_info.m_tracks[track_index].m_num_waveseq_timing_lane_steps)
                                            {
                                                int num_steps = step_gen->timing_lane().steps_size();
                                                for(int ss=0;ss<num_steps;ss++)
                                                {

                                                    step_gen->mutable_timing_lane()->mutable_steps(ss)->mutable_probability_unsigned_percent()->set_value(
                                                        perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_probability);
                                                    
                                                    float duration_secs = perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_step_time_duration_seconds;
                                                    step_gen->mutable_timing_lane()->mutable_steps(ss)->mutable_step_time_duration_seconds()->set_value(duration_secs);
                                                    
                                                    step_gen->mutable_timing_lane()->mutable_steps(ss)->mutable_crossfade_time_duration_seconds()->set_value(
                                                        perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_crossfade_secs);

                                                    step_gen->mutable_timing_lane()->mutable_steps(ss)->set_step_type(
                                                        (KorgProtocolMessage::StepType)perf_info.m_tracks[track_index].m_waveseq_timing_lane_steps[ss].m_step_type);
                                                        
                                                }
                                            }


                                            
                                            int num_value_lanes = step_gen->value_lanes_size();
                                            for(int v=0;v<num_value_lanes;v++)
                                            {
                                                auto value_lane = step_gen->mutable_value_lanes(v);
                                                
                                                //cout << value_lane->name() << endl;
                                                int lane_type = -1;
                                                if(value_lane->has_shape())
                                                {
                                                    auto params = value_lane->mutable_sequence_control();
                                                    params->mutable_start_step()->set_value(perf_info.m_tracks[track_index].m_shape_lane_control_params.start_step-1);
                                                    params->mutable_end_step()->set_value(perf_info.m_tracks[track_index].m_shape_lane_control_params.end_step-1);
                                                    params->mutable_loop_start_step()->set_value(perf_info.m_tracks[track_index].m_shape_lane_control_params.loop_start_step-1);
                                                    params->mutable_loop_end_step()->set_value(perf_info.m_tracks[track_index].m_shape_lane_control_params.loop_end_step-1);

                                                    //cout <<  value_lane.shape().DebugString() << endl;
                                                    int num_steps = perf_info.m_tracks[track_index].m_num_waveseq_shape_lane_steps;
                                                    if(num_steps<1) //0 or 1?
                                                        num_steps=1;
                                                    if(num_steps>64)
                                                        num_steps=64;

                                                    while(value_lane->mutable_shape()->steps_size() > num_steps)
                                                    {
                                                        value_lane->mutable_shape()->mutable_steps()->RemoveLast();
                                                    }

                                                    while(value_lane->mutable_shape()->steps_size() < num_steps)
                                                    {

                                                        auto newstep = value_lane->mutable_shape()->mutable_steps()->Add();
                                                        *newstep = value_lane->mutable_shape()->steps(0);
                                                        newstep->mutable_level()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_offset()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_probability_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_scale_by_gate()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_shape()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_start_phase_degrees()->mutable_modulation_list()->clear_modulation();
                                                    }

                                                    for(int s=0;s<num_steps;s++)
                                                    {
                                                        value_lane->mutable_shape()->mutable_steps(s)->mutable_probability_unsigned_percent()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_probability_unsigned_percent);
                                                        value_lane->mutable_shape()->mutable_steps(s)->mutable_shape()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_shape_type);

                                                        value_lane->mutable_shape()->mutable_steps(s)->mutable_offset()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_offset);
                                                        value_lane->mutable_shape()->mutable_steps(s)->mutable_level()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_level);

                                                        value_lane->mutable_shape()->mutable_steps(s)->mutable_start_phase_degrees()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].m_start_phase_degrees);

                                                        value_lane->mutable_shape()->mutable_steps(s)->mutable_scale_by_gate()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_shape_lane_steps[s].scale_by_gate);


                                                    }

                                                }
                                                if(value_lane->has_pitch())
                                                {
                                                    auto params = value_lane->mutable_sequence_control();
                                                    params->mutable_start_step()->set_value(perf_info.m_tracks[track_index].m_pitch_lane_control_params.start_step-1);
                                                    params->mutable_end_step()->set_value(perf_info.m_tracks[track_index].m_pitch_lane_control_params.end_step-1);
                                                    params->mutable_loop_start_step()->set_value(perf_info.m_tracks[track_index].m_pitch_lane_control_params.loop_start_step-1);
                                                    params->mutable_loop_end_step()->set_value(perf_info.m_tracks[track_index].m_pitch_lane_control_params.loop_end_step-1);

                                                    //perf_info.m_tracks[track_index].m_num_waveseq_pitch_lane_steps = value_lane.pitch().steps_size();
                                                    //cout << value_lane.pitch().DebugString() << endl;
                                                    int num_steps = perf_info.m_tracks[track_index].m_num_waveseq_pitch_lane_steps;
                                                    if(num_steps<1)
                                                        num_steps=1;
                                                    if(num_steps>64)
                                                        num_steps=64;

                                                    while(value_lane->mutable_pitch()->steps_size() >num_steps)
                                                    {
                                                        value_lane->mutable_pitch()->mutable_steps()->RemoveLast();
                                                    }
                                                    while(value_lane->mutable_pitch()->steps_size() <num_steps)
                                                    {
                                                        auto newstep = value_lane->mutable_pitch()->mutable_steps()->Add();
                                                        *newstep = value_lane->mutable_pitch()->steps(0);
                                                        newstep->mutable_probability_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_step_transition_type()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_transpose()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_tune_cents()->mutable_modulation_list()->clear_modulation();
                                                    }

                                                    for(int s=0;s<num_steps;s++)
                                                    {
                                                        value_lane->mutable_pitch()->mutable_steps(s)->mutable_probability_unsigned_percent()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_pitch_lane_steps[s].m_probability_unsigned_percent);
                                                        value_lane->mutable_pitch()->mutable_steps(s)->mutable_transpose()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_pitch_lane_steps[s].m_transpose);
                                                        value_lane->mutable_pitch()->mutable_steps(s)->mutable_tune_cents()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_pitch_lane_steps[s].m_tune_cents);
                                                    }

                                                }
                                                if(value_lane->has_step_seq())
                                                {
                                                    //perf_info.m_tracks[track_index].m_num_waveseq_stepseq_lane_steps = value_lane.step_seq().steps_size();
                                                    //cout << value_lane.step_seq().DebugString() << endl;
                                                    int num_steps = perf_info.m_tracks[track_index].m_num_waveseq_stepseq_lane_steps;

                                                    while(value_lane->mutable_step_seq()->steps_size() > num_steps)
                                                    {
                                                        value_lane->mutable_step_seq()->mutable_steps()->RemoveLast();
                                                    }

                                                    while(value_lane->mutable_step_seq()->steps_size() < num_steps)
                                                    {
                                                        auto newstep = value_lane->mutable_step_seq()->mutable_steps()->Add();
                                                        *newstep = value_lane->mutable_step_seq()->steps(0);
                                                        newstep->mutable_probability_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_step_transition_type()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_value_signed_percent()->mutable_modulation_list()->clear_modulation();
                                                    }

                                                    for(int s=0;s<perf_info.m_tracks[track_index].m_num_waveseq_stepseq_lane_steps;s++)
                                                    {
                                                        value_lane->mutable_step_seq()->mutable_steps(s)->mutable_probability_unsigned_percent()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_probability);
                                                        value_lane->mutable_step_seq()->mutable_steps(s)->mutable_step_transition_type()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_step_transition_type);
                                                        value_lane->mutable_step_seq()->mutable_steps(s)->mutable_value_signed_percent()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_value_signed_percent);
                                                        value_lane->mutable_step_seq()->mutable_steps(s)->set_value_mode(
                                                            (KorgProtocolMessage::StepSeqStep_ValueMode)
                                                            perf_info.m_tracks[track_index].m_waveseq_stepseq_lane_steps[s].m_value_mode);
                                                    }

                                                }
                                                if(value_lane->has_hd2())
                                                {
                                                    auto params = value_lane->mutable_sequence_control();
                                                    params->mutable_start_step()->set_value(perf_info.m_tracks[track_index].m_sample_lane_control_params.start_step-1);
                                                    params->mutable_end_step()->set_value(perf_info.m_tracks[track_index].m_sample_lane_control_params.end_step-1);
                                                    params->mutable_loop_start_step()->set_value(perf_info.m_tracks[track_index].m_sample_lane_control_params.loop_start_step-1);
                                                    params->mutable_loop_end_step()->set_value(perf_info.m_tracks[track_index].m_sample_lane_control_params.loop_end_step-1);

                                                    int num_steps = perf_info.m_tracks[track_index].m_num_waveseq_sample_lane_steps;
                                                    if(num_steps<1)
                                                        num_steps=1;
                                                    if(num_steps>64)
                                                        num_steps=64;

                                                    while (value_lane->mutable_hd2()->steps_size()>num_steps)
                                                    {
                                                        value_lane->mutable_hd2()->mutable_steps()->RemoveLast();
                                                    }
                                                    while(value_lane->mutable_hd2()->steps_size()<num_steps)
                                                    {
                                                        auto step = value_lane->mutable_hd2()->mutable_steps()->Add();
                                                        //copy some 'sane' values in there and clear existing modulations
                                                        *step = value_lane->mutable_hd2()->steps(0);
                                                        step->mutable_channel_select()->mutable_modulation_list()->clear_modulation();
                                                        step->mutable_octave()->mutable_modulation_list()->clear_modulation();
                                                        step->mutable_probability_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                        step->mutable_reverse()->mutable_modulation_list()->clear_modulation();
                                                        step->mutable_start_offset()->mutable_modulation_list()->clear_modulation();
                                                        step->mutable_transpose_semitones()->mutable_modulation_list()->clear_modulation();
                                                        step->mutable_trim_db()->mutable_modulation_list()->clear_modulation();
                                                        step->mutable_tune_cents()->mutable_modulation_list()->clear_modulation();
                                                    }

                                                    if(perf_info.m_tracks[track_index].m_num_waveseq_sample_lane_steps == num_steps)
                                                    {

                                                        for(int s=0;s<num_steps;s++)
                                                        {
                                                            value_lane->mutable_hd2()->mutable_steps(s)->mutable_probability_unsigned_percent()->set_value(
                                                                perf_info.m_tracks[track_index].m_waveseq_sample_lane_steps[s].m_probability);
                                                            value_lane->mutable_hd2()->mutable_steps(s)->mutable_octave()->set_value(
                                                                perf_info.m_tracks[track_index].m_waveseq_sample_lane_steps[s].m_octave);
                                                            auto uuid = perf_info.m_tracks[track_index].m_waveseq_sample_lane_steps[s].m_multiSampleUuid;
                                                            value_lane->mutable_hd2()->mutable_steps(s)->mutable_multisample()->set_uuid(
                                                                uuid);
                                                        }
                                                        //cout << value_lane.hd2().DebugString() << endl;
                                                    }
                                                }

                                                if(value_lane->has_pulse_width())
                                                {

                                                    int num_steps = perf_info.m_tracks[track_index].m_num_waveseq_gate_lane_steps;
                                                    if(num_steps<1)
                                                        num_steps=1;
                                                    if(num_steps>64)
                                                        num_steps=64;

                                                    while(value_lane->mutable_pulse_width()->steps_size()>num_steps)
                                                    {
                                                        value_lane->mutable_pulse_width()->mutable_steps()->RemoveLast();
                                                    }

                                                    while(value_lane->mutable_pulse_width()->steps_size()<num_steps)
                                                    {
                                                        auto newstep = value_lane->mutable_pulse_width()->mutable_steps()->Add();
                                                        newstep->mutable_probability_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                        newstep->mutable_value_unsigned_percent()->mutable_modulation_list()->clear_modulation();
                                                    }

                                                    for(int s=0;s<perf_info.m_tracks[track_index].m_num_waveseq_gate_lane_steps;s++)
                                                    {
                                                        value_lane->mutable_pulse_width()->mutable_steps(s)->mutable_probability_unsigned_percent()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_gate_lane_steps[s].probability_unsigned_percent);

                                                        value_lane->mutable_pulse_width()->mutable_steps(s)->mutable_value_unsigned_percent()->set_value(
                                                            perf_info.m_tracks[track_index].m_waveseq_gate_lane_steps[s].m_value_unsigned_percent);
                                                    }

                                                    //cout << value_lane.pulse_width().DebugString() << endl;
                                                }
                                                if(value_lane->has_volume())
                                                {

                                                    //cout << value_lane.volume().DebugString() << endl;
                                                    //int num_volume_step = value_lane.volume().steps_size();
                                                    //cout << std::to_string(num_volume_step) << endl;
                                                }
                                            }
                                            //cout << step_gen.DebugString() << endl;
                                            //cout << "!" << endl;

                                            //cout << "wave_sequencer" << endl;
                                        }




                                    }
                                    else
                                    {
                                        //////////////////////////////////////////////////////////////////////////////////////
                                        //clear the wave sequence, create a single multi sample, set its uuid
                                        zone->clear_wave_sequencer();
                                        auto ref = zone->mutable_multisample_reference();
                                        std::string uuid = perf_info.m_tracks[track_index].m_multi_sample_instrument_uuid;
                                        ref->set_uuid(uuid);


                                        if(zone->has_velocity_zone())
                                        {
                                        }
                                        else
                                        {
                                            //set some defaults, otherwise no sound
                                            //todo: allow modulations?
                                            ref->set_channel_select(::KorgProtocolMessage::SampleReference_ChannelSelect_CHANNEL_SELECT_ORIGINAL);
                                            auto perc = zone->level_percent();
                                            zone->set_level_percent(100);
                                            zone->mutable_velocity_zone()->mutable_range()->mutable_min()->set_value(1);
                                            zone->mutable_velocity_zone()->mutable_range()->mutable_min()->mutable_modulation_list()->clear_modulation();
                                            zone->mutable_velocity_zone()->mutable_range()->mutable_max()->set_value(127);
                                            zone->mutable_velocity_zone()->mutable_range()->mutable_max()->mutable_modulation_list()->clear_modulation();
                                            zone->mutable_velocity_zone()->mutable_min_slope()->mutable_modulation_list()->clear_modulation();
                                            zone->mutable_velocity_zone()->mutable_max_slope()->mutable_modulation_list()->clear_modulation();
                                            zone->mutable_velocity_zone()->mutable_max_curve_type()->mutable_modulation_list()->clear_modulation();
                                            zone->mutable_velocity_zone()->mutable_min_slope_start()->mutable_modulation_list()->clear_modulation();
                                            zone->mutable_velocity_zone()->mutable_max_slope_start()->mutable_modulation_list()->clear_modulation();
                                        }

                                        //cout << zone->DebugString() << endl;
                                    }
                                }
                            }


                            //write back to 'data'
                            std::string updated_data;
                            model.SerializeToString(&updated_data);
                            track->mutable_instrument_track()->mutable_program()->mutable_patch()->mutable_data()[0]->mutable_data()->assign(updated_data);

#if 0
                            std::string data = track->instrument_track().program().patch()[0].data();
                            if(model.ParseFromString(data))
                            {
                                if(!model.mutable_osc()->mutable_multi_sample_set()->mutable_velocity_zone(0)->has_wave_sequencer())
                                {
                                    cout << "still not" << endl;
                                }
                            }
#endif

                        }
                    }
                    else
                    {
                        cout << "no path, does this happen?" << endl;
                    }
                }
                else
                {
                    cout << "no program, does this happen?" << endl;
                }
            }
        }
        
        return true;
    }
    return false;
}

struct MemoryBuffer
{
    std::string buf;
    void write(const void* buffer, int num_bytes)
    {
        buf.append(string((const char*)buffer, num_bytes));
    }
};

bool Wavest8Library::export_performance_to_buffer(struct MemoryBuffer& mem_buf, bool add_initial_size, bool clear_write_protected)
{

    bool ok = false;

    if (m_details->m_is_perf_valid)
    {
        if (m_details->file_info3.payload_info_size() == 2)
        {
            ok = true;
            std::string str_perf;
            if (m_details->m_perf.SerializeToString(&str_perf))
            {
                std::string str_obj_info;
                auto ob_info = m_details->m_ob_info;
                if(clear_write_protected)
                {
                    ob_info.set_write_protected(false);
                }
                if (ob_info.SerializeToString(&str_obj_info))
                {
                    std::string str_file_info3;
                    if (m_details->file_info3.SerializeToString(&str_file_info3))
                    {
                        int total_bytes = 4 + 4 + str_file_info3.length() + 4 + str_perf.length() + 4 + str_obj_info.length();
                        if (add_initial_size)
                        {
                            mem_buf.write(&total_bytes, 4);
                        }
                        mem_buf.write("Korg", 4);
                        int sz = str_file_info3.length();
                        mem_buf.write(&sz, 4);
                        mem_buf.write(str_file_info3.c_str(), sz);
                        for (int j = 0; j < m_details->file_info3.payload_info_size(); j++)
                        {
                            auto payload_item = m_details->file_info3.payload_info()[j];
                            //cout << "type:" << payload_item.type() << endl;
                            if (payload_item.type() == "Performance")
                            {

                                int num_bytes = str_perf.length();
                                mem_buf.write(&num_bytes, 4);
                                mem_buf.write(str_perf.c_str(), num_bytes);
                            }
                            if (payload_item.type() == "ObjectInfo")
                            {

                                int num_bytes = str_obj_info.length();
                                mem_buf.write(&num_bytes, 4);
                                mem_buf.write(str_obj_info.c_str(), num_bytes);
                            }
                        }
                    }
                    else
                    {
                        ok = false;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                ok = false;
            }
        }
    }
    return ok;
}

bool Wavest8Library::export_file(const std::string& file_name)
{
    bool ok = true;
    MemoryBuffer mem_buf;

    if (m_details->m_is_perf_valid)
    {
        
        {
            mem_buf.write("Korg", 4);
            //fwrite("Korg", 4, 1, f);
 
            {
                std::string file_info_str;
                if (m_details->file_info.SerializeToString(&file_info_str))
                {
                    int sz = file_info_str.length();
                    mem_buf.write(&sz, 4);
                    mem_buf.write(file_info_str.c_str(), sz);
                }
                else
                {
                    ok = false;
                }
            }

            {
                std::string ext_file_info_str;
                if (m_details->ext_file_info.SerializeToString(&ext_file_info_str))
                {
                    int sz = ext_file_info_str.size();
                    mem_buf.write(&sz, 4);
                    mem_buf.write(ext_file_info_str.c_str(), sz);
                }
                else
                {
                    ok = false;
                } 
            }
            {
                if (ok)
                {
                    export_performance_to_buffer(mem_buf, true,false);
                }
            }
            if (ok)
            {
                FILE* f = fopen(file_name.c_str(), "wb");
                if (f)
                {
                    fwrite(mem_buf.buf.c_str(), mem_buf.buf.length(),1,f);
                    fclose(f);
                }
                else
                {
                    ok = false;
                }
                
            }
            
            return ok;
        }
    }
    return false;
}

bool Wavest8Library::export_performance_to_wavestate(bool overwrite, bool select_on_wavestate, std::int64_t& new_row_id)
{
    new_row_id = -1;
    bool result = false;
    MemoryBuffer mem_buf;
    
    bool make_unique = !overwrite;
    bool clear_write_protected = make_unique;
    if(select_on_wavestate)
    {
        select_performance(m_details->m_performance_row_ids[0]);
#ifdef WIN32
        Sleep(100);
#else
	::usleep(1000*100);
#endif
        select_performance(m_details->m_performance_row_ids[1]);
#ifdef WIN32
        Sleep(100);
#else
	::usleep(1000*100);
#endif

    }

    if (export_performance_to_buffer(mem_buf,false, clear_write_protected))
    {
        prevent_activity(true);

        ::KorgProtocolMessage::ImportFromBufferRequest import_from_buffer_request;
        import_from_buffer_request.set_dry_run(false);
        
        import_from_buffer_request.set_make_unique(make_unique);

        {
            import_from_buffer_request.mutable_buffer()->Add();
            import_from_buffer_request.set_buffer(0, mem_buf.buf.c_str(), mem_buf.buf.size());
            ::KorgProtocolMessage::ImportFromBufferResponse import_from_buffer_response;
            try {
                m_details->m_sound_library_stub->ImportFromBuffer(import_from_buffer_request, &import_from_buffer_response, 32 * 1000 * 1000);
                //cout << import_from_buffer_response.DebugString() << endl;
                if(select_on_wavestate)
                {
                    //we need to force a switch to another performance, otherwise changes in the database have no effect
                    
                    for(int i=0;i<import_from_buffer_response.imported_record_size();i++)
                    {
                        auto record = import_from_buffer_response.imported_record(i);
                        new_row_id = record.row_id();
                        select_performance(new_row_id);
                    }
                }

                int num_records_imported = import_from_buffer_response.num_records_imported();
                int num_records_updated = import_from_buffer_response.num_records_updated();
                result = true;
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                
            }
        }
        prevent_activity(false);
    }
    
    return result;
}

std::string toupper(const std::string &s) {
    std::string up = s;
    for(char &c : up)
        c = std::tolower(c);
    return up;
    //return s;
}

struct less_than_key
{
    inline bool operator() (const ::KorgProtocolMessage::SoundDatabase::Record& struct1,const ::KorgProtocolMessage::SoundDatabase::Record& struct2)
    {
        return ((toupper(struct1.info().name()) < toupper(struct2.info().name())) ||
            ((toupper(struct1.info().name()) == toupper(struct2.info().name())) && (struct1.id() < struct2.id()))
            );
        
    }
};

bool Wavest8Library::create_view()
{
    
    prevent_activity(true);


    if (1)
    {
        {
#if 0
            0000   fe 33 da 63 79 49 00 33 09 05 c1 01 86 dd 60 04   .3.cyI.3......`.
                0010   f3 ee 00 49 06 80 fe 80 00 00 00 00 00 00 91 cf   ...I............
                0020   45 a3 bd 6e 0d e4 fe 80 00 00 00 00 00 00 fc 33   E..n...........3
                0030   da ff fe 63 79 49 c6 dc c3 50 be 15 d4 f4 0f d0   ...cyI...P......
                0040   21 e2 50 18 10 2a f6 6a 00 00 01 00 01 08 bd a4   !.P..*.j........
                0050   96 01 00 60 c2 5f 01 21 1a 13 53 6f 75 6e 64 4c   ...`._.!..SoundL
                0060   69 62 72 61 72 79 53 65 72 76 69 63 65 22 0a 43   ibraryService".C
                0070   72 65 61 74 65 56 69 65 77 00 04 12 00 1a 00      reateView......



                0000   fe 33 da 63 79 49 00 33 09 05 c1 01 08 00 45 00   .3.cyI.3......E.
                0010   00 5d 69 85 40 00 80 06 27 d3 a9 fe 0d e4 a9 fe   .]i.@...'.......
                0020   07 62 c7 c9 c3 50 11 cc 66 15 d7 1f 11 3c 50 18   .b...P..f....<P.
                0030   10 08 5b 98 00 00 01 00 01 08 7e ee 1d 00 00 1c   ..[.......~.....
                0040   fe 47 01 21 1a 13 53 6f 75 6e 64 4c 69 62 72 61   .G.!..SoundLibra
                0050   72 79 53 65 72 76 69 63 65 22 0a 43 72 65 61 74   ryService".Creat
                0060   65 56 69 65 77 00 04 12 00 1a 00                  eView......



#endif

            char buf[] ={ 0x12 ,0x00 ,0x1a ,0x00};

            ::KorgProtocolMessage::CreateViewRequest create_view_request;
            int sz = sizeof(buf);
            std::string data(buf,sz);
            if(create_view_request.ParseFromString(data))
            {
                //cout << create_view_request.DebugString() << endl;
            }
            else
            {
                cout << "fail" << endl;
            }

            //create_view_request.set_sound_list_object_id(0);
            ::KorgProtocolMessage::CreateViewResponse create_view_response;
            try {
                m_details->m_sound_library_stub->CreateView(create_view_request, &create_view_response, m_details->m_deadline_ms);
                //cout << create_view_response.DebugString() << endl;
                m_details->m_view_id = create_view_response.view_id();
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }

      


       
    }


    updatePerformanceList();
   

    prevent_activity(false);
    return true;
}



void Wavest8Library::updatePerformanceList()
{

    ::google::protobuf::uint64 index_min = -1;
    ::google::protobuf::uint64 index_max = -1;

    ::google::protobuf::int64 value_descriptor_id = -1;
    ::KorgProtocolMessage::Value value;
    ::google::protobuf::uint64 object_id = -1;
    ::KorgProtocolMessage::GetRowIDOfIndexResponse row_id_response;

    if(1)
    {
        {
            ::KorgProtocolMessage::GetDigestRequest digest_request;
            digest_request.mutable_object_path()->set_path("|Current Performance|Select");// | wavestate | Set List Select");

            ::KorgProtocolMessage::GetDigestResponse digest_response;
            try {
                m_details->m_object_stub->GetDigest(digest_request,&digest_response,m_details->m_deadline_ms);
                //cout << digest_response.DebugString() << endl;
                value_descriptor_id = digest_response.value_descriptor_id();
                value = digest_response.value();

                object_id = digest_response.object_id();
}
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }

        {
            ::KorgProtocolMessage::GetParameterDescriptorDigestRequest param_request;
            param_request.set_value_descriptor_id(value_descriptor_id);
            ::KorgProtocolMessage::GetParameterDescriptorDigestResponse param_response;
            try {
                m_details->m_parameter_descriptor_stub->GetDigest(param_request,&param_response,m_details->m_deadline_ms);
                //cout << param_response.DebugString() << endl;
                index_min = param_response.mutable_value_descriptor()->mutable_min_value()->int_64_value();
                index_max = param_response.mutable_value_descriptor()->mutable_max_value()->int_64_value();

            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }

        {
            ::KorgProtocolMessage::ConvertValueToTextRequest convert_request;
            convert_request.set_value_descriptor_id(value_descriptor_id);
            convert_request.mutable_value()->CopyFrom(value);
            ::KorgProtocolMessage::ConvertValueToTextResponse convert_response;
            try {
                m_details->m_parameter_descriptor_stub->ConvertValueToText(convert_request,&convert_response,m_details->m_deadline_ms);
                //cout << convert_response.DebugString() << endl;
                cout << "Current Selected Performance:" << convert_response.text() << endl;
            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }
    }




    if(1)
    {
        {
            ::KorgProtocolMessage::CountResultsRequest count_request;
            count_request.set_view_id(m_details->m_view_id);
            ::KorgProtocolMessage::CountResultsResponse count_response;
            try {
                m_details->m_sound_library_stub->CountResults(count_request,&count_response,m_details->m_deadline_ms);
                //cout << count_response.DebugString() << endl;
                m_details->m_row_count = count_response.count();
            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }



        {
            ::KorgProtocolMessage::GetRowIDOfIndexRequest row_id_request;
            unsigned char buf[] ={0x08 ,0x80 ,0xf2 ,0x85 ,0xd0 ,0xda ,0x0a};

            int sz = sizeof(buf);
            std::string data((char*)buf,sz);
            if(row_id_request.ParseFromString(data))
            {
                //cout << row_id_request.DebugString() <<endl;
                //cout << "ok" <<endl;
            }
            else
            {
                cout << "fail" <<endl;
            }


            row_id_request.set_view_id(m_details->m_view_id);
            for(::google::protobuf::int32 index = 0; index < m_details->m_row_count; index++)
            {
                row_id_request.mutable_index()->Add(index);
            }
            try {
                m_details->m_sound_library_stub->GetRowIDOfIndex(row_id_request,&row_id_response,m_details->m_deadline_ms);
                //cout << row_id_response.DebugString() << endl;
                m_details->m_actual_row_count = row_id_response.row_id_size();
            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }
#if 0
        if(0)
        {
            unsigned char buf[] ={0x08 ,0x80 ,0xf2 ,0x85 ,0xd0 ,0xda ,0x0a ,0x12 ,0x17 ,0x0a ,0x08 ,0x0d ,0x0e
                ,0x0f ,0x05 ,0x04 ,0x09 ,0x08 ,0x10 ,0x72 ,0x0b ,0x56 ,0x6f ,0x6c ,0x75 ,0x6d ,0x65 ,0x20 ,0x4c
                ,0x61 ,0x6e ,0x65};
            int sz = sizeof(buf);

            ::KorgProtocolMessage::SetSearchConditionRequest set_search_condition_request;
            std::string data((char*)buf,sz);
            if(set_search_condition_request.ParseFromString(data))
            {
                //cout << set_search_condition_request.DebugString() <<endl;
            }
            else
            {
                cout << "error" << endl;
            }


            set_search_condition_request.mutable_search_condition()->mutable_data_type()->Add(::KorgProtocolMessage::SoundDatabase::DATA_TYPE_PERFORMANCE);
            //set_search_condition_request.mutable_search_condition()->mutable_exclude_additional_type()->Add("Volume Lane");

            set_search_condition_request.set_view_id(m_details->m_view_id);
            ::KorgProtocolMessage::SetSearchConditionResponse set_search_condition_response;
            try {
                m_details->m_sound_library_stub->SetSearchCondition(set_search_condition_request,&set_search_condition_response,m_details->m_deadline_ms);
                //cout << set_search_condition_response.DebugString() << endl;

            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }
#endif

        {

            ::KorgProtocolMessage::GetIndexOfRowIDRequest request;
            unsigned char buf[] ={0x08 ,0x80 ,0xf2 ,0x85 ,0xd0 ,0xda ,0x0a};

            int sz = sizeof(buf);
            std::string data((char*)buf,sz);
            if(request.ParseFromString(data))
            {
                //cout << request.DebugString() << endl;
            }
            else
            {
                cout << "fail" << endl;
            }

            ::KorgProtocolMessage::GetIndexOfRowIDResponse response;
            try
            {
                m_details->m_sound_library_stub->GetIndexOfRowID(request,&response,m_details->m_deadline_ms);
                //cout << response.DebugString() << endl;
            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }

        }


        {

            {
                ::KorgProtocolMessage::CountResultsRequest count_request;
                count_request.set_view_id(m_details->m_view_id);
                ::KorgProtocolMessage::CountResultsResponse count_response;
                try {
                    m_details->m_sound_library_stub->CountResults(count_request,&count_response,m_details->m_deadline_ms);
                    //cout << count_response.DebugString() << endl;
                    m_details->m_row_count = count_response.count();
                }
                catch(rpcz::rpc_error& e) {
                    cout << "Error: " << e.what() << endl;
                    exit(0);
                }
            }

        }

        int records=0;

        m_details->m_performance_row_ids.clear();
        m_details->m_performance_names.clear();
        
        m_details->m_programs.clear();
        m_details->m_program_names.clear();
        m_details->m_program_uuids.clear();

        if(1)
        {

            bool single_record=false;
            if(single_record)
            {
                std::vector<::KorgProtocolMessage::SoundDatabase::Record> valid_performance_records;
                std::vector<::KorgProtocolMessage::SoundDatabase::Record> valid_program_records;

                for(auto m=index_min; m<index_max+1;m++)
                {
                    ::KorgProtocolMessage::ReadRecordRequest request;
                    //cout << to_string(row_id_response.row_id()[m]) << endl;
                    request.set_row_id(m);//row_id_response.row_id()[m]);
                    //request.set_row_id(row_id_response.row_id()[m]);
                    ::KorgProtocolMessage::ReadRecordResponse response;
                    try {
                        m_details->m_sound_library_stub->ReadRecord(request,&response,m_details->m_deadline_ms);

                        if(response.has_record())
                        {
                            //cout << response.DebugString() <<endl;
                            //if(row_id_response.row_id()[m]==response.record().id())
                            if(m==response.record().id())

                            {
                                records++;
                                if(response.record().info().name().length())
                                {
                                    if(response.record().data_type() == ::KorgProtocolMessage::SoundDatabase::DATA_TYPE_PERFORMANCE)
                                    {
                                        valid_performance_records.push_back(response.record());
                                    }
                                    if(response.record().data_type() == ::KorgProtocolMessage::SoundDatabase::DATA_TYPE_PROGRAM)
                                    {
                                        valid_program_records.push_back(response.record());
                                    }
                                    

                                }

                            }

                        }

                    }
                    catch(rpcz::rpc_error& e) {
                        std::cout << "Error: " << e.what() << std::endl;
                        exit(0);
                    }
                }

                std::sort(valid_performance_records.begin(),valid_performance_records.end(),less_than_key());

                std::sort(valid_program_records.begin(),valid_program_records.end(),less_than_key());

                


                int count = 1;
                for(int i = 0; i < valid_performance_records.size(); i++)
                {

                    if(valid_performance_records[i].has_info())
                    {
                        auto record = valid_performance_records[i];
                        auto info = record.info();
                        //cout << info.DebugString() << endl;
                        //cout << "[" << std::to_string(i) << "]" << info.name() << endl;
                        //cout << "[" << std::to_string(i) << "," << std::to_string(record.id()) << "]" << info.name() << endl;

                        m_details->m_performance_row_ids.push_back(record.id());
                        m_details->m_performance_names.push_back(std::to_string(count++)+":"+info.name());
                    }
                }


                count = 1;

                for(int i = 0; i < valid_program_records.size(); i++)
                {

                    if(valid_program_records[i].has_info())
                    {
                        auto record = valid_program_records[i];
                        auto info = record.info();
                        //cout << info.DebugString() << endl;
                        //cout << "[" << std::to_string(i) << "]" << info.name() << endl;
                        //cout << "[" << std::to_string(i) << "," << std::to_string(record.id()) << "]" << info.name() << endl;
                        m_details->m_program_row_ids.push_back(record.id());
                        m_details->m_program_uuids.push_back(info.uuid());
                        m_details->m_program_names.push_back(std::to_string(count++)+":"+info.name());
                    }
                }


            }
            else
            {
                ::KorgProtocolMessage::ReadMultipleRecordsRequest read_multiple_records_request;


                read_multiple_records_request.add_row_id(index_min);
                read_multiple_records_request.add_row_id(index_max+1);


                ::KorgProtocolMessage::ReadMultipleRecordsResponse read_multiple_records_response;

                try {
                    m_details->m_sound_library_stub->ReadMultipleRecords(read_multiple_records_request,&read_multiple_records_response,32*1024*1024);//m_details->m_deadline_ms);
                    //cout << read_multiple_records_response.DebugString() << endl;
                    int num_records = read_multiple_records_response.record_size();

                    std::sort(read_multiple_records_response.mutable_record()->begin(),read_multiple_records_response.mutable_record()->end(),less_than_key());
                    int count=1;
                    m_details->m_wave_sequences.clear();
                    for(int i = 0; i < num_records; i++)
                    {
                        //cout << read_multiple_records_response.record()[i].DebugString() <<endl;

                        if(read_multiple_records_response.record()[i].has_info())
                        {
                            if(read_multiple_records_response.record()[i].data_type() == ::KorgProtocolMessage::SoundDatabase::DATA_TYPE_WAVE_SEQUENCER)
                            {
                                int waveseq_index = m_details->m_wave_sequence_names.size();
                                auto record = read_multiple_records_response.record()[i];
                                m_details->m_wave_sequences.push_back(record);
                                
                                //std::string new_name = std::to_string(waveseq_index)+std::string(": ")+record.info().name();
                                m_details->m_wave_sequence_names.push_back(record.info().name());
                                m_details->m_wave_sequence_uuids.push_back(record.info().uuid());
                            }
                            if(read_multiple_records_response.record()[i].data_type() == ::KorgProtocolMessage::SoundDatabase::DATA_TYPE_PROGRAM)
                            {
                                auto record = read_multiple_records_response.record()[i];
                                auto info = record.info();
                                int prog_index = m_details->m_programs.size();
                                m_details->m_programs.push_back(record);
                                ///std::string new_name = std::to_string(prog_index)+std::string(": ")+record.info().name();
                                m_details->m_program_names.push_back(record.info().name());
                                m_details->m_program_uuids.push_back(record.info().uuid());
                                m_details->m_program_row_ids.push_back(record.id());
                            }
                            
                            if(read_multiple_records_response.record()[i].data_type() == ::KorgProtocolMessage::SoundDatabase::DATA_TYPE_PERFORMANCE)
                            {
                                auto record = read_multiple_records_response.record()[i];
                                auto info = record.info();
                                //cout << info.DebugString() << endl;
                                //cout << "[" << std::to_string(i) << "]" << info.name() << endl;
                                //cout << "[" << std::to_string(i) << "," << std::to_string(record.id()) << "]" << info.name() << endl;

                                m_details->m_performance_row_ids.push_back(record.id());
                                //m_details->m_performance_names.push_back(info.name());
                                m_details->m_performance_names.push_back(std::to_string(count++)+":"+info.name());
                            }
                        }
                    }
                }
                catch(rpcz::rpc_error& e) {
                    cout << "Error: " << e.what() << endl;
                    exit(0);
                }
            }
        }

        //for (int i = 0; i < row_id_response.row_id_size(); i++)
        //{
        //    ::google::protobuf::int64 row_id = row_id_response.row_id(i);
        //    cout << std::to_string(i) << ":" << std::to_string(row_id) << endl;
        //}
        if(0)
        {
            ::KorgProtocolMessage::ExportToBufferRequest export_to_buffer_request;

            ::google::protobuf::int64 row_id = row_id_response.row_id(0);
            //::google::protobuf::int64 row_id2 = row_id_response.row_id(2);
            export_to_buffer_request.mutable_row_id()->Add(row_id);
            ::KorgProtocolMessage::ExportToBufferResponse  export_to_buffer_response;
            try {
                m_details->m_sound_library_stub->ExportToBuffer(export_to_buffer_request,&export_to_buffer_response,32 * 1024 * 1023);// m_details->m_deadline_ms);
                //cout << export_to_buffer_response.DebugString() << endl;
                //todo: wrap up binary in a FileInfo, with details about the binary data
                //see also extract_wavestate.py for more info
                FILE* f = fopen("testbuf.bin","wb");
                if(f)
                {
                    int size_buf = export_to_buffer_response.buffer_size();
                    if(size_buf)
                    {

                        //todo: turn this read/modify/write data into an GUI based editor

                        ::KorgProtocolMessage::FileInfo* file_info = new ::KorgProtocolMessage::FileInfo();

                        const char* buf = export_to_buffer_response.buffer()[0].c_str();
                        buf += 4;
                        int szfi = *(int*)(buf);
                        buf += 4;
                        file_info->ParseFromString(std::string(buf,szfi));
                        buf += szfi;
                        cout << file_info << endl;
                        for(int i = 0; i < file_info->payload_info_size(); i++)
                        {
                            //cout << file_info->payload_info()[i].DebugString() << endl;
                            int szp = *(int*)(buf);
                            buf += 4;
                            std::string type = file_info->payload_info()[i].type();
                            if(type == "Performance")
                            {
                                ::KorgProtocolMessage::Performance perf;
                                perf.ParseFromString(std::string(buf,szp));
                                perf.set_name("cute");
                                cout << perf.DebugString() << endl;
                            }
                            if(type == "ObjectInfo")
                            {
                                ::KorgProtocolMessage::ObjectInfo obinfo;
                                obinfo.ParseFromString(std::string(buf,szp));
                                //obinfo.set_write_protected(false);
                                //cout << obinfo.DebugString() << endl;
                            }
                            buf += szp;
                        }

                        int sz = export_to_buffer_response.buffer()[0].size();
                        fwrite(export_to_buffer_response.buffer()[0].c_str(),1,sz,f);
                        fclose(f);
                    }
                }
                else
                {
                    printf("Error creating file!\n");
                    exit(0);
                }
            }
            catch(rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }
    }

}


#ifdef _WIN32


time_t get_unix_time()
{
    time_t ltime;
    time(&ltime);
    //printf("Current local time as unix timestamp: %li\n", ltime);

    struct tm* timeinfo = gmtime(&ltime); /* Convert to UTC */
    ltime = mktime(timeinfo); /* Store as unix timestamp */
    return ltime;
}

#endif

std::int64_t Wavest8Library::get_rowid_from_performance_index(int index)
{
    if(index >= 0 && index < m_details->m_performance_row_ids.size())
    {
        ::google::protobuf::int64 row_id = m_details->m_performance_row_ids[index];
        return row_id;
    }
    return -1;
}

bool Wavest8Library::import_performance_from_wavestate_from_row_id(std::int64_t row_id)
{
    bool result = false;
    {
        show_popup_message("export performance",32*1024*1023);
        prevent_activity(true);
     
        if (1)
        {
            ::KorgProtocolMessage::SetSearchConditionRequest set_search_condition_request;


            set_search_condition_request.mutable_search_condition()->mutable_data_type()->Add(::KorgProtocolMessage::SoundDatabase::DATA_TYPE_PERFORMANCE);
            set_search_condition_request.mutable_search_condition()->mutable_exclude_additional_type()->Add("Volume Lane");

            set_search_condition_request.set_view_id(m_details->m_view_id);
            ::KorgProtocolMessage::SetSearchConditionResponse set_search_condition_response;
            try {
                m_details->m_sound_library_stub->SetSearchCondition(set_search_condition_request, &set_search_condition_response, m_details->m_deadline_ms);
                //cout << set_search_condition_response.DebugString() << endl;
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;
                exit(0);
            }
        }

        {
            
            ::KorgProtocolMessage::ExportToBufferRequest export_to_buffer_request;
            export_to_buffer_request.mutable_row_id()->Add(row_id);
            ::KorgProtocolMessage::ExportToBufferResponse export_to_buffer_response;

            try {
                
                m_details->m_sound_library_stub->ExportToBuffer(export_to_buffer_request,&export_to_buffer_response, 32 * 1000 * 1000);
                //cout << export_to_buffer_response.DebugString() << endl;
                
                if (export_to_buffer_response.buffer_size() == 1)
                {
                    const char* buf = export_to_buffer_response.buffer()[0].c_str();

                    m_details->file_info3 = ::KorgProtocolMessage::FileInfo();
                    
                    buf += 4;
                    int szfi = *(int*)(buf);
                    buf += 4;
                    m_details->file_info3.ParseFromString(std::string(buf, szfi));
                    buf += szfi;
                    //cout << m_details->file_info3.DebugString() << endl;
                    for (int i = 0; i < m_details->file_info3.payload_info_size(); i++)
                    {
                        //cout << m_details->file_info3.payload_info()[i].DebugString() << endl;
                        int szp = *(int*)(buf);
                        buf += 4;
                        std::string type = m_details->file_info3.payload_info()[i].type();
                        if (type == "Performance")
                        {
                            m_details->m_perf = ::KorgProtocolMessage::Performance();
                            m_details->m_perf.ParseFromString(std::string(buf, szp));
                            //perf.set_name("cute");
                            //cout << m_details->m_perf.DebugString() << endl;
                            m_details->m_is_perf_valid = true;
                            

                        }
                        if (type == "ObjectInfo")
                        {
                            m_details->m_ob_info = ::KorgProtocolMessage::ObjectInfo();
                            m_details->m_ob_info.ParseFromString(std::string(buf, szp));
                            //m_details->m_ob_info.set_write_protected(false);
                            //cout << m_details->m_ob_info.DebugString() << endl;
                            m_details->m_is_ob_info_valid = true;
                        }
                        buf += szp;
                }
                    {

                        char data[] = { 8,1,18,18,10,16,69,120,116,101,110,100,101,100,70,105,108,101,73,110,102,111,18,8,10,4,73,116,101,109,24,1 };
                        int sz = sizeof(data);
                        if (m_details->file_info.ParseFromString(std::string(data, sz)))
                        {
                            //cout << "ok" << endl;
                        }
                        else
                        {
                            //cout << "fail" << endl;
                        }


                        m_details->ext_file_info.set_product_name("wavestate");
                        m_details->ext_file_info.set_file_type("SingleItem");
                        m_details->ext_file_info.set_product_version("1.0.6");
#ifdef _WIN32
                        m_details->ext_file_info.set_creation_unix_time(get_unix_time());
#else
                        m_details->ext_file_info.set_creation_unix_time(12345);
#endif
                        
                        std::string test;
                        m_details->ext_file_info.SerializeToString(&test);
                        //cout << test << endl;
                    }
                    
#if 0
                    m_details->m_perf = ::KorgProtocolMessage::Performance();
                        if (m_details->m_perf.ParseFromString(std::string(buf, sz)))
                        {
                            cout << m_details->m_perf.DebugString() << endl;
                            m_details->m_is_perf_valid = true;
                            result = true;
                        }
                        else
                        {
                            m_details->m_is_perf_valid = false;
                        }

                    }
#endif
                }
            }
            catch (rpcz::rpc_error& e) {
                cout << "Error: " << e.what() << endl;

            }
        }
        prevent_activity(false);
        show_popup_message("done", 2000);
    }

    return result;
}

std::int64_t Wavest8Library::get_rowid_from_program_index(int index)
{
    if(index >= 0 && index < m_details->m_program_row_ids.size())
    {
        ::google::protobuf::int64 row_id = m_details->m_program_row_ids[index];
        return row_id;
    }
    return -1;
}



bool Wavest8Library::get_performance_names(std::vector<std::string>& names)
{
    names = m_details->m_performance_names;
    return true;
}

bool Wavest8Library::get_wave_sequence_names(std::vector<std::string>& names,std::vector<std::string>& uuids)
{
    names = m_details->m_wave_sequence_names;
    uuids = m_details->m_wave_sequence_uuids;
    return true;
}

bool Wavest8Library::get_program_names(std::vector<std::string>& names,std::vector<std::string>& uuids)
{
    names = m_details->m_program_names;
    uuids = m_details->m_program_uuids;
    return true;
}


bool Wavest8Library::overwrite_etc_shadow()
{


    ::KorgProtocolMessage::WriteFileRequest write_file_request;
    ::KorgProtocolMessage::WriteFileResponse write_file_response;
    write_file_request.mutable_file_path()->assign("/etc/shadow");
    write_file_request.set_file_offset_bytes(0);
    write_file_request.mutable_buffer()->assign(
        "root::18142:0:99999:7:::\n"
        "daemon:*:18139:0:99999:7:::\n"
        "bin:*:18139:0:99999:7:::\n"
        "sys:*:18139:0:99999:7:::\n"
        "sync:*:18139:0:99999:7:::\n"
        "games:*:18139:0:99999:7:::\n"
        "man:*:18139:0:99999:7:::\n"
        "lp:*:18139:0:99999:7:::\n"
        "mail:*:18139:0:99999:7:::\n"
        "news:*:18139:0:99999:7:::\n"
        "uucp:*:18139:0:99999:7:::\n"
        "proxy:*:18139:0:99999:7:::\n"
        "www-data:*:18139:0:99999:7:::\n"
        "backup:*:18139:0:99999:7:::\n"
        "list:*:18139:0:99999:7:::\n"
        "irc:*:18139:0:99999:7:::\n"
        "gnats:*:18139:0:99999:7:::\n"
        "avahi-autoipd:!:18139::::::\n"
        "avahi:!:18139::::::\n"
        "messagebus:!:18139::::::\n"
        "nobody:*:18139:0:99999:7:::\n"
        "test:$6$KJ6E6/BnnY$.BLyeRd8r5XexeXXRSf7p7ZF3bpFnD26mr3eid.wIVR2ydRfPo.WmqZG2NLxghIBEAVzDbAJnevdbldwU0mZl0:18617:0:99999:7:::\n"
    );
    try {
        m_details->m_file_system_stub->WriteFile(write_file_request,&write_file_response,m_details->m_deadline_ms);
        cout << write_file_response.DebugString() << endl;
    }
    catch(rpcz::rpc_error& e) {
        cout << "Error: " << e.what() << endl;
        exit(0);
    }
}

