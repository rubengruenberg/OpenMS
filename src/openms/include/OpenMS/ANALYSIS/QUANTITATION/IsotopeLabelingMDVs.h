// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2020.
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
// $Maintainer: Ahmed Khalil $
// $Authors: Ahmed Khalil $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/config.h>

//Kernal classes
// #include <OpenMS/DATASTRUCTURES/DefaultParamHandler.h>
// #include <OpenMS/KERNEL/StandardTypes.h>
// #include <OpenMS/KERNEL/MSExperiment.h>
// #include <OpenMS/KERNEL/MSChromatogram.h>
#include <OpenMS/KERNEL/FeatureMap.h>
#include <OpenMS/KERNEL/Feature.h>
// #include <OpenMS/KERNEL/MRMTransitionGroup.h>
// #include <OpenMS/KERNEL/MRMFeature.h>

//Analysis classes
// #include <OpenMS/ANALYSIS/MAPMATCHING/TransformationDescription.h>
// #include <OpenMS/ANALYSIS/MAPMATCHING/TransformationModel.h>
// #include <OpenMS/ANALYSIS/TARGETED/TargetedExperiment.h>

//Quantitation classes
// #include <OpenMS/METADATA/AbsoluteQuantitationStandards.h>
// #include <OpenMS/ANALYSIS/QUANTITATION/AbsoluteQuantitationMethod.h>


//Standard library
 #include <cstddef> // for size_t & ptrdiff_t
 #include <vector>
 #include <string>
 #include <cmath>
 #include <numeric>
// #include <boost/math/special_functions/erf.hpp>
 #include <algorithm>

namespace OpenMS
{

  /**
    @brief IsotopeLabelingMDVs is a class to support and analyze isotopic labeling experiments
            (i.e. MDVs)

    Method: TODO
    
    Terms: TODO
    
  */
  class OPENMS_DLLAPI IsotopeLabelingMDVs 
  {

public:    
    //@{
    /// Constructor
    IsotopeLabelingMDVs();

    /// Destructor
    ~IsotopeLabelingMDVs();
    //@}
 
    /**
      @brief This function performs an isotopic correction to account for unlabeled abundances coming from
      the derivatization agent (e.g., TBMS, see references for the formulas)
     
      @param[in]  normalized_featuremap FeatureMap with normalized values for each component and unlabeled chemical formula for each component group
      @param[out] corrected_featuremap FeatureMap with corrected values for each component
    */
    void isotopicCorrection(FeatureMap& normalized_featuremap, FeatureMap& corrected_featuremap);

   /**
     @brief This function calculates the isotopic purity of the MDV (see Long et al 2019 for the formula)

     @param[in]  normalized_featuremap FeatureMap with normalized values for each component and the number of heavy labeled e.g., carbons
     @param[out] featuremap_with_isotopic_purity  FeatureMap with the calculated isotopic purity for the component group
   */
    void calculateIsotopicPurity(FeatureMap& normalized_featuremap, FeatureMap& featuremap_with_isotopic_purity);

  // 4
  /**
    @brief calculate the accuracy of the MDV as compared to the theoretical MDV (only for 12C quality control experiments)
   
    @param[in]  normalized_featuremap FeatureMap with normalized values for each component and the chemical formula of the component group
    @param[out] featuremap_with_accuracy_info  FeatureMap with the component group accuracy and accuracy for the error for each component
  */
    void calculateMDVAccuracy(FeatureMap& normalized_featuremap, FeatureMap& featuremap_with_accuracy_info);
 
    /**
    @brief This function calculates the mass distribution vector (MDV)
    either normalized to the highest mass intensity (norm_max) or normalized
    to the sum of all mass intensities (norm_sum)
     
    @param[in]   mass_intensity_type       Mass intensity type (either norm_max or norm_sum)
    @param[in]   feature_name                      Feature name to determine which features are needed to apply calculations on
    @param[in]   measured_featuremap      FeatureMap with measured intensity for each component
    @param[out]  normalized_featuremap  FeatureMap with normalized values for each component
    */
    void calculateMDV(
      Feature& measured_feature, Feature& normalized_feature,
      const String& mass_intensity_type, const String& feature_name);
     
  };

}
