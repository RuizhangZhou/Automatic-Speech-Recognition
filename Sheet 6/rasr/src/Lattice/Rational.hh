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
#ifndef _LATTICE_RATIONAL_HH
#define _LATTICE_RATIONAL_HH

#include "Lattice.hh"
#include <Core/Vector.hh>

namespace Lattice {

ConstWordLatticeRef transpose(ConstWordLatticeRef l, bool progress = false,
                              WordBoundary final = WordBoundary());
ConstWordLatticeRef unite(ConstWordLatticeRef, ConstWordLatticeRef);
ConstWordLatticeRef unite(const Core::Vector<ConstWordLatticeRef> &);

} // namespace Lattice

#endif // _LATTICE_RATIONAL_HH
