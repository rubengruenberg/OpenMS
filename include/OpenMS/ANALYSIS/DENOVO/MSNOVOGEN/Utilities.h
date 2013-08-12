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
// $Maintainer: Jens Allmer $
// $Authors: Jens Allmer $
// --------------------------------------------------------------------------

#ifndef OPENMS__ANALYSIS_DENOVO_MSNOVOGEN_UTILITIES_H
#define OPENMS__ANALYSIS_DENOVO_MSNOVOGEN_UTILITIES_H

#include <OpenMS/config.h>
#include <OpenMS/CHEMISTRY/Residue.h>
#include <OpenMS/CHEMISTRY/AASequence.h>
#include <vector>
#include <OpenMS/KERNEL/MSSpectrum.h>
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>

using boost::random::variate_generator;
using boost::random::mt19937;
using boost::random::normal_distribution;

namespace OpenMS
{
  /**
  * @brief The Utilities are a collection of methods which are 
  * used in several classes of the MSNovoGen algorithm.
  * They could in the future become part of other classes within OpenMS.
  */
  class OPENMS_DLLAPI Utilities
  {
private:
	/// random number generator.
	mutable boost::mt19937 rng;

	/// Copy c'tor shouldn't be used.
	Utilities(const Utilities& other);

	/// Assignment operator shouldn't be used.
	Utilities & operator=(const Utilities& rhs);

  public:
    /// Default c'tor
    Utilities();

    /// provide the seed for the random number generator
    Utilities(const Size seed);

	/// Generates a random sequence of the desired length and then adjusts the
	/// amino acids until the weight matches the desired criteria.
	/// srand must have been called before.
	const AASequence getRandomSequence(const int len, const double weight, const double tolerance, const std::vector<const Residue *> aaList) const;

    /// from the list of available amino acids selects a random one.
	/// srand must have been called before.
    const String getRandomAA(const std::vector<const Residue *> aaList) const;

    /// The random string may not fit to the expected precursor mass and is then adjusted to fit (if possible with a maximum of 3 changes).
    /// The passed in sequence is directly modified.
	/// srand must have been called before.
    bool adjustToFitMass(AASequence & sequence, const double weight, const double tolerance, const std::vector<const Residue *> aaList) const;

	/// Uses the Seqan Needleman-Wunsch implementation to calculated the edit distance
	/// of the two amino acid sequences.
    Size editDistance(const AASequence & lhs, const AASequence & rhs) const;

	/// Iterates over the provided spectrum and returns the sum of all intensities.
    double getSummedIntensity(const MSSpectrum<> * ms) const;

    /// to change the seed or to fix it for unit tests.
    void seed(const Size seed);

  };
} // namespace

#endif // OPENMS__ANALYSIS_DENOVO_MSNOVOGEN_UTILITIES_H
