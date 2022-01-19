#ifndef MODULATION_ENUMS_H
#define MODULATION_ENUMS_H

#include "Wavest8Library.h"

typedef void (*AddFloatLinkCallback)(int from_uid,int to_uid,const FloatModulation& mod);
typedef void (*DeleteFloatLinkCallback)(int from_uid,int to_uid,const FloatModulation& mod);
typedef void (*ModifyFloatLinkIntensityCallback)(int node_uid,const FloatModulation& mod);

typedef void (*AddIntLinkCallback)(int from_uid,int to_uid,const IntModulation& mod);
typedef void (*DeleteIntLinkCallback)(int from_uid,int to_uid,const IntModulation& mod);
typedef void (*ModifyIntLinkIntensityCallback)(int node_uid,const IntModulation& mod);

std::string blueprint_add_float_link(int korgfrom,int korgto,const FloatModulation& mod);
std::string blueprint_add_int_link(int korgfrom,int korgto,const IntModulation& mod);

void  blueprint_register_addFloatLink_callback(AddFloatLinkCallback addLink);
void  blueprint_register_deleteFloatLink_callback(DeleteFloatLinkCallback deleteLink);
void  blueprint_register_modifyFloatLink_callback(ModifyFloatLinkIntensityCallback modifyLink);

void  blueprint_register_addIntLink_callback(AddIntLinkCallback addLink);
void  blueprint_register_deleteIntLink_callback(DeleteIntLinkCallback deleteLink);
void  blueprint_register_modifyIntLink_callback(ModifyIntLinkIntensityCallback modifyLink);


void blueprint_clearLinks(const std::string& settings_file_name);


inline int track_and_id_to_uid(int track,int id)
{
    int uid = id+(track<<22);
    return uid;
}

inline void uid_to_track_and_id(int uid, int &track,int &id)
{
    int num = ((1<<22)-1);
    id = uid & num;
    track = uid >> 22;
}

#define MAX_MOD_TARGET_DUPLICATES 16

enum ModulationEnums
{
    MOD_WHEEL = 1,
    MOD_PITCH_BEND = 258,
    MOD_VECTOR_JOYSTICK_X_16 = 143,
    MOD_VECTOR_JOYSTICK_Y_17 = 144,

    MOD_PERF_KNOB_MASTER=403,
    MOD_PERF_KNOB_TIMING=404,
    MOD_PERF_KNOB_SAMPLE=405,
    MOD_PERF_KNOB_PITCH=406,
    MOD_PERF_KNOB_SHAPE=407,
    MOD_PERF_KNOB_GATE=408,
    MOD_PERF_KNOB_SEQ=409,
    MOD_PERF_KNOB_SPEED=410,

    MOD_KNOB_MASTER=600,
    MOD_KNOB_TIMING=601,
    MOD_KNOB_SAMPLE=602,
    MOD_KNOB_PITCH=603,
    MOD_KNOB_SHAPE=604,
    MOD_KNOB_GATE=605,
    MOD_KNOB_SEQ=606,
    MOD_KNOB_SPEED=607,

    MOD_FILTER_LFO=20004,
    MOD_AMP_LFO=20005,
    MOD_PITCH_LFO=20006,
    MOD_PAN_LFO=20007,

    MOD_KNOB_UNUSED_RANGE_BEGIN=1024,
    MOD_KNOB_UNUSED_RANGE_END=65536-1,

    MOD_TARGET_PITCH_TUNE=65536,
    MOD_TARGET_PITCH_OCTAVE=MOD_TARGET_PITCH_TUNE+MAX_MOD_TARGET_DUPLICATES,
    MOD_TARGET_AMP_ATTACK=MOD_TARGET_PITCH_OCTAVE+MAX_MOD_TARGET_DUPLICATES,
    MOD_TARGET_AMP_DECAY=MOD_TARGET_AMP_ATTACK+MAX_MOD_TARGET_DUPLICATES,
    MOD_TARGET_FILTER_CUTOFF_FREQUENCY=MOD_TARGET_AMP_DECAY+MAX_MOD_TARGET_DUPLICATES,

};


inline FloatModulation CreatePitchTuneModulation(int modulation_uuid)
{
    return FloatModulation(-144,144,100.,modulation_uuid);
}

inline FloatModulation CreateAMPAttackModulation(int modulation_uuid)
{
    return FloatModulation(0.000005855,17080,1,modulation_uuid);
}
inline FloatModulation CreateAMPDecayModulation(int modulation_uuid)
{
    return FloatModulation(0.000005855,17080,1,modulation_uuid);
}

inline FloatModulation  CreateFilterCutoffFrequencyModulation(int modulation_uuid)
{
    return FloatModulation(-142,142,1,modulation_uuid);
}


inline IntModulation CreatePitchOctaveModulation(int modulation_uuid)
{
    return IntModulation(-4,4,1,modulation_uuid);
}





#endif //MODULATION_ENUMS_H