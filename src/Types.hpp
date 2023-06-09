/*****************************************************************************/
/*                                                                           */
/*       COPYRIGHT (C) 2015 Lehrstuhl fuer Informatik VI, RWTH Aachen        */
/*                                                                           */
/*****************************************************************************/

#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <stdint.h>
#include <vector>
#include <utility>
#include <stddef.h>

typedef size_t   SegmentIdx;
typedef size_t   WordIdx;
typedef uint16_t StateIdx;
typedef uint16_t DensityIdx;

struct MixtureDensity {
  DensityIdx mean_idx;
  DensityIdx var_idx;

  MixtureDensity(DensityIdx mean_idx, DensityIdx var_idx)
                : mean_idx(mean_idx), var_idx(var_idx) {}
};

typedef std::vector<MixtureDensity> Mixture;

struct AlignmentItem {
  uint16_t count;
  StateIdx state;
  float    weight;

  AlignmentItem() : count(0u), state(0u), weight(0.0) {}//constructor without parameters, then set variables as defaulted values

  AlignmentItem(uint16_t count, StateIdx state, float weight)
               : count(count), state(state), weight(weight) {}//equal to this.count=count; count outside is the variable, inside is the value
};

typedef std::vector<AlignmentItem> Alignment;

typedef std::vector<WordIdx>::const_iterator WordIter;
/*
 const_iterator: C++ defines a type called const_iterator for each container type,
 which can only be used to read the elements inside the container,
 but cannot change its value. Dereferencing the const_iterator type yields a reference to a const object.
 */

#include "Eigen/Core"
#include <valarray>

using Matr = Eigen::MatrixXf;
using Vect = Eigen::VectorXf;
using BufferT = std::vector<Matr>;

Vect valarr_to_vect(const std::valarray<float>& v);
Matr valarr_to_matr(const std::valarray<float>& v, const std::gslice& slice);
std::valarray<float> matr_to_valarr(const Matr& m);
std::ostream& operator<<(std::ostream& out, const std::valarray<float>& val);

#endif /* TYPES_HPP */
