// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2013.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest, Erhan Kenar$ 
// --------------------------------------------------------------------------

#include <OpenMS/TRANSFORMATIONS/RAW2PEAK/PeakPickerMaxima.h>

#include <OpenMS/FILTERING/NOISEESTIMATION/SignalToNoiseEstimatorMedianRapid.h>
//#include <OpenMS/MATH/MISC/Spline2d.h>
#include <Wm5IntpAkimaNonuniform1.h>


#include <cmath>
#include <limits>

namespace OpenMS
{
  PeakPickerMaxima::PeakPickerMaxima(double signal_to_noise, double spacing_difference, double sn_window_length) :
    signal_to_noise_(signal_to_noise),
    spacing_difference_(spacing_difference),
    sn_window_length_(sn_window_length)
    {}

  void PeakPickerMaxima::findMaxima(const std::vector<double>& mz_array, const std::vector<double>& int_array, std::vector<PeakCandidate>& pc)
  {
    if (mz_array.size() < 5) return;

    SignalToNoiseEstimatorMedianRapid::NoiseEstimator noise_estimator(0,0,0);
    if (signal_to_noise_ > 0.0)
    {
      SignalToNoiseEstimatorMedianRapid rapid_sne(sn_window_length_);
      noise_estimator = rapid_sne.estimateNoise(mz_array, int_array);
    }

    // find local maxima in raw data
    for (size_t i = 2; i < mz_array.size() - 2; ++i)
    {
      double central_peak_mz = mz_array[i], central_peak_int = int_array[i];
      double left_neighbor_mz = mz_array[i - 1], left_neighbor_int = int_array[i - 1];
      double right_neighbor_mz = mz_array[i + 1], right_neighbor_int = int_array[i + 1];

      //do not interpolate when the left or right support is a zero-data-point
      if(std::fabs(left_neighbor_int) < std::numeric_limits<double>::epsilon() )
        continue;
      if(std::fabs(right_neighbor_int) < std::numeric_limits<double>::epsilon() )
        continue;

      // MZ spacing sanity checks
      double left_to_central = std::fabs(central_peak_mz - left_neighbor_mz);
      double central_to_right = std::fabs(right_neighbor_mz - central_peak_mz);
      double min_spacing = (left_to_central < central_to_right) ? left_to_central : central_to_right;

      double act_snt = 0.0, act_snt_l1 = 0.0, act_snt_r1 = 0.0;

      if (signal_to_noise_ > 0.0)
      {
        act_snt = central_peak_int / noise_estimator.get_noise_value(central_peak_mz);
        act_snt_l1 = left_neighbor_int / noise_estimator.get_noise_value(left_neighbor_mz);
        act_snt_r1 = right_neighbor_int / noise_estimator.get_noise_value(right_neighbor_mz);
      }

      // look for peak cores meeting MZ and intensity/SNT criteria
      if (act_snt >= signal_to_noise_
         && left_to_central < spacing_difference_ * min_spacing
         && central_peak_int > left_neighbor_int
         && act_snt_l1 >= signal_to_noise_
         && central_to_right < spacing_difference_ * min_spacing
         && central_peak_int > right_neighbor_int
         && act_snt_r1 >= signal_to_noise_)
      {
        // special case: if a peak core is surrounded by more intense
        // satellite peaks (indicates oscillation rather than
        // real peaks) -> remove

        double act_snt_l2 = 0.0, act_snt_r2 = 0.0;
        PeakCandidate candidate;
        candidate.pos = i;
        candidate.mz_max = -1;
        candidate.int_max = -1;

        if (signal_to_noise_ > 0.0)
        {
          act_snt_l2 = int_array[i-2] / noise_estimator.get_noise_value(mz_array[i-2]);
          act_snt_r2 = int_array[i+2] / noise_estimator.get_noise_value(mz_array[i+2]);
        }

        if ((i > 1
            && std::fabs(left_neighbor_mz - mz_array[i - 2]) < spacing_difference_ * min_spacing
            && left_neighbor_int < int_array[i - 2]
            && act_snt_l2 >= signal_to_noise_)
           &&
            ((i + 2) < mz_array.size()
            && std::fabs(mz_array[i + 2] - right_neighbor_mz) < spacing_difference_ * min_spacing
            && right_neighbor_int < int_array[i + 2]
            && act_snt_r2 >= signal_to_noise_)
            )
        {
          ++i;
          continue;
        }

        double boundary_mz, boundary_int;

        // peak core found, now extend it
        // to the left
        size_t k = 2;

        size_t missing_left(0);
        size_t missing_right(0);

        boundary_mz = left_neighbor_mz;
        boundary_int = left_neighbor_int;

        while ( k <= i // prevent underflow
              && (i - k + 1) > 0
              && (missing_left < 2)
              && int_array[i - k] <= boundary_int)
        {

          double act_snt_lk = 0.0;
          if (signal_to_noise_ > 0.0)
          {
            act_snt_lk = int_array[i-k] / noise_estimator.get_noise_value(mz_array[i-k]);
          }

          if (act_snt_lk >= signal_to_noise_ && std::fabs(mz_array[i - k] - boundary_mz) < spacing_difference_ * min_spacing)
          {
            boundary_mz = mz_array[i - k];
            boundary_int = int_array[i - k];
          }
          else
          {
            boundary_mz = mz_array[i - k];
            boundary_int = int_array[i - k];
            ++missing_left;
          }
          ++k;
        }
        candidate.left_boundary = i - k + 1;

        // to the right
        k = 2;
        boundary_mz = right_neighbor_mz;
        boundary_int = right_neighbor_int;
        while ((i + k) < mz_array.size()
              && (missing_right < 2)
              && int_array[i + k] <= boundary_int)
        {
          double act_snt_rk = 0.0;
          if (signal_to_noise_ > 0.0)
          {
            act_snt_rk = int_array[i+k] / noise_estimator.get_noise_value(mz_array[i+k]);
          }

          if (act_snt_rk >= signal_to_noise_ && std::fabs(mz_array[i + k] - boundary_mz) < spacing_difference_ * min_spacing)
          {
            boundary_mz = mz_array[i + k];
            boundary_int = int_array[i + k];
          }
          else
          {
            boundary_mz = mz_array[i + k];
            boundary_int = int_array[i + k];
            ++missing_right;
          }
          ++k;
        }
        candidate.right_boundary = i + k - 1;
        // jump over raw data points that have been considered already
        i = i + k - 1;
        pc.push_back(candidate);
      }
    }
  }

  void PeakPickerMaxima::pick(std::vector<double>& mz_array, std::vector<double>& int_array, std::vector<PeakCandidate>& pc)
  {
    if (mz_array.size() < 5) return;

    findMaxima(mz_array, int_array, pc);

    // Go through all peak candidates and find accurate mz / int values based on the spline interpolation
    for (size_t j = 0; j < pc.size(); ++j)
    {
        PeakCandidate candidate = pc[j];

        // output all raw data points selected for one peak
        // TODO: #ifdef DEBUG_ ...
        // for (std::map<double, double>::const_iterator map_it = peak_raw_data.begin(); map_it != peak_raw_data.end(); ++map_it) {
        // PeakType peak;
        // peak.setMZ(map_it->first);
        // peak.setIntensity(map_it->second);
        // output.push_back(peak);
        // std::cout << map_it->first << " " << map_it->second << " snt: " << std::endl;
        // }
        // std::cout << "--------------------" << std::endl;

        double central_peak_mz = mz_array[candidate.pos], central_peak_int = int_array[candidate.pos];
        double left_neighbor_mz = mz_array[candidate.pos - 1]; //, left_neighbor_int = int_array[candidate.pos - 1];
        double right_neighbor_mz = mz_array[candidate.pos + 1]; //, right_neighbor_int = int_array[candidate.pos + 1];


        std::vector<double> raw_mz_values;
        std::vector<double> raw_int_values;

        raw_mz_values.reserve(candidate.right_boundary - candidate.left_boundary);
        raw_int_values.reserve(candidate.right_boundary - candidate.left_boundary);

        raw_mz_values.insert(raw_mz_values.begin(), mz_array.begin() + candidate.left_boundary, mz_array.begin() + candidate.right_boundary + 1);
        raw_int_values.insert(raw_int_values.begin(), int_array.begin() + candidate.left_boundary, int_array.begin() + candidate.right_boundary + 1);

        // skip if the minimal number of 3 points for fitting is not reached
        if(raw_mz_values.size() < 4)
          continue;

        //Spline2d<double> peak_spline (3, raw_mz_values, raw_int_values);
        Wm5::IntpAkimaNonuniform1<double> interpolator (raw_mz_values.size(), &raw_mz_values[0], &raw_int_values[0]);

        // calculate maximum by evaluating the spline's 1st derivative
        // (bisection method)
        double max_peak_mz = central_peak_mz;
        double max_peak_int = central_peak_int;
        double threshold = 0.000001;
        double lefthand = left_neighbor_mz;
        double righthand = right_neighbor_mz;

        bool lefthand_sign = 1;
        double eps = std::numeric_limits<double>::epsilon();

        // bisection
        do
        {
          double mid = (lefthand + righthand) / 2;

          //double midpoint_deriv_val = peak_spline.derivatives(mid, 1);
          double midpoint_deriv_val = interpolator(1,mid);

          // if deriv nearly zero then maximum already found
          if (!(std::fabs(midpoint_deriv_val) > eps))
          {
            break;
          }

          bool midpoint_sign = (midpoint_deriv_val < 0.0) ? 0 : 1;

          if (lefthand_sign ^ midpoint_sign)
          {
            righthand = mid;
          }
          else
          {
            lefthand = mid;
          }
        }
        while (std::fabs(lefthand - righthand) > threshold);

        // sanity check?
        max_peak_mz = (lefthand + righthand) / 2;
        //max_peak_int = peak_spline.eval( max_peak_mz );
        max_peak_int = interpolator(0, max_peak_mz );

        // save picked pick into output spectrum
        pc[j].mz_max = max_peak_mz;
        pc[j].int_max = max_peak_int;
    }
  }

}
