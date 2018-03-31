// ===================================================================
//  XenTuner 
//
//
//  Released under the MIT License
//
//  The MIT License (MIT)
//
//  Copyright (c) 2018 Michael Wayne Harwood
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
// ===================================================================

#include "XenTuner.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <vector>

const int kNumPrograms = 1;

enum EParams
{
  kMeter = 0,
  kScaleDegree,
  kScaleDescription,
  kPitchText,
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

  if (!scl_loaded)
  {
      scl_loaded = 1;
      // Initialize 12EDO as base scale
      sprintf(scl.description, "24EDO");
      scl.notes = 24;
      scl.cents[0] = 50.;
      scl.cents[1] = 100.;
      scl.cents[2] = 150.;
      scl.cents[3] = 200.;
      scl.cents[4] = 250.;
      scl.cents[5] = 300.;
      scl.cents[6] = 350.;
      scl.cents[7] = 400.;
      scl.cents[8] = 450.;
      scl.cents[9] = 500.;
      scl.cents[10] = 550.;
      scl.cents[11] = 600.;
      scl.cents[12] = 650.;
      scl.cents[13] = 700.;
      scl.cents[14] = 750.;
      scl.cents[15] = 800.;
      scl.cents[16] = 850.;
      scl.cents[17] = 900.;
      scl.cents[18] = 950.;
      scl.cents[19] = 1000.;
      scl.cents[20] = 1050.;
      scl.cents[21] = 1100.;
      scl.cents[22] = 1150.;
      scl.cents[23] = 1200.;
  }

  TuningMap_Generate();

  //arguments are: name, defaultVal, minVal, maxVal, label
  GetParam(kMeter)->InitInt("Meter", 20, 0, 40, "Meter_Level");
  //GetParam(kMeter)->SetShape(1);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BACKGROUND_ID, BACKGROUND_FN);

  IBitmap meter = pGraphics->LoadIBitmap(METER_ID, METER_FN, kMeterFrames);
  meter_control = new IBitmapControl(this, kMeterX, kMeterY, kMeter, &meter);
  pGraphics->AttachControl(meter_control);

  IText textProps(20, &COLOR_WHITE, "Ariel", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
  
  // Base Freqency
  base_frequency = new ITextControl(this, IRECT(210, 95, 322, 115), &textProps, base_freq);
  pGraphics->AttachControl(base_frequency);

  // Diff in Cents
  diff_in_cents = new ITextControl(this, IRECT(213, 232, 302, 252), &textProps, diff_cents);
  pGraphics->AttachControl(diff_in_cents);
  
  // Scale Description
  sprintf(scale_desc, "%s", scl.description);
  scale_description = new ITextControl(this, IRECT(48, 56, 350, 76), &textProps, scale_desc);
  pGraphics->AttachControl(scale_description);

  // Scale Degree
  sprintf(scale_deg, "%.3lf", tunmap.freq[1048]);
  scale_degree = new ITextControl(this, IRECT(104, 152, 295, 172), &textProps, scale_deg);
  pGraphics->AttachControl(scale_degree);
  
  // Detected Pitch
  pitch_text = new ITextControl(this, IRECT(100, 232, 185, 252), &textProps, pitch);
  pGraphics->AttachControl(pitch_text);

  //pGraphics->ShowControlBounds(true);

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

    double freq_diff;
    double closest_freq = 0;
    int temp_scale_degree = 0;
    double curr_freq;
    double next_cents;
    double prev_cents;
    double curr_diff_cents;

    double q;

    for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
    {
        *out1 = *in1;
        *out2 = *in2;
        if (buffer_pointer != 6144)
        {
            buffer[buffer_pointer] = *in1;
            buffer_pointer++;
        }

    }
    // Arguments are (double* buffer, number of samples, min period, max period, quality) 

    if (buffer_pointer == 6144)
    {
        pEst = EstimatePeriod(buffer, 6144, 10, 3072, q);
        buffer_pointer = 0;

        double fEst = 0;
        if (pEst > 0)
            fEst = 44100 / pEst;

        sprintf(pitch, "%.3lf", fEst);
        pitch_text->SetTextFromPlug(pitch);

        freq_diff = abs(fEst - tunmap.freq[2048]);

        for (int i = 0; i < 2048; i++)
        {
            curr_freq = abs(fEst - tunmap.freq[i]);
            if (curr_freq < freq_diff)
            {
                freq_diff = curr_freq;
                closest_freq = tunmap.freq[i];
                temp_scale_degree = i;
            }
        }

        // Set Scale Degree Field in GUI
        if (fEst == 0)
            sprintf(scale_deg, "%s", " ");
        else
            if (temp_scale_degree == 1024)
                sprintf(scale_deg, "[B] %.3lf", closest_freq);
            else
                sprintf(scale_deg, "[%d] %.3lf", tunmap.degree[temp_scale_degree], closest_freq);

        scale_degree->SetTextFromPlug(scale_deg);

        // Set "Differnce in Cents" Field in GUI
        curr_diff_cents = 1200 * log2(fEst / closest_freq);

        if (fEst == 0)
        {
            sprintf(diff_cents, " ");
            meter_control->SetValueFromPlug(.5);
        }
        else
            if (curr_diff_cents >= 0)
            {
                sprintf(diff_cents, "+%.3lf", abs(curr_diff_cents) );
            }
            else
            {
                sprintf(diff_cents, "-%.3lf", abs(curr_diff_cents) );
            }
        diff_in_cents->SetTextFromPlug(diff_cents);

        // Update Meter
        if ((temp_scale_degree > 0) && (temp_scale_degree < 2048))
        {
            prev_cents = abs(1200 * log2(tunmap.freq[temp_scale_degree] / tunmap.freq[temp_scale_degree - 1]));
            next_cents = abs(1200 * log2(tunmap.freq[temp_scale_degree] / tunmap.freq[temp_scale_degree + 1]));

            if(curr_diff_cents > 0)
                for (int i = 21; i < 41; i++)
                {
                    double temp = (next_cents / 40.)*(i-20);
                    if (curr_diff_cents >= temp)
                        meter_control->SetValueFromPlug( ((1. / 41.) * i)+.024);
              
                }
            else
                for (int i = 19; i >= 0; i--)
                {
                    double temp = (prev_cents / 40.)*(i-20);
                    if (curr_diff_cents <= temp)
                        meter_control->SetValueFromPlug( ((1. / 41.) * i) );
                }

            if (abs(curr_diff_cents) < 1.)
                meter_control->SetValueFromPlug(.5);
        
        
        }

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

void XenTuner::TuningMap_Generate()
{
    // Generate Tuning Frequency Map
    double base_freq;
    int note_counter;
    int pitch_bend_temp;

    base_freq = 261.625565300599;
    tunmap.freq[1024] = base_freq;
    tunmap.degree[1024] = 0;

    note_counter = 0;
    int incr = 1023;
    while (incr > -1)
    {
        tunmap.freq[incr] = base_freq / (pow(2, scl.cents[note_counter] / 1200));
 
        note_counter += 1;
        tunmap.degree[incr] = scl.notes - note_counter;
        if (note_counter == scl.notes)
        {
            note_counter = 0;
            base_freq = tunmap.freq[incr];
        }

        incr -= 1;
    }

    base_freq = 261.625565300599;
    note_counter = 0;
    incr = 1025;
    while (incr < 2048)
    {
        tunmap.freq[incr] = base_freq * (pow(2, scl.cents[note_counter] / 1200));

        note_counter += 1;
        tunmap.degree[incr] = note_counter;
        if (note_counter == scl.notes)
        {
            note_counter = 0;
            base_freq = tunmap.freq[incr];
        }



        incr += 1;
    }
}



// ===================================================================
//  EstimatePeriod
//
//  Returns best estimate of period.
// ===================================================================
double XenTuner::EstimatePeriod(
    const double    *x,         //  Sample data.
    const int       n,          //  Number of samples.  Should be at least 2 x maxP
    const int       minP,       //  Minimum period of interest
    const int       maxP,       //  Maximum period
    double&         q)         //  Quality (1= perfectly periodic)
{

    q = 0;

    //  --------------------------------
    //  Compute the normalized autocorrelation (NAC).  The normalization is such that
    //  if the signal is perfectly periodic with (integer) period p, the NAC will be
    //  exactly 1.0.  (Bonus: NAC is also exactly 1.0 for periodic signal
    //  with exponential decay or increase in magnitude).

    std::vector <double> nac(maxP + 2); // Size is maxP+2 (not maxP+1!) because we need up to element maxP+1 to  check                       
                                        // whether element at maxP is a peak. Thanks to Les Cargill for spotting the bug. 

    for (int p = minP - 1; p <= maxP + 1; p++)
    {
        double ac = 0.0;        // Standard auto-correlation
        double sumSqBeg = 0.0;  // Sum of squares of beginning part
        double sumSqEnd = 0.0;  // Sum of squares of ending part

        for (int i = 0; i < n - p; i++)
        {
            ac += x[i] * x[i + p];
            sumSqBeg += x[i] * x[i];
            sumSqEnd += x[i + p] * x[i + p];
        }
        nac[p] = ac / sqrt(sumSqBeg * sumSqEnd);
    }

    //  ---------------------------------------
    //  Find the highest peak in the range of interest.

    //  Get the highest value
    int bestP = minP;
    for (int p = minP; p <= maxP; p++)
        if (nac[p] > nac[bestP])
            bestP = p;

    //  Give up if it's highest value, but not actually a peak.
    //  This can happen if the period is outside the range [minP, maxP]
    if (nac[bestP] < nac[bestP - 1]
        && nac[bestP] < nac[bestP + 1])
    {
        return 0.0;
    }

    //  "Quality" of periodicity is the normalized autocorrelation
    //  at the best period (which may be a multiple of the actual
    //  period).
    q = nac[bestP];


    //  --------------------------------------
    //  Interpolate based on neighboring values
    //  E.g. if value to right is bigger than value to the left,
    //  real peak is a bit to the right of discretized peak.
    //  if left  == right, real peak = mid;
    //  if left  == mid,   real peak = mid-0.5
    //  if right == mid,   real peak = mid+0.5

    double mid = nac[bestP];
    double left = nac[bestP - 1];
    double right = nac[bestP + 1];

    //assert(2 * mid - left - right > 0.0);

    double shift = 0.5*(right - left) / (2 * mid - left - right);

    double pEst = bestP + shift;

    //  -----------------------------------------------
    //  If the range of pitches being searched is greater
    //  than one octave, the basic algo above may make "octave"
    //  errors, in which the period identified is actually some
    //  integer multiple of the real period.  (Makes sense, as
    //  a signal that's periodic with period p is technically
    //  also period with period 2p).
    //
    //  Algorithm is pretty simple: we hypothesize that the real
    //  period is some "submultiple" of the "bestP" above.  To
    //  check it, we see whether the NAC is strong at each of the
    //  hypothetical subpeak positions.  E.g. if we think the real
    //  period is at 1/3 our initial estimate, we check whether the 
    //  NAC is strong at 1/3 and 2/3 of the original period estimate.

    const double k_subMulThreshold = 0.90;  //  If strength at all submultiple of peak pos are 
                                            //  this strong relative to the peak, assume the 
                                            //  submultiple is the real period.

                                            //  For each possible multiple error (starting with the biggest)
    int maxMul = bestP / minP;
    bool found = false;
    for (int mul = maxMul; !found && mul >= 1; mul--)
    {
        //  Check whether all "submultiples" of original
        //  peak are nearly as strong.
        bool subsAllStrong = true;

        //  For each submultiple
        for (int k = 1; k < mul; k++)
        {
            int subMulP = int(k*pEst / mul + 0.5);
            //  If it's not strong relative to the peak NAC, then
            //  not all submultiples are strong, so we haven't found
            //  the correct submultiple.
            if (nac[subMulP] < k_subMulThreshold * nac[bestP])
                subsAllStrong = false;

            //  TODO: Use spline interpolation to get better estimates of nac
            //  magnitudes for non-integer periods in the above comparison
        }

        //  If yes, then we're done.   New estimate of 
        //  period is "submultiple" of original period.
        if (subsAllStrong == true)
        {
            found = true;
            pEst = pEst / mul;
        }
    }

    return pEst;
}