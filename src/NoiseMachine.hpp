#pragma once

enum ParamIds {
    VCO_FREQ,
    VCO_LFO_MOD,
    VCO_AR_MOD,
    VCF_FREQ,
    VCF_RESO,
    VCF_MOD_DEPTH,
    AR_ATTACK,
    AR_RELEASE,
    LFO_RATE,
    OUTPUT_VOLUME,
    VCO_SHAPE_SW,
    VCO_AR_MOD_SW,
    VCF_INPUT_SEL,
    VCF_MOD_SRC,
    VCA_SWITCH,
    AR_REPEAT,
    AR_MANUAL,
    LFO_SHAPE_1,
    LFO_SHAPE_2,
    NUM_PARAMS
};

enum InputIds {
    PITCH_INPUT,
    NUM_INPUTS
};

enum OutputIds {
    FINAL_OUTPUT,
    LFO_OUTPUT,
    VCO_OUTPUT,
    NUM_OUTPUTS
};

enum LightIds {
    LFO_LED,
    NUM_LIGHTS
};
