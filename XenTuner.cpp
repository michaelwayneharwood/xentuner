#include "XenTuner.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 1;

enum EParams
{
  kMeter = 0,
  kNumParams
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,

  kMeterX = 33,
  kMeterY = 176,
  kMeterFrames = 41
};
  
XenTuner::XenTuner(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mMeter(20)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, label
  GetParam(kMeter)->InitInt("Meter", 20, 0, 40, "Meter_Level");
  GetParam(kMeter)->SetShape(1);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BACKGROUND_ID, BACKGROUND_FN);

  IBitmap meter = pGraphics->LoadIBitmap(METER_ID, METER_FN, kMeterFrames);

  pGraphics->AttachControl(new IBitmapControl(this, kMeterX, kMeterY, kMeter, &meter));


  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

XenTuner::~XenTuner() {}


void XenTuner::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1;
    *out2 = *in2;
  }
}

void XenTuner::Reset()
{
  TRACE;
  IMutexLock lock(this);
}

void XenTuner::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {

    default:
      break;
  }
}
