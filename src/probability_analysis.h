/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file probability_analysis.h
/// Contains functionality to do numerical analysis
/// of probabilities and importance factors.

#ifndef SCRAM_SRC_PROBABILITY_ANALYSIS_H_
#define SCRAM_SRC_PROBABILITY_ANALYSIS_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/container/flat_set.hpp>

#include "bdd.h"
#include "event.h"
#include "settings.h"

namespace scram {

/// @struct ImportanceFactors
/// Collection of importance factors for variables.
struct ImportanceFactors {
  double dif;  ///< Fussel-Vesely diagnosis importance factor.
  double mif;  ///< Birnbaum marginal importance factor.
  double cif;  ///< Critical importance factor.
  double rrw;  ///< Risk reduction worth factor.
  double raw;  ///< Risk achievement worth factor.
};

/// @class ProbabilityAnalysis
/// Main quantitative analysis.
class ProbabilityAnalysis {
 public:
  using BasicEventPtr = std::shared_ptr<BasicEvent>;
  using GatePtr = std::shared_ptr<Gate>;
  using VertexPtr = std::shared_ptr<Vertex>;
  using ItePtr = std::shared_ptr<Ite>;

  /// Probability analysis
  /// on the fault tree represented by the root gate
  /// with Binary decision diagrams.
  ///
  /// @param[in] root The top event of the fault tree.
  /// @param[in] settings Analysis settings for probability calculations.
  ///
  /// @note This technique does not require cut sets.
  ProbabilityAnalysis(const GatePtr& root, const Settings& settings);

  virtual ~ProbabilityAnalysis() {}

  /// Sets the databases of basic events with probabilities.
  /// Resets the main basic event database
  /// and clears the previous information.
  /// This information is the main source
  /// for calculations and internal indexes for basic events.
  ///
  /// @param[in] basic_events The database of basic events in cut sets.
  ///
  /// @note  If not enough information is provided,
  ///        the analysis behavior is undefined.
  void UpdateDatabase(
      const std::unordered_map<std::string, BasicEventPtr>& basic_events);

  /// Performs quantitative analysis on minimal cut sets
  /// containing basic events provided in the databases.
  /// It is assumed that the analysis is called only once.
  ///
  /// @param[in] min_cut_sets Minimal cut sets with string ids of events.
  ///                         Negative event is indicated by "'not' + id"
  ///
  /// @note  Undefined behavior if analysis called two or more times.
  virtual void Analyze(
      const std::set< std::set<std::string> >& min_cut_sets) noexcept;

  /// @returns The total probability calculated by the analysis.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline double p_total() const { return p_total_; }

  /// @returns Map with minimal cut sets and their probabilities.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline const std::map< std::set<std::string>, double >&
      prob_of_min_sets() const {
    return prob_of_min_sets_;
  }

  /// @returns Map with basic events and their importance factors.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline const std::unordered_map<std::string, ImportanceFactors>&
      importance() const {
    return importance_;
  }

  /// @returns Warnings generated upon analysis.
  inline const std::string warnings() const { return warnings_; }

  /// @returns The container of basic events of supplied for the analysis.
  inline const std::unordered_map<std::string, BasicEventPtr>&
      basic_events() const {
    return basic_events_;
  }

  /// @returns The probability with the rare-event approximation.
  ///
  /// @note The user should make sure that the analysis is actually done.
  inline double p_rare() const { return p_rare_; }

  /// @returns Analysis time spent on calculating the total probability.
  inline double prob_analysis_time() const { return p_time_; }

  /// @returns Analysis time spent on calculating the importance factors.
  inline double imp_analysis_time() const { return imp_time_; }

 protected:
  using FlatSet = boost::container::flat_set<int>;  ///< Faster set.

  /// Assigns an index to each basic event,
  /// and then populates with these indices
  /// new databases and basic-to-integer converting maps.
  /// The previous data are lost.
  /// These indices will be used for future analysis.
  void AssignIndices() noexcept;

  /// Populates databases of minimal cut sets
  /// with indices of the events.
  /// This traversal detects
  /// if cut sets contain complement events
  /// and turns non-coherent analysis.
  ///
  /// @param[in] min_cut_sets Minimal cut sets with event IDs.
  void IndexMcs(const std::set<std::set<std::string> >& min_cut_sets) noexcept;

  /// Calculates probabilities
  /// using the minimal cut set upper bound (MCUB) approximation.
  ///
  /// @param[in] min_cut_sets Sets of indices of basic events.
  ///
  /// @returns The total probability with the MCUB approximation.
  double ProbMcub(const std::vector<FlatSet>& min_cut_sets) noexcept;

  /// Calculates probabilities
  /// using the Rare-Event approximation.
  ///
  /// @param[in] min_cut_sets Sets of indices of basic events.
  ///
  /// @returns The total probability with the rare-event approximation.
  double ProbRareEvent(const std::vector<FlatSet>& min_cut_sets) noexcept;

  /// Calculates a probability of a cut set,
  /// whose members are in AND relationship with each other.
  /// This function assumes independence of each member.
  ///
  /// @param[in] cut_set A flat set of indices of basic events.
  ///
  /// @returns The total probability of the set.
  ///
  /// @note O_avg(N) where N is the size of the passed set.
  double ProbAnd(const FlatSet& cut_set) noexcept;

  /// Calculates the total probability
  /// using the fault tree directly
  /// without cut sets.
  ///
  /// @todo Replace the main probability calculation functionality
  ///       with BDD based approach.
  double CalculateTotalProbability() noexcept;

  /// Calculates exact probability
  /// of a function graph represented by its root BDD vertex.
  ///
  /// @param[in] vertex The root vertex of a function graph.
  /// @param[in] mark A flag to mark traversed vertices.
  ///
  /// @returns Probability value.
  ///
  /// @warning If a vertice is already marked with the input mark,
  ///          it will not be traversed and updated with a probability value.
  double CalculateProbability(const VertexPtr& vertex, bool mark) noexcept;

  /// Importance analysis of basic events that are in minimal cut sets.
  void PerformImportanceAnalysis() noexcept;

  GatePtr top_event_;  ///< Top gate of the passed fault tree.
  std::unique_ptr<Bdd> bdd_graph_;  ///< The main BDD graph for analysis.
  const Settings kSettings_;  ///< All settings for analysis.
  std::string warnings_;  ///< Register warnings.

  /// Container for input basic events.
  std::unordered_map<std::string, BasicEventPtr> basic_events_;
  std::vector<BasicEventPtr> ordered_basic_events_;  ///< Ordering by indices.

  std::vector<BasicEventPtr> index_to_basic_;  ///< Indices to basic events.
  /// Indices of basic events.
  std::unordered_map<std::string, int> id_to_index_;
  std::vector<double> var_probs_;  ///< Variable probabilities.

  /// Minimal cut sets passed for analysis.
  std::set<std::set<std::string>> min_cut_sets_;

  /// Minimal cut sets with indices of events.
  std::vector<boost::container::flat_set<int>> imcs_;
  /// Indices min cut sets to strings min cut sets mapping.
  /// The same position as in imcs_ container is assumed.
  std::vector<std::set<std::string>> imcs_to_smcs_;
  /// Container for basic event indices that are in minimal cut sets.
  std::set<int> mcs_basic_events_;

  double p_total_;  ///< Total probability of the top event.
  double p_rare_;  ///< Total probability applying the rare-event approximation.
  bool current_mark_; ///< To keep track of BDD current mark.

  /// Container for minimal cut sets and their respective probabilities.
  std::map<std::set<std::string>, double> prob_of_min_sets_;

  /// Container for basic event importance factors.
  std::unordered_map<std::string, ImportanceFactors> importance_;

  bool coherent_;  ///< Indication of coherent optimized analysis.
  double p_time_;  ///< Time for probability calculations.
  double imp_time_;  ///< Time for importance calculations.
};

}  // namespace scram

#endif  // SCRAM_SRC_PROBABILITY_ANALYSIS_H_
