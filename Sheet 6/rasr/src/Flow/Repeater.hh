// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _FLOW_REPEATER_HH
#define _FLOW_REPEATER_HH

#include "Node.hh"

namespace Flow {

/** Flow network repeater */
class RepeaterNode : public Node {
public:
  static std::string filterName() { return "generic-repeater"; }
  RepeaterNode(const Core::Configuration &c);
  virtual ~RepeaterNode() {}

  /** returns 0 is @param name is "data"
   *  Remark: input is created only on demand to avoid dead input at
   * configuration.
   */
  virtual PortId getInput(const std::string &name);
  /** returns 0 is @param name is "data" */
  virtual PortId getOutput(const std::string &name);

  virtual bool work(PortId out);
};

} // namespace Flow

#endif // _FLOW_REPEATER_HH
