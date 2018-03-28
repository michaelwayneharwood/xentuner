#ifndef __XENTUNER__
#define __XENTUNER__

#include "IPlug_include_in_plug_hdr.h"

class XenTuner : public IPlug
{
public:
  XenTuner(IPlugInstanceInfo instanceInfo);
  ~XenTuner();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  int mMeter;
};

#endif
