// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2018.
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
// $Maintainer: Douglas McCloskey, Pasquale Domenico Colaianni, Svetlana Kutuzova $
// $Authors: Douglas McCloskey, Pasquale Domenico Colaianni, Svetlana Kutuzova $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/OPENSWATH/MRMFeatureSelector.h>
#include <OpenMS/DATASTRUCTURES/LPWrapper.h>
#include <OpenMS/KERNEL/FeatureMap.h>
#include <algorithm>
#include <set>

namespace OpenMS
{
  Int MRMFeatureSelector::addVariable_(
    LPWrapper& problem,
    const String& name,
    const bool bounded,
    const double obj
  ) const
  {
    const Int index = problem.addColumn();

    if (bounded)
    {
      problem.setColumnBounds(index, 0, 1, LPWrapper::DOUBLE_BOUNDED);
    }
    else
    {
      problem.setColumnBounds(index, 0, 1, LPWrapper::UNBOUNDED);
    }

    problem.setColumnName(index, name);

    if (getVariableType() == VariableType::INTEGER)
    {
      problem.setColumnType(index, LPWrapper::INTEGER);
    }
    else if (getVariableType() == VariableType::CONTINUOUS)
    {
      problem.setColumnType(index, LPWrapper::CONTINUOUS);
    }
    else
    {
      throw "Variable type not supported\n";
    }

    problem.setObjective(index, obj);
    return index;
  }

  void MRMFeatureSelector::addConstraint_(
    LPWrapper& problem,
    std::vector<Int> indices,
    std::vector<double> values,
    const String& name,
    const double lb,
    const double ub,
    const LPWrapper::Type param
  ) const
  {
    problem.addRow(indices, values, name, lb, ub, param);
  }

  void MRMFeatureSelectorScore::optimize(
    const std::vector<std::pair<double, String>>& time_to_name,
    const std::map< String, std::vector<Feature> >& feature_name_map,
    std::vector<String>& result
  )
  {
    result.clear();
    std::set<String> variables;
    LPWrapper problem;
    problem.setObjectiveSense(LPWrapper::MIN);
    for (const std::pair<double, String>& elem : time_to_name)
    {
      std::vector<Int> constraints;
      for (const Feature& feature : feature_name_map.at(elem.second))
      {
        const String name1 = elem.second + "_" + String(feature.getUniqueId());
        if (variables.count(name1) == 0)
        {
            constraints.push_back(addVariable_(problem, name1, true, computeScore_(feature)));
            variables.insert(name1);
        }
      }
      std::vector<double> constraints_values(constraints.size(), 1.0);
      addConstraint_(problem, constraints, constraints_values, elem.second + "_constraint", 1.0, 1.0, LPWrapper::DOUBLE_BOUNDED);
    }
    LPWrapper::SolverParam param;
    problem.solve(param);
    for (Int c = 0; c < problem.getNumberOfColumns(); ++c)
    {
      if (problem.getColumnValue(c) >= getOptimalThreshold())
      {
        result.push_back(problem.getColumnName(c));
      }
    }
  }

  String MRMFeatureSelector::removeSpaces_(String str) const
  {
    String::iterator end_pos = std::remove(str.begin(), str.end(), ' ');
    str.erase(end_pos, str.end());
    return str;
  }

  void MRMFeatureSelectorQMIP::optimize(
    const std::vector<std::pair<double, String>>& time_to_name,
    const std::map<String, std::vector<Feature>>& feature_name_map,
    std::vector<String>& result
  )
  {
    result.clear();
    std::set<String> variables;
    LPWrapper problem;
    // problem.setSolver(LPWrapper::SOLVER_GLPK); // glpk
    problem.setObjectiveSense(LPWrapper::MIN);
    Size n_constraints = 0;
    Size n_variables = 0;
    for (Int cnt1 = 0; static_cast<Size>(cnt1) < time_to_name.size(); ++cnt1)
    {
      const Size start_iter = std::max(cnt1 - getNNThreshold(), 0);
      const Size stop_iter = std::min(static_cast<Size>(cnt1 + getNNThreshold() + 1), time_to_name.size()); // assuming nn_threshold_ >= -1
      std::vector<Int> constraints;
      const std::vector<Feature> feature_row1 = feature_name_map.at(time_to_name[cnt1].second);

      for (Size i = 0; i < feature_row1.size(); ++i)
      {
        const String name1 = time_to_name[cnt1].second + "_" + String(feature_row1[i].getUniqueId());

        if (variables.count(name1) == 0)
        {
          constraints.push_back(addVariable_(problem, name1, true, 0));
          variables.insert(name1);
          ++n_variables;
        }
        else
        {
          constraints.push_back(problem.getColumnIndex(name1));
        }

        double score_1 = computeScore_(feature_row1[i]);
        const Size n_score_weights = getScoreWeights().size();

        if (n_score_weights > 1)
        {
          score_1 = std::pow(score_1, 1.0 / n_score_weights);
        }

        const Int index1 = problem.getColumnIndex(name1);

        for (Size cnt2 = start_iter; cnt2 < stop_iter; ++cnt2)
        {
          if (static_cast<Size>(cnt1) == cnt2)
          {
            continue;
          }

          const std::vector<Feature> feature_row2 = feature_name_map.at(time_to_name[cnt2].second);
          const double locality_weight = getLocalityWeight()
            ? 1.0 / (getNNThreshold() - std::abs(static_cast<Int>(start_iter + cnt2) - cnt1) + 1)
            : 1.0;
          const double tr_delta_expected = time_to_name[cnt1].first - time_to_name[cnt2].first;

          for (Size j = 0; j < feature_row2.size(); ++j)
          {
            const String name2 = time_to_name[cnt2].second + "_" + String(feature_row2[j].getUniqueId());
            if (variables.count(name2) == 0)
            {
              addVariable_(problem, name2, true, 0);
              variables.insert(name2);
              ++n_variables;
            }

            const String var_qp_name = time_to_name[cnt1].second + "_" + String(i) + "-" + time_to_name[cnt2].second + "_" + String(j);

            // The two following problem's variables need the variable type to be "continuous"
            // Save current variable type to later set it back to the same value
            const VariableType prev_variable_type = getVariableType();
            setVariableType(VariableType::CONTINUOUS);
            const Int index_var_qp = addVariable_(problem, var_qp_name, true, 0);
            const Int index_var_abs = addVariable_(problem, var_qp_name + "-ABS", false, 1);
            setVariableType(prev_variable_type);

            const Int index2 = problem.getColumnIndex(name2);

            double score_2 = computeScore_(feature_row2[j]);
            if (n_score_weights > 1)
            {
              score_2 = std::pow(score_2, 1.0 / n_score_weights);
            }

            const double tr_delta = feature_row1[i].getRT() - feature_row2[j].getRT();
            const double score = locality_weight * score_1 * score_2 * (tr_delta - tr_delta_expected);

            addConstraint_(problem, {index1, index_var_qp}, {1.0, -1.0}, var_qp_name + "-QP1", 0.0, 1.0, LPWrapper::LOWER_BOUND_ONLY);
            addConstraint_(problem, {index2, index_var_qp}, {1.0, -1.0}, var_qp_name + "-QP2", 0.0, 1.0, LPWrapper::LOWER_BOUND_ONLY);
            addConstraint_(problem, {index1, index2, index_var_qp}, {1.0, 1.0, -1.0}, var_qp_name + "-QP3", 0.0, 1.0, LPWrapper::UPPER_BOUND_ONLY);
            std::vector<Int> indices_abs = {index_var_abs, index_var_qp};
            addConstraint_(problem, indices_abs, {-1.0, score}, var_qp_name + "-obj+", -1.0, 0.0, LPWrapper::UPPER_BOUND_ONLY);
            addConstraint_(problem, indices_abs, {-1.0, -score}, var_qp_name + "-obj-", -1.0, 0.0, LPWrapper::UPPER_BOUND_ONLY);

            n_constraints += 5;
            n_variables += 2;
          }
        }
      }
      std::vector<double> constraints_values(constraints.size(), 1.0);
      addConstraint_(problem, constraints, constraints_values, time_to_name[cnt1].second + "_constraint", 1.0, 1.0, LPWrapper::DOUBLE_BOUNDED);
      // addConstraint_(problem, constraints, constraints_values, time_to_name[cnt1].second + "_constraint", 1.0, 1.0, LPWrapper::FIXED); // glpk
      ++n_constraints;
    }
    LPWrapper::SolverParam param;
    problem.solve(param);
    const double optimal_threshold = getOptimalThreshold();
    for (Int c = 0; c < problem.getNumberOfColumns(); ++c)
    {
      const String name = problem.getColumnName(c);
      if (problem.getColumnValue(c) > optimal_threshold - 1e-17 && variables.count(name))
      {
        result.push_back(name);
      }
    }
  }

  void MRMFeatureSelector::constructTargTransList_(
    const FeatureMap& features,
    std::vector<std::pair<double, String>>& time_to_name,
    std::map<String, std::vector<Feature>>& feature_name_map
  ) const
  {
    time_to_name.clear();
    feature_name_map.clear();
    std::set<String> names;
    for (const Feature& feature : features)
    {
      const String component_group_name = removeSpaces_(feature.getMetaValue("PeptideRef").toString());
      const double assay_retention_time = feature.getMetaValue("assay_rt");
      if (names.count(component_group_name) == 0)
      {
        time_to_name.push_back(std::make_pair(assay_retention_time, component_group_name));
        names.insert(component_group_name);
      }
      if (feature_name_map.count(component_group_name) == 0)
      {
        feature_name_map[component_group_name] = std::vector<Feature>();
      }
      feature_name_map[component_group_name].push_back(feature);
      if (getSelectTransitionGroup())
      {
        continue;
      }
      for (const Feature& subordinate : feature.getSubordinates())
      {
        const String component_name = removeSpaces_(subordinate.getMetaValue("native_id").toString());
        if (names.count(component_name))
        {
          time_to_name.push_back(std::make_pair(assay_retention_time, component_name));
          names.insert(component_name);
        }
        if (feature_name_map.count(component_name) == 0)
        {
          feature_name_map[component_name] = std::vector<Feature>();
        }
        feature_name_map[component_name].push_back(subordinate);
      }
    }
  }

  void MRMFeatureSelector::selectMRMFeature(const FeatureMap& features, FeatureMap& selected_filtered)
  {
    selected_filtered.clear();

    std::vector<std::pair<double, String>> time_to_name;
    std::map<String, std::vector<Feature>> feature_name_map;
    constructTargTransList_(features, time_to_name, feature_name_map);

    sort(time_to_name.begin(), time_to_name.end());
    Int window_length = getSegmentWindowLength();
    Int step_length = getSegmentStepLength();
    if (window_length == -1 && step_length == -1)
    {
      window_length = step_length = time_to_name.size();
    }
    Size n_segments = time_to_name.size() / step_length;
    if (time_to_name.size() % step_length)
    {
      ++n_segments;
    }
    std::vector<String> result_names;

    for (Size i = 0; i < n_segments; ++i)
    {
      const Size start = step_length * i;
      const Size end = std::min(start + window_length, time_to_name.size());
      const std::vector<std::pair<double, String>> time_slice(time_to_name.begin() + start, time_to_name.begin() + end);
      std::vector<String> result;
      optimize(time_slice, feature_name_map, result);
      result_names.insert(result_names.end(), result.begin(), result.end());
    }
    const std::set<String> result_names_set(result_names.begin(), result_names.end());
    for (const Feature& feature : features)
    {
      std::vector<Feature> subordinates_filtered;
      for (const Feature& subordinate : feature.getSubordinates())
      {
        const String feature_name = getSelectTransitionGroup()
          ? removeSpaces_(feature.getMetaValue("PeptideRef").toString()) + "_" + String(feature.getUniqueId())
          : removeSpaces_(subordinate.getMetaValue("native_id").toString()) + "_" + String(feature.getUniqueId());

        if (result_names_set.count(feature_name))
        {
          subordinates_filtered.push_back(subordinate);
        }
      }
      if (subordinates_filtered.size())
      {
        Feature feature_filtered(feature);
        feature_filtered.setSubordinates(subordinates_filtered);
        selected_filtered.push_back(feature_filtered);
      }
    }
  }

  double MRMFeatureSelector::computeScore_(const Feature& feature) const
  {
    double score_1 = 1.0;
    for (const std::pair<String, LambdaScore>& score_weight : score_weights_)
    {
      const String& metavalue_name = score_weight.first;
      const LambdaScore lambda_score = score_weight.second;
      if (!feature.metaValueExists(metavalue_name))
      {
        LOG_WARN << "computeScore_(): Metavalue \"" << metavalue_name << "\" not found.";
        continue;
      }
      const double value = weightScore_(feature.getMetaValue(metavalue_name), lambda_score);
      if (value > 0.0)
      {
        score_1 *= value;
      }
    }
    return score_1;
  }

  double MRMFeatureSelector::weightScore_(const double score, const LambdaScore lambda_score) const
  {
    if (lambda_score == LambdaScore::LINEAR)
    {
      return score;
    }
    else if (lambda_score == LambdaScore::INVERSE)
    {
      return 1.0 / score;
    }
    else if (lambda_score == LambdaScore::LOG)
    {
      return std::log(score);
    }
    else if (lambda_score == LambdaScore::INVERSE_LOG)
    {
      return 1.0 / std::log(score);
    }
    else if (lambda_score == LambdaScore::INVERSE_LOG10)
    {
      return 1.0 / std::log10(score);
    }
    else
    {
      throw Exception::IllegalArgument(__FILE__, __LINE__, __FUNCTION__,
        "`lambda_score`'s value is not handled by any current condition.");
    }
  }

  void MRMFeatureSelector::setNNThreshold(const Int nn_threshold)
  {
    nn_threshold_ = nn_threshold;
  }

  Int MRMFeatureSelector::getNNThreshold() const
  {
    return nn_threshold_;
  }

  void MRMFeatureSelector::setLocalityWeight(const bool locality_weight)
  {
    locality_weight_ = locality_weight;
  }

  bool MRMFeatureSelector::getLocalityWeight() const
  {
    return locality_weight_;
  }

  void MRMFeatureSelector::setSelectTransitionGroup(const bool select_transition_group)
  {
    select_transition_group_ = select_transition_group;
  }

  bool MRMFeatureSelector::getSelectTransitionGroup() const
  {
    return select_transition_group_;
  }

  void MRMFeatureSelector::setSegmentWindowLength(const Int segment_window_length)
  {
    segment_window_length_ = segment_window_length;
  }

  Int MRMFeatureSelector::getSegmentWindowLength() const
  {
    return segment_window_length_;
  }

  void MRMFeatureSelector::setSegmentStepLength(const Int segment_step_length)
  {
    segment_step_length_ = segment_step_length;
  }

  Int MRMFeatureSelector::getSegmentStepLength() const
  {
    return segment_step_length_;
  }

  void MRMFeatureSelector::setVariableType(const VariableType variable_type)
  {
    variable_type_ = variable_type;
  }

  MRMFeatureSelector::VariableType MRMFeatureSelector::getVariableType() const
  {
    return variable_type_;
  }

  void MRMFeatureSelector::setOptimalThreshold(const double optimal_threshold)
  {
    optimal_threshold_ = optimal_threshold;
  }

  double MRMFeatureSelector::getOptimalThreshold() const
  {
    return optimal_threshold_;
  }

  void MRMFeatureSelector::setScoreWeights(const std::map<String, LambdaScore>& score_weights)
  {
    score_weights_ = score_weights;
  }

  std::map<String, MRMFeatureSelector::LambdaScore> MRMFeatureSelector::getScoreWeights() const
  {
    return score_weights_;
  }
}
