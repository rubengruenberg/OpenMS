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
// $Maintainer: Timo Sachsenberg$
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/METADATA/SpectrumSettings.h>
#include <OpenMS/VISUAL/LayerData.h>

namespace OpenMS
{
  class TOPPViewBase;

  /**
  @brief Base behavior for different visualizaton modules in TOPPView.
  */
  class TVControllerBase
    : public QObject
  {
    Q_OBJECT

protected:
    ///@name Type definitions
    //@{
    /// Feature map type
    typedef FeatureLayer::FeatureMapType FeatureMapType;
    /// Feature map managed type
    typedef FeatureLayer::FeatureMapSharedPtrType FeatureMapSharedPtrType;

    /// Consensus feature map type
    typedef ConsensusLayer::ConsensusMapType ConsensusMapType;
    /// Consensus  map managed type
    typedef ConsensusLayer::ConsensusMapSharedPtrType ConsensusMapSharedPtrType;

    /// Peak map type
    typedef PeakLayer::ExperimentType ExperimentType;
    /// Main managed data type (experiment)
    typedef PeakLayer::ExperimentSharedPtrType ExperimentSharedPtrType;
    /// Peak spectrum type
    typedef ExperimentType::SpectrumType SpectrumType;
    //@}

public:
    TVControllerBase() = delete;

    virtual ~TVControllerBase() = default;
public slots:
    /// Slot for behavior activation. The default behaviour does nothing. Override in child class if desired.
    virtual void activateBehavior();

    /// Slot for behavior deactivation. The default behaviour does nothing. Override in child class if desired.
    virtual void deactivateBehavior();
protected:
    /// Construct the behaviour
    TVControllerBase(TOPPViewBase* parent);

    TOPPViewBase* tv_;
  };
}