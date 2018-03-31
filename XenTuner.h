#ifndef __XENTUNER__
#define __XENTUNER__

#include "IPlug_include_in_plug_hdr.h"
#include "scala-import.h"

class XenTuner : public IPlug
{
public:
    XenTuner(IPlugInstanceInfo instanceInfo);
    ~XenTuner();

    void Reset();
    void OnParamChange(int paramIdx);
    void TuningMap_Generate();
    void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
    double EstimatePeriod(const double * x, const int n, const int minP, const int maxP, double & q);

    char pitch[24] = "0.000";
    char scale_deg[512];
    char scale_desc[512];
    char base_freq[512]="261.62556";
    char diff_cents[512]="0.000";


    ITextControl* pitch_text;
    ITextControl* scale_degree;
    ITextControl* scale_description;
    ITextControl* base_frequency;
    ITextControl* diff_in_cents;
    IBitmapControl* meter_control;



    double buffer[6144] = { 0 };
    int buffer_pointer = 0;
    double pEst = 0;

private:
    int mMeter;

    ScalaScaleFile scl;
    
    int scl_loaded = 0; //Flag to determine whether to load inital values for the SCL fields

    // Tuning Map 
    struct TuningMap
    {
        double freq[2048];
        int degree[2048];
    } tunmap;
};
#endif
