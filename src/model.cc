/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// Implementation of functions in Model class.

#include "model.h"

#include "error.h"
#include "ext/find_iterator.h"
#include "ext/multi_index.h"

namespace scram::mef {

const char Model::kDefaultName[] = "__unnamed-model__";

Model::Model(std::string name)
    : Element(name.empty() ? kDefaultName : std::move(name)),
      mission_time_(std::make_unique<MissionTime>()) {}

void Model::Add(InitiatingEventPtr initiating_event) {
  mef::AddElement(std::move(initiating_event), &initiating_events_,
                  "initiating event");
}

void Model::Add(EventTreePtr event_tree) {
  mef::AddElement(std::move(event_tree), &event_trees_, "event tree");
}

void Model::Add(SequencePtr sequence) {
  mef::AddElement(std::move(sequence), &sequences_, "sequence");
}

void Model::Add(RulePtr rule) {
  mef::AddElement(std::move(rule), &rules_, "rule");
}

void Model::Add(FaultTreePtr fault_tree) {
  mef::AddElement(std::move(fault_tree), &fault_trees_, "fault tree");
}

void Model::Add(AlignmentPtr alignment) {
  mef::AddElement(std::move(alignment), &alignments_, "alignment");
}

void Model::Add(SubstitutionPtr substitution) {
  mef::AddElement(std::move(substitution), &substitutions_, "substitution");
}

void Model::Add(ParameterPtr parameter) {
  mef::AddElement(std::move(parameter), &parameters_, "parameter");
}

void Model::CheckDuplicateEvent(const Event& event) {
  const std::string& id = event.id();
  if (gates_.count(id) || basic_events_.count(id) || house_events_.count(id))
    SCRAM_THROW(DuplicateElementError()) << errinfo_element(id, "event");
}

void Model::Add(HouseEventPtr house_event) {
  CheckDuplicateEvent(*house_event);
  house_events_.insert(std::move(house_event));
}

void Model::Add(BasicEventPtr basic_event) {
  CheckDuplicateEvent(*basic_event);
  basic_events_.insert(std::move(basic_event));
}

void Model::Add(GatePtr gate) {
  CheckDuplicateEvent(*gate);
  gates_.insert(std::move(gate));
}

void Model::Add(CcfGroupPtr ccf_group) {
  mef::AddElement(std::move(ccf_group), &ccf_groups_, "CCF group");
}

void Model::Add(std::unique_ptr<ExternLibrary> library) {
  mef::AddElement(std::move(library), &libraries_, "extern library");
}

void Model::Add(ExternFunctionPtr extern_function) {
  mef::AddElement(std::move(extern_function), &extern_functions_,
                  "extern function");
}

Formula::ArgEvent Model::GetEvent(const std::string& id) {
  if (auto it = ext::find(basic_events(), id))
    return it->get();
  if (auto it = ext::find(gates(), id))
    return it->get();
  if (auto it = ext::find(house_events(), id))
    return it->get();
  SCRAM_THROW(UndefinedElement("The event " + id + " is not in the model."));
}

namespace {

/// Helper function to remove events from containers.
template <class T, class Table>
std::unique_ptr<T> RemoveEvent(T* event, Table* table) {
  auto it = table->find(event->id());
  if (it == table->end())
    SCRAM_THROW(
        UndefinedElement("Event " + event->id() + " is not in the model."));

  if (it->get() != event)
    SCRAM_THROW(UndefinedElement("Duplicate event " + event->id() +
                                 " does not belong to the model."));
  return ext::extract(it, table);
}

}  // namespace

HouseEventPtr Model::Remove(HouseEvent* house_event) {
  return RemoveEvent(house_event, &house_events_);
}

BasicEventPtr Model::Remove(BasicEvent* basic_event) {
  return RemoveEvent(basic_event, &basic_events_);
}

GatePtr Model::Remove(Gate* gate) { return RemoveEvent(gate, &gates_); }

FaultTreePtr Model::Remove(FaultTree* fault_tree) {
  auto it = fault_trees_.find(fault_tree->name());
  if (it == fault_trees_.end())
    SCRAM_THROW(UndefinedElement("Fault tree " + fault_tree->name() +
                                 " is not in the model."));
  if (it->get() != fault_tree)
    SCRAM_THROW(UndefinedElement("Duplicate fault tree " + fault_tree->name() +
                                 " does not belong to the model."));
  return ext::extract(it, &fault_trees_);
}

}  // namespace scram::mef
