#include "plugin.hpp"
#include "widgets.hpp"
#include "NoiseMachine.hpp"
#include "dsp/resampler.hpp"
#include "lfo.hpp"
#include "argen.hpp"

using namespace widgets;
using namespace rack;
using namespace dsp;
using namespace simd;

struct NoiseMachine : Module
{

    float lfoPhase = 0.f;
    float vcoPhase = 0.f;
    float vcoFreq = 0.f;
    float lfoOutput = 0.f;
    float lfoLED = 0.f;
    float vcfOutput = 0.f;
    float arGenOutput = 0.f;
    float sampleTime, sampleRate;

    LFO oscillators[3];
    ARGen arGen;

    NoiseMachine()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // VCO Section
        configParam(VCO_FREQ, -1.f, 2.f, 0.f, "Frequency", "Hz");
        configParam(VCO_SHAPE_SW, 0.f, 1.f, 1.f, "Shape");
        configParam(VCO_LFO_MOD, 0.f, .05f, 0.f, "LFO Mod Depth");
        configParam(VCO_AR_MOD, 0.f, 1.f, 0.f, "AR Mod Depth");
        configParam(VCO_AR_MOD_SW, 0.f, 1.f, 0.f, "AR On/Off");

        // VCF Section
        configParam(VCF_INPUT_SEL, 0.f, 1.f, 0.f, "Input Select");
        configParam(VCF_FREQ, 0.f, 1.f, 1.f, "Frequency", "Hz");
        configParam(VCF_RESO, 1.f, 100.f, 1.f, "Resonance", "%");
        configParam(VCF_MOD_DEPTH, 0.f, .3f, 0.f, "Modulation Depth");
        configParam(VCF_MOD_SRC, 0.f, 1.f, 1.f, "Modulation Source");

        // Output Section
        configParam(OUTPUT_VOLUME, 0.f, 1.f, 0.5f, "Output Volume");

        // AR Section
        configParam(AR_ATTACK, 2.5f, 2500.f, 2.5f, "AR Attack Time", "mS");
        configParam(AR_RELEASE, 2.5f, 3000.f, 2.5f, "AR Release Time", "ms");
        configParam(AR_REPEAT, 0.f, 1.f, 0.f, "AR Repeat");
        configParam(AR_MANUAL, 0.f, 1.f, 0.f, "AR Manual");
        arGen.reset();

        // LFO Section
        configParam(LFO_RATE, -7.f, -1.f, -2.f, "Freq", "Hz"); // 2Hz to 130Hz
        configParam(LFO_SHAPE_1, 0.f, 1.f, 0.f, "Shape");
        configParam(LFO_SHAPE_2, 0.f, 1.f, 0.f, "Shape");
    }

    void process(const ProcessArgs &args) override
    {
        sampleTime = args.sampleTime;
        sampleRate = args.sampleRate;

        // Process the LFO --------------------------------------------------------------
        lfoOutput = lfoProcess();
        outputs[LFO_OUTPUT].setVoltage(lfoOutput);
        lights[LFO_LED].setBrightness(lfoLED);

        // Process the VCO --------------------------------------------------------------
        vcoInit();
        float vcoFreq = params[VCO_SHAPE_SW].getValue() > 0.0f ? vcoRamp() : vcoSqr();
        float vcoVolt = vcoFreq * 5.f;

        outputs[VCO_OUTPUT].setVoltage(vcoVolt);

        // Generate some (Gaussian) noise -----------------------------------------------
        float noise = .5f * random::normal();

        // Compute the AR generator output ----------------------------------------------
        arGen.setAttackRate((params[AR_ATTACK].getValue() / 1000.0) * sampleRate);
        arGen.setReleaseRate((params[AR_RELEASE].getValue() / 1000.0) * sampleRate);
        // check to see if "Repeat" is on or one-shot pressed
        if (params[AR_REPEAT].getValue() > 0.0 || params[AR_MANUAL].getValue() > 0.0)
        {
            if (arGen.state == 0)
                arGen.state = 1;
        }
        // Re-trigger the AR generator if one-shot is pressed and we are not repeating
        if (params[AR_REPEAT].getValue() < 1.0 && params[AR_MANUAL].getValue() > 0.0) {
            if (arGen.state == 1) {
                arGen.reset();
                arGen.state = 1;
            }
        }
        arGenOutput = arGen.process();
        outputs[AR_OUTPUT].setVoltage(arGenOutput);
        //DEBUG("arGen output = %f",arGenOutput);

        // process the VCF --------------------------------------------------------------
        if (params[VCF_INPUT_SEL].getValue() < 1.f)
            vcfOutput = vcfProcess(vcoFreq);
        else
            vcfOutput = vcfProcess(noise);

        // process the VCA --------------------------------------------------------------
        if (params[VCA_SWITCH].getValue() < 1.f) {             // apply VCA to VCF output
            vcfOutput = clamp((vcfOutput * arGenOutput), -5.f, 5.f);
        }

        // Set the final output ---------------------------------------------------------
        outputs[FINAL_OUTPUT].setVoltage(clamp((vcfOutput * params[OUTPUT_VOLUME].getValue()), -5.f, 5.f));

    } // end of process loop

    // LFO Section ----------------------------------------------------------------------

    float lfoProcess()
    {
        float freqParam = params[LFO_RATE].getValue();
        float waveParam1 = params[LFO_SHAPE_1].getValue();
        float waveParam2 = params[LFO_SHAPE_2].getValue();
        float vOut = 0.f;

        float multiplier = rescaleOutput(freqParam);

        for (int c = 0; c < 3; c++)
        {
            auto *oscillator = &oscillators[c];

            float pitch = freqParam;
            oscillator->setPitch(pitch);
            oscillator->process(sampleTime);

            // Outputs
            if (waveParam1 < 1.f)
            {
                vOut += oscillator->tri();
                lfoLED = oscillator->light();
            }
            else if (waveParam2 < 1.f)
            {
                vOut += oscillator->sqr();
                lfoLED = oscillator->light();
            }
            else
            {
                vOut += oscillator->pls() * multiplier;
                lfoLED = oscillator->light();
            }
        }

        return vOut;
    }

    // Rescale output voltage so it stays constant across range
    // Code provided by my youngest son, Jack Spink, Programmer, Sumo Digital Nottingham
    float rescaleOutput(float input)
    {
        float initialRangeStart = paramQuantities[LFO_RATE]->getMinValue();
        float initialRangeEnd = paramQuantities[LFO_RATE]->getMaxValue();
        float remapRangeStart = 0.6f;
        float remapRangeEnd = 0.9f;
        float valuePercent = (input - initialRangeStart) / (initialRangeEnd - initialRangeStart);

        return remapRangeStart + (remapRangeEnd - remapRangeStart) * valuePercent;
    }

    // VCO Section ----------------------------------------------------------------------
    void vcoInit()
    {
        float rate = params[VCO_FREQ].getValue();
        rate += inputs[PITCH_INPUT].getVoltage() / 5.f;             // add in external cv
        rate += lfoOutput * params[VCO_LFO_MOD].getValue();         // add in LFO modulation
        if (params[VCO_AR_MOD_SW].getValue() > 0.0) {               // add in SR generator if enabled
            rate += arGenOutput * params[VCO_AR_MOD].getValue();
        } 

        vcoFreq = dsp::FREQ_C4 * std::pow(2.f, (rate * 2.0));
        vcoPhase += vcoFreq * sampleTime;
        if (vcoPhase >= 0.5f)
            vcoPhase -= 1.f;
    }

    float vcoRamp()
    {
        return (2 * (vcoPhase + 0.5f)) - 1.f;
    }

    float vcoSqr()
    {
        return vcoPhase < 0.f ? -1.f : 1.f;
    }

    // VCF Section ----------------------------------------------------------------------
    float vcfProcess(float input)
    {
        float mod = 0.0;
        if (params[VCF_MOD_SRC].getValue() > 0.0) {         // mod source is AR Generator
            mod = arGenOutput * params[VCF_MOD_DEPTH].getValue();
        } else {                                            // mod source is LFO
            mod = lfoOutput * params[VCF_MOD_DEPTH].getValue();
        }
        
        float cutoffFreq = std::pow(2.f, rescale(clamp(params[VCF_FREQ].getValue() + mod, 0.f, 1.f), 0.f, 1.f, 4.5f, 13.f));
        return vcfCalcOutput(input, cutoffFreq);
    }

    float vcfCalcOutput(float input, float cutOff)
    {
        static float mem1 = 0.f;
        static float mem2 = 0.f;
        static float lp = 0.f;
        float g = tan(float(M_PI) * cutOff / sampleRate);
        float R = 1.0f / (2.0f * params[VCF_RESO].getValue());
        float hp = (input - (2.0f * R + g) * mem1 - mem2) / (1.0f + 2.0f * R * g + g * g);
        float bp = g * hp + mem1;
        lp = g * bp + mem2;
        mem1 = g * hp + bp;
        mem2 = g * bp + lp;
        return lp;
    }
};

struct NMWidget : ModuleWidget
{
    template <typename T>
    T *adjust(T *w)
    {
        w->box.pos.x -= w->box.size.x / 2;
        w->box.pos.y -= w->box.size.y / 2;

        return w;
    }

    NMWidget(NoiseMachine *module)
    {
        INFO("%s","Welcome to Noise Machine!");
        setModule(module);
        auto panel = APP->window->loadSvg(asset::plugin(pluginInstance, "res/NoiseMachineV3.svg"));
        setPanel(panel);

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(98.f, 118.f)), module, LightIds::LFO_LED));

        // Code added by my eldest son, Dr. Tom Spink, University of Edinburgh
        // Loop over each shape in the panel SVG and place control.
        for (auto shape = panel->handle->shapes; shape; shape = shape->next)
        {
            //fprintf(stderr,"Shape= %s\n",shape->id);
            // If the shape ID starts with a $, then this is a widget placeholder.
            if (shape->id[0] == '$')
            {
                // Hide the placeholder.
                shape->opacity = 0.f;

                // Extract the details of the widget, i.e. its type and id.
                char type[16];
                int id;

                sscanf(shape->id, "$%[A-Z]_%d", type, &id);

                auto loc = Vec(
                    shape->bounds[0] + ((shape->bounds[2] - shape->bounds[0]) / 2.0),
                    shape->bounds[1] + ((shape->bounds[3] - shape->bounds[1]) / 2.0));

                //fprintf(stderr, "widget: type=%s, id=%d @ %f,%f\n", type, id, loc.x, loc.y);
                if (!strcmp(type, "KNOB"))
                {
                    addParam(adjust(createParam<Knob32>(loc, module, id)));
                }
                else if (!strcmp(type, "SWITCH"))
                {
                    addParam(adjust(createParam<ToggleSwitch2State>(loc, module, id)));
                }
                else if (!strcmp(type, "OUTPUT"))
                {
                    addOutput(adjust(createOutput<JackSocket>(loc, module, id)));
                }
                else if (!strcmp(type, "INPUT"))
                {
                    addInput(adjust(createInput<JackSocket>(loc, module, id)));
                }
                else if (!strcmp(type, "BUTTON"))
                {
                    addParam(adjust(createParam<PushButton>(loc, module, id)));
                }
            }
        }
    }
};

Model *modelNoiseMachine = createModel<NoiseMachine, NMWidget>("NoiseMachine");
