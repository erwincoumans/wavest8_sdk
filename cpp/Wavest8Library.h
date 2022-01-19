#ifndef WAVEST8_LIBRARY_H
#define WAVEST8_LIBRARY_H
#include <vector>
#include <string>


#include <cstdint>

static const unsigned char s_multi_sample_uuid[] ={75,111,114,103,0,0,0,0,0,2,0,16,0,0,0,0};

struct Wavest8ObjectInfo
{
    std::string m_name;
};



struct IntModulation{
    int m_lower_bound;
    int m_upper_bound;
    int m_scaling;
    int m_modulation_source;
    int m_intensity;
    int m_intensity_mod_source;
    int m_modulation_uuid;
    IntModulation(int lower_bound, int upper_bound, int scaling,int modulation_uuid)
        :
        m_lower_bound(lower_bound),
        m_upper_bound(upper_bound),
        m_scaling(scaling),
        m_modulation_source(0),
        m_intensity(0),
        m_intensity_mod_source(0),
        m_modulation_uuid(modulation_uuid)
    {
    }


};

enum MODULATION_TYPE
{
    MY_MODULATION_FLOAT = 15,
    MY_MODULATION_INT,
    MY_MODULATION_UNDEFINED,
};




struct FloatModulation{
    float m_lower_bound;
    float m_upper_bound;
    float m_scaling_factor;

    int m_modulation_source;
    float m_intensity;
    int m_intensity_mod_source;
    int m_modulation_uuid;
    //int source_output_index;
    //int intensity_mod_source_output_index;
    FloatModulation(float lower_bound, float upper_bound, float scaling_factor,int modulation_uuid)
        :
            m_lower_bound(lower_bound),
            m_upper_bound(upper_bound),
            m_scaling_factor(scaling_factor),
            m_modulation_source(0),
            m_intensity(0),
            m_intensity_mod_source(0),
        m_modulation_uuid(modulation_uuid)
    {
    }
};


struct FloatModulationList
{
    float value;
    std::vector<FloatModulation> m_modulations;
};

struct IntModulationList
{
    int value;
    std::vector<IntModulation> m_modulations;
};

struct Pitch
{
    FloatModulationList m_tune;
    IntModulationList m_transpose_octaves;
};



/// warning: those are using 'UI' values starting from '1'
/// while the actual data is stored starting from 0 !
struct SequenceControlParams {
    int number_of_steps;
    int start_step;
    int end_step;
    int loop_start_step;
    int loop_end_step;
    int loop_repeat_count;
    int loop_type;
    int note_on_advance;
    int randomization;

    //enum LoopType {
    //    LOOP_TYPE_FORWARD = 0;
    //LOOP_TYPE_BACKWARD_FORWARD = 1;
    //LOOP_TYPE_BACKWARD = 2;
    //}

    SequenceControlParams()
        :start_step(1),
        end_step(1),
        loop_start_step(1),
        loop_end_step(1),
        loop_repeat_count(0),
        loop_type(0),
        note_on_advance(0),
        randomization(0)
    {
    }
};

struct SampleLaneStep
{
    std::string m_multiSampleUuid;
    int m_payload_n;
    float m_probability;
    int m_octave;
    //message HD2Step{
    //SampleReference multisample = 1;
    //IntParameter channel_select = 2;
    //IntParameter start_offset = 3;
    //BoolParameter reverse = 4;
    //IntParameter octave = 5;
    //FloatParameter trim_db = 6;
    //FloatParameter probability_unsigned_percent = 7;
    //IntParameter transpose_semitones = 8;
    //FloatParameter tune_cents = 9;
    //FixedPitchSelect fixed_pitch_select = 10;
    //}

    SampleLaneStep()
        :m_payload_n(-1),
        m_probability(100),
        m_octave(0)
    {
        m_multiSampleUuid = std::string((char*)s_multi_sample_uuid,sizeof(s_multi_sample_uuid));
        
    }

};


struct TimingLaneStep
{
    int m_step_type; //note, rest, gate
    int m_step_tempo_duration;
    float m_step_time_duration_seconds;
    float m_probability;
    float m_crossfade_secs;

    TimingLaneStep()
        :m_step_type(0),
        m_step_tempo_duration(0),
        m_step_time_duration_seconds(0.2886),
        m_probability(100),
        m_crossfade_secs(0.0)
    {
    }
};

struct ShapeLaneStep
{
    int m_shape_type;
    float m_offset;
    float m_level;
    float m_start_phase_degrees;
    float m_probability_unsigned_percent;
    int scale_by_gate;
    ShapeLaneStep()
        :m_shape_type(4),
        m_offset(0),
        m_level(1),
        m_start_phase_degrees(0),
        scale_by_gate(0),
        m_probability_unsigned_percent(100)
    {
    }

};

struct GateLaneStep
{
    float m_value_unsigned_percent;
    float probability_unsigned_percent;
    GateLaneStep()
        :m_value_unsigned_percent(100),
        probability_unsigned_percent(100)
    {
    }
};

struct PitchLaneStep
{
    int m_transpose;
    float m_tune_cents;
    float m_probability_unsigned_percent;
    //int step_transition_type;
    PitchLaneStep()
        :m_transpose(0),
        m_tune_cents(0),
        m_probability_unsigned_percent(100)
    {
    }
};
struct StepSeqStep
{
    float m_value_signed_percent;
    int m_value_mode;//value, random, sample hold, random unipolar
    float m_probability;
    int m_step_transition_type;
    
    //StepSeqStep_ValueMode_VALUE_MODE_VALUE = 0,
    //StepSeqStep_ValueMode_VALUE_MODE_RANDOM = 1,
    //StepSeqStep_ValueMode_VALUE_MODE_SAMPLE_HOLD = 2,
    //StepSeqStep_ValueMode_VALUE_MODE_RANDOM_UNIPOLAR = 3,
    //StepSeqStep_ValueMode_StepSeqStep_ValueMode_INT_MIN_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32min,
    //StepSeqStep_ValueMode_StepSeqStep_ValueMode_INT_MAX_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32max
    StepSeqStep()
        :m_value_mode(0),
        m_value_signed_percent(100),
        m_probability(100),
        m_step_transition_type(0)
    {
    }

};

struct Trigger
{
    bool trigger_at_note_on;
    int modulation_source_type;
    float threshold_signed_percent;
};

enum FILTER_TYPE
{
    BYPASS_FILTER = 1,
    TWO_POLE_LOW_PASS_FILTER = 10,
    FOUT_POLE_LOW_PASS_FILTER = 11,
    TWO_POLE_RESONANT_LOW_PASS_FILTER = 12,
    TWO_POLE_RESONANT_HIGH_PASS_FILTER = 13,
    TWO_POLE_RESONANT_BAND_PASS_FILTER = 14,
    TWO_POLE_RESONANT_BAND_REJECT_FILTER = 15,
    FOUR_POLE_RESONANT_LOW_PASS_FILTER = 16,
    FOUR_POLE_RESONANT_HIGH_PASS_FILTER = 17,
    FOUR_POLE_RESONANT_BAND_PASS_FILTER = 18,
    FOUR_POLE_RESONANT_BAND_REJECT_FILTER = 19,
    TWO_POLE_MULTI_FILTER = 20,
    MS20_HIGH_PASS_FILTER = 21,
    MS20_LOW_PASS_FILTER = 22,
    POLYSIX_FILTER = 23,
    ODYSSEY_REV3_FILTER = 26,
    TYPE_NOT_SET_FILTER = 0,
};


struct Wavest8Filter
{
    int m_filter_type;
    FloatModulationList m_frequency_cutoff_semi_tones;
    FloatModulationList m_resonance_percent;

    Wavest8Filter()
        :m_filter_type(TYPE_NOT_SET_FILTER)
    {
    }
};

struct Wavest8PerformanceTrackInfo
{
    int m_enabled_track;
    int m_has_wave_sequence;
    Wavest8Filter m_filter;
    
    std::string m_program_name;
    std::string m_program_copied_from_uuid;

    std::string m_multi_sample_instrument_uuid;

    bool use_master_lane;

    int m_timing_lane_mode_use_tempo;
    SequenceControlParams m_timing_lane_control_params;
    TimingLaneStep m_waveseq_timing_lane_steps[64];
    int m_num_waveseq_timing_lane_steps;

    SequenceControlParams m_sample_lane_control_params;
    SampleLaneStep m_waveseq_sample_lane_steps[64];
    int m_num_waveseq_sample_lane_steps;

    SequenceControlParams m_pitch_lane_control_params;
    PitchLaneStep m_waveseq_pitch_lane_steps[64];
    int m_num_waveseq_pitch_lane_steps;

    SequenceControlParams m_shape_lane_control_params;
    ShapeLaneStep m_waveseq_shape_lane_steps[64];
    int m_num_waveseq_shape_lane_steps;
    
    SequenceControlParams m_gate_lane_control_params;
    GateLaneStep m_waveseq_gate_lane_steps[64];
    int m_num_waveseq_gate_lane_steps;
    
    SequenceControlParams m_stepseq_lane_control_params;
    StepSeqStep m_waveseq_stepseq_lane_steps[64];
    int m_num_waveseq_stepseq_lane_steps;

    Trigger m_trigger;
    
    Pitch m_pitch;

    FloatModulationList m_adrs_attack;//seconds
    FloatModulationList m_adrs_decay;
    
    int m_adrs_sustain;//percent
    float m_adrs_release;//seconds
    bool m_trigger_at_note_on;
    
    IntModulationList m_hold_note;

    
    Wavest8PerformanceTrackInfo()
        :m_enabled_track(0),
        m_has_wave_sequence(0),
        m_num_waveseq_sample_lane_steps(1),
        m_num_waveseq_timing_lane_steps(1),
        m_num_waveseq_pitch_lane_steps(1),
        m_num_waveseq_shape_lane_steps(1),
        m_num_waveseq_gate_lane_steps(1),
        m_num_waveseq_stepseq_lane_steps(1),
        m_timing_lane_mode_use_tempo()
    {
        m_multi_sample_instrument_uuid = std::string((char*)s_multi_sample_uuid,sizeof(s_multi_sample_uuid));
    }
};

struct Wavest8PerformanceInfo
{
    std::string m_name;
    float m_tempo;
    bool m_write_protected;
    Wavest8PerformanceTrackInfo m_tracks[4];

    Wavest8PerformanceInfo()
        :m_write_protected(false),
        m_tempo(100)
    {
    }
};

class Wavest8Library
{
  struct Wavest8LibraryDetails* m_details;
  
  const char* parse_korg_proto(const char* orgbuf, int size, const std::string& type);
  bool export_performance_to_buffer(struct MemoryBuffer& mem_buffer, bool add_initial_size, bool clear_write_protected);
public:
    Wavest8Library(int deadline_ms = 5000);
    virtual ~Wavest8Library();
    
    void connect();
    void disconnect();
    bool is_connected();


    bool create_view();

    bool select_performance(std::int64_t row_id);
    bool get_current_performance(std::string& performance,std::uint64_t& object_id);

    void show_popup_message(const char* msg, int time_ms);
    void prevent_activity(bool prevent);

    void note_on_off(int note_val, int channel, int velocity, bool on);
    void send_raw_midi_message(std::string msg);
    
    void get_multi_sample_list(std::vector<std::string>& samples, std::vector<std::string>& uuids);
    void get_instruments(std::vector<std::string>& instruments);

    void export_all();
    void get_all_collection_info();
    
    bool import_file(const std::string& file_name);
    bool export_file(const std::string& file_name);
    
    bool get_object_info(Wavest8ObjectInfo& ob_info);
    bool set_object_info(const Wavest8ObjectInfo& ob_info);

    bool get_performance_info(Wavest8PerformanceInfo& perf_info);
    bool set_performance_info(const Wavest8PerformanceInfo& perf_info);
    
    bool export_performance_to_wavestate(bool overwrite, bool select_on_wavestate,std::int64_t& new_row_id);
    
    bool import_performance_from_wavestate_from_row_id(std::int64_t row_id);
    std::int64_t get_rowid_from_performance_index(int index);

    std::int64_t get_rowid_from_program_index(int index);

    bool get_object_path(const std::string& object_path, std::string& output_name,std::uint64_t& object_id);
    
    //only call this when needed, not all the time (slowish)
    void updatePerformanceList();

    bool get_performance_names(std::vector<std::string>& names);
    bool get_wave_sequence_names(std::vector<std::string>& names,std::vector<std::string>& uuids);
    bool get_program_names(std::vector<std::string>& names,std::vector<std::string>& uuids);

    bool replace_program(int track_index, const std::string& program_uuid);

    bool overwrite_etc_shadow();
    
};

#endif //WAVEST8_LIBRARY_H
